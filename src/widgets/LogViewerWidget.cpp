/*
 * Copyright 2017 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "LogViewerWidget.h"
#include "ui_LogViewerWidget.h"
#include "models/LogViewerModel.h"
#include "models/LogViewerFilterModel.h"
#include "delegates/LogViewerDelegate.h"
#include <quentier/utility/StandardPaths.h>
#include <quentier/utility/MessageBox.h>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QDir>
#include <QFileDialog>
#include <QColor>
#include <QTextStream>
#include <QClipboard>
#include <QApplication>

#define QUENTIER_NUM_LOG_LEVELS (6)
#define FETCHING_MORE_TIMER_PERIOD (800)
#define DELAY_SECTION_RESIZE_TIMER_PERIOD (3000)

namespace quentier {

LogViewerWidget::LogViewerWidget(QWidget * parent) :
    QWidget(parent, Qt::Window),
    m_pUi(new Ui::LogViewerWidget),
    m_logFilesFolderWatcher(),
    m_pLogViewerModel(new LogViewerModel(this)),
    m_pLogViewerFilterModel(new LogViewerFilterModel(this)),
    m_modelFetchingMoreTimer(),
    m_delayedSectionResizeTimer(),
    m_minLogLevelBeforeTracing(LogLevel::InfoLevel),
    m_filterByContentBeforeTracing(),
    m_filterByLogLevelBeforeTracing(),
    m_logLevelEnabledCheckboxPtrs()
{
    m_pUi->setupUi(this);
    m_pUi->resetPushButton->setEnabled(false);
    m_pUi->tracePushButton->setCheckable(true);

    m_pUi->statusBarLineEdit->hide();

    m_pUi->logEntriesTableView->verticalHeader()->hide();
    m_pUi->logEntriesTableView->setWordWrap(true);

    LogViewerDelegate * pDelegate = new LogViewerDelegate(m_pUi->logEntriesTableView);
    m_pUi->logEntriesTableView->setItemDelegate(pDelegate);

    m_pUi->filterByLogLevelTableWidget->horizontalHeader()->hide();
    m_pUi->filterByLogLevelTableWidget->verticalHeader()->hide();

    for(size_t i = 0; i < sizeof(m_filterByLogLevelBeforeTracing); ++i) {
        m_filterByLogLevelBeforeTracing[i] = true;
        m_logLevelEnabledCheckboxPtrs[i] = Q_NULLPTR;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    m_pUi->filterByContentLineEdit->setClearButtonEnabled(true);
#endif

    setupLogLevels();
    setupLogFiles();
    setupFilterByLogLevelWidget();
    startWatchingForLogFilesFolderChanges();

    m_pLogViewerFilterModel->setSourceModel(m_pLogViewerModel);
    m_pUi->logEntriesTableView->setModel(m_pLogViewerFilterModel);

    LogViewerDelegate * pLogViewerDelegate = new LogViewerDelegate(m_pUi->logEntriesTableView);
    m_pUi->logEntriesTableView->setItemDelegate(pLogViewerDelegate);

    QObject::connect(m_pUi->copyAllToClipboardPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(LogViewerWidget,onCopyAllToClipboardButtonPressed));
    QObject::connect(m_pUi->saveToFilePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(LogViewerWidget,onSaveAllToFileButtonPressed));
    QObject::connect(m_pUi->clearPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(LogViewerWidget,onClearButtonPressed));
    QObject::connect(m_pUi->resetPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(LogViewerWidget,onResetButtonPressed));
    QObject::connect(m_pUi->tracePushButton, QNSIGNAL(QPushButton,toggled,bool),
                     this, QNSLOT(LogViewerWidget,onTraceButtonToggled,bool));
    QObject::connect(m_pUi->filterByContentLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(LogViewerWidget,onFilterByContentEditingFinished));
    QObject::connect(m_pLogViewerModel, QNSIGNAL(LogViewerModel,notifyError,ErrorString),
                     this, QNSLOT(LogViewerWidget,onModelError,ErrorString));
    QObject::connect(m_pLogViewerModel, QNSIGNAL(LogViewerModel,rowsInserted,QModelIndex,int,int),
                     this, QNSLOT(LogViewerWidget,onModelRowsInserted,QModelIndex,int,int));
}

LogViewerWidget::~LogViewerWidget()
{
    delete m_pUi;
}

void LogViewerWidget::setupLogLevels()
{
    m_pUi->logLevelComboBox->addItem(LogViewerModel::logLevelToString(LogLevel::TraceLevel));
    m_pUi->logLevelComboBox->addItem(LogViewerModel::logLevelToString(LogLevel::DebugLevel));
    m_pUi->logLevelComboBox->addItem(LogViewerModel::logLevelToString(LogLevel::InfoLevel));
    m_pUi->logLevelComboBox->addItem(LogViewerModel::logLevelToString(LogLevel::WarnLevel));
    m_pUi->logLevelComboBox->addItem(LogViewerModel::logLevelToString(LogLevel::ErrorLevel));
    m_pUi->logLevelComboBox->addItem(LogViewerModel::logLevelToString(LogLevel::FatalLevel));

    m_pUi->logLevelComboBox->setCurrentIndex(QuentierMinLogLevel());
    QObject::connect(m_pUi->logLevelComboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(onCurrentLogLevelChanged(int)));
}

void LogViewerWidget::setupLogFiles()
{
    QDir dir(QuentierLogFilesDirPath());
    if (Q_UNLIKELY(!dir.exists())) {
        clear();
        return;
    }

    QFileInfoList entries = dir.entryInfoList(QDir::Files, QDir::Name);
    if (entries.isEmpty()) {
        clear();
        return;
    }

    QString originalLogFileName = m_pUi->logFileComboBox->currentText();
    QString currentLogFileName = originalLogFileName;
    if (currentLogFileName.isEmpty()) {
        currentLogFileName = QStringLiteral("Quentier-log.txt");
    }

    int numCurrentLogFileComboBoxItems = m_pUi->logFileComboBox->count();
    int numEntries = entries.size();
    if (numCurrentLogFileComboBoxItems == numEntries) {
        // as the number of entries didn't change, assuming there's no need
        // to change anything
        return;
    }

    QObject::disconnect(m_pUi->logFileComboBox, SIGNAL(currentIndexChanged(QString)),
                        this, SLOT(onCurrentLogFileChanged(QString)));

    m_pUi->logFileComboBox->clear();

    int currentLogFileIndex = -1;
    for(int i = 0, size = entries.size(); i < size; ++i)
    {
        const QFileInfo & entry = entries.at(i);
        QString fileName = entry.fileName();

        if (fileName == currentLogFileName) {
            currentLogFileIndex = i;
        }

        m_pUi->logFileComboBox->addItem(fileName);
    }

    if (currentLogFileIndex < 0) {
        currentLogFileIndex = 0;
    }

    m_pUi->logFileComboBox->setCurrentIndex(currentLogFileIndex);

    QObject::connect(m_pUi->logFileComboBox, SIGNAL(currentIndexChanged(QString)),
                     this, SLOT(onCurrentLogFileChanged(QString)), Qt::UniqueConnection);

    QString logFileName = entries.at(currentLogFileIndex).fileName();
    if (logFileName == originalLogFileName) {
        // The current log file didn't change, no need to set it to the model etc.
        return;
    }

    m_pLogViewerModel->setLogFileName(logFileName);
    resizeLogEntriesViewColumns();

    if (!m_modelFetchingMoreTimer.isActive()) {
        m_modelFetchingMoreTimer.start(FETCHING_MORE_TIMER_PERIOD, this);
    }
}

void LogViewerWidget::startWatchingForLogFilesFolderChanges()
{
    m_logFilesFolderWatcher.addPath(QuentierLogFilesDirPath());
    QObject::connect(&m_logFilesFolderWatcher, QNSIGNAL(FileSystemWatcher,directoryRemoved,QString),
                     this, QNSLOT(LogViewerWidget,onLogFileDirRemoved,QString));
    QObject::connect(&m_logFilesFolderWatcher, QNSIGNAL(FileSystemWatcher,directoryChanged,QString),
                     this, QNSLOT(LogViewerWidget,onLogFileDirChanged,QString));
}

void LogViewerWidget::setupFilterByLogLevelWidget()
{
    m_pUi->filterByLogLevelTableWidget->setRowCount(QUENTIER_NUM_LOG_LEVELS);
    m_pUi->filterByLogLevelTableWidget->setColumnCount(2);

    QColor rowColors[QUENTIER_NUM_LOG_LEVELS];
    rowColors[LogLevel::TraceLevel] = QColor(127, 230, 255);    // Light blue
    rowColors[LogLevel::DebugLevel] = QColor(127, 255, 142);    // Light green
    rowColors[LogLevel::InfoLevel]  = QColor(252, 255, 127);    // Light yellow
    rowColors[LogLevel::WarnLevel]  = QColor(255, 212, 127);    // Orange
    rowColors[LogLevel::ErrorLevel] = QColor(255, 128, 127);    // Pink
    rowColors[LogLevel::FatalLevel] = QColor(255, 38, 10);      // Red

    for(size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i)
    {
        QTableWidgetItem * pItem = new QTableWidgetItem(LogViewerModel::logLevelToString(static_cast<LogLevel::type>(i)));
        pItem->setData(Qt::BackgroundRole, rowColors[i]);
        m_pUi->filterByLogLevelTableWidget->setItem(static_cast<int>(i), 0, pItem);

        QWidget * pWidget = new QWidget;
        QCheckBox * pCheckbox = new QCheckBox;
        pCheckbox->setObjectName(QString::number(i));
        pCheckbox->setChecked(m_pLogViewerFilterModel->logLevelEnabled(static_cast<LogLevel::type>(i)));
        QHBoxLayout * pLayout = new QHBoxLayout(pWidget);
        pLayout->addWidget(pCheckbox);
        pLayout->setAlignment(Qt::AlignCenter);
        pLayout->setContentsMargins(0, 0, 0, 0);
        pWidget->setLayout(pLayout);
        m_pUi->filterByLogLevelTableWidget->setCellWidget(static_cast<int>(i), 1, pWidget);

        m_logLevelEnabledCheckboxPtrs[i] = pCheckbox;

        QObject::connect(pCheckbox, QNSIGNAL(QCheckBox,stateChanged,int),
                         this, QNSLOT(LogViewerWidget,onFilterByLogLevelCheckboxToggled,int));
    }
}

void LogViewerWidget::onCurrentLogLevelChanged(int index)
{
    QNDEBUG(QStringLiteral("LogViewerWidget::onCurrentLogLevelChanged: ") << index);

    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    if (Q_UNLIKELY((index < 0) || (index >= QUENTIER_NUM_LOG_LEVELS))) {
        return;
    }

    QuentierSetMinLogLevel(static_cast<LogLevel::type>(index));
}

void LogViewerWidget::onFilterByContentEditingFinished()
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    m_pLogViewerFilterModel->setFilterWildcard(m_pUi->filterByContentLineEdit->text());
}

void LogViewerWidget::onFilterByLogLevelCheckboxToggled(int state)
{
    QNDEBUG(QStringLiteral("LogViewerWidget::onFilterByLogLevelCheckboxToggled: state = ") << state);

    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    QCheckBox * pCheckbox = qobject_cast<QCheckBox*>(sender());
    if (Q_UNLIKELY(!pCheckbox)) {
        return;
    }

    bool parsedCheckboxRow = false;
    int checkboxRow = pCheckbox->objectName().toInt(&parsedCheckboxRow);
    if (Q_UNLIKELY(!parsedCheckboxRow)) {
        return;
    }

    if (Q_UNLIKELY((checkboxRow < 0) || (checkboxRow >= QUENTIER_NUM_LOG_LEVELS))) {
        return;
    }

    m_pLogViewerFilterModel->setLogLevelEnabled(static_cast<LogLevel::type>(checkboxRow), (state == Qt::Checked));
}

void LogViewerWidget::onCurrentLogFileChanged(const QString & currentLogFile)
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    if (currentLogFile.isEmpty()) {
        return;
    }

    m_pLogViewerModel->setLogFileName(currentLogFile);
}

void LogViewerWidget::onLogFileDirRemoved(const QString & path)
{
    if (path == QuentierLogFilesDirPath()) {
        clear();
    }
}

void LogViewerWidget::onLogFileDirChanged(const QString & path)
{
    if (path == QuentierLogFilesDirPath()) {
        setupLogFiles();
    }
}

void LogViewerWidget::onCopyAllToClipboardButtonPressed()
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    QClipboard * pClipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!pClipboard)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't copy data to clipboard: got null pointer to clipboard from app"));
        QNWARNING(errorDescription);
        m_pUi->statusBarLineEdit->setText(errorDescription.localizedString());
        m_pUi->statusBarLineEdit->show();
        return;
    }

    QString text = displayedLogEntriesToString();
    pClipboard->setText(text);
}

void LogViewerWidget::onSaveAllToFileButtonPressed()
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    QString absoluteFilePath = QFileDialog::getSaveFileName(this, tr("Save as") + QStringLiteral("..."),
                                                            documentsPath());
    QFileInfo fileInfo(absoluteFilePath);
    if (fileInfo.exists())
    {
        if (Q_UNLIKELY(!fileInfo.isFile())) {
            ErrorString errorDescription(QT_TR_NOOP("Can't save the log to file: the selected entity is not a file"));
            m_pUi->statusBarLineEdit->setText(errorDescription.localizedString());
            m_pUi->statusBarLineEdit->show();
            Q_UNUSED(errorDescription)
            return;
        }

        int overwriteFile = questionMessageBox(this, tr("Overwrite"),
                                               tr("File") + QStringLiteral(" ") + fileInfo.fileName() + QStringLiteral(" ") +
                                               tr("already exists"), tr("Are you sure you want to overwrite the existing file?"));
        if (overwriteFile != QMessageBox::Ok) {
            return;
        }
    }
    else
    {
        QDir fileDir(fileInfo.absoluteDir());
        if (!fileDir.exists() && !fileDir.mkpath(fileDir.absolutePath())) {
            ErrorString errorDescription(QT_TR_NOOP("Failed to create the folder to contain the file with saved logs"));
            m_pUi->statusBarLineEdit->setText(errorDescription.localizedString());
            m_pUi->statusBarLineEdit->show();
            Q_UNUSED(errorDescription)
            return;
        }
    }

    QString text = displayedLogEntriesToString();
    if (text.isEmpty()) {
        ErrorString errorDescription(QT_TR_NOOP("No logs to save"));
        m_pUi->statusBarLineEdit->setText(errorDescription.localizedString());
        m_pUi->statusBarLineEdit->show();
        Q_UNUSED(errorDescription)
        return;
    }

    QFile file(fileInfo.absoluteFilePath());
    bool open = file.open(QIODevice::WriteOnly);
    if (!open) {
        ErrorString errorDescription(QT_TR_NOOP("Failed to open the target file for writing"));
        m_pUi->statusBarLineEdit->setText(errorDescription.localizedString());
        m_pUi->statusBarLineEdit->show();
        Q_UNUSED(errorDescription)
        return;
    }

    Q_UNUSED(file.write(text.toLocal8Bit()))
    file.close();
}

void LogViewerWidget::onClearButtonPressed()
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    int numRows = m_pLogViewerModel->rowCount();
    m_pLogViewerFilterModel->setFilterOutBeforeRow(numRows);
    m_pUi->resetPushButton->setEnabled(true);
}

void LogViewerWidget::onResetButtonPressed()
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    m_pLogViewerFilterModel->setFilterOutBeforeRow(0);
    m_pUi->resetPushButton->setEnabled(false);
}

void LogViewerWidget::onTraceButtonToggled(bool checked)
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    if (checked)
    {
        m_pUi->tracePushButton->setText(tr("Stop tracing"));

        // Clear the currently displayed log entries to create some space for new ones
        onClearButtonPressed();
        m_pUi->resetPushButton->setEnabled(false);

        // Backup the settings used before tracing
        m_minLogLevelBeforeTracing = QuentierMinLogLevel();
        m_filterByContentBeforeTracing = m_pUi->filterByContentLineEdit->text();
        for(size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i) {
            m_filterByLogLevelBeforeTracing[i] = m_pLogViewerFilterModel->logLevelEnabled(static_cast<LogLevel::type>(i));
        }

        // Enable tracing
        QuentierSetMinLogLevel(LogLevel::TraceLevel);

        m_pUi->filterByContentLineEdit->setText(QString());
        onFilterByContentEditingFinished();

        for(size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i) {
            m_pLogViewerFilterModel->setLogLevelEnabled(static_cast<LogLevel::type>(i), true);
            m_logLevelEnabledCheckboxPtrs[i]->setChecked(true);
        }
        m_pUi->filterByLogLevelTableWidget->setEnabled(false);
    }
    else
    {
        m_pUi->tracePushButton->setText(tr("Trace"));

        // Restore the previously backed up settings
        QuentierSetMinLogLevel(m_minLogLevelBeforeTracing);
        m_minLogLevelBeforeTracing = LogLevel::InfoLevel;

        m_pUi->filterByContentLineEdit->setText(m_filterByContentBeforeTracing);
        onFilterByContentEditingFinished();
        m_filterByContentBeforeTracing.clear();

        for(size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i) {
            m_pLogViewerFilterModel->setLogLevelEnabled(static_cast<LogLevel::type>(i), m_filterByLogLevelBeforeTracing[i]);
            m_logLevelEnabledCheckboxPtrs[i]->setChecked(m_filterByLogLevelBeforeTracing[i]);
            m_filterByLogLevelBeforeTracing[i] = false;
        }
        m_pUi->filterByLogLevelTableWidget->setEnabled(true);

        onResetButtonPressed();
    }

    scheduleLogEntriesViewColumnsResize();
}

void LogViewerWidget::onModelError(ErrorString errorDescription)
{
    m_pUi->statusBarLineEdit->setText(errorDescription.localizedString());
    m_pUi->statusBarLineEdit->show();
}

void LogViewerWidget::onModelRowsInserted(const QModelIndex & parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
    scheduleLogEntriesViewColumnsResize();
}

void LogViewerWidget::clear()
{
    m_pUi->logFileComboBox->clear();
    m_pLogViewerModel->clear();
    m_modelFetchingMoreTimer.stop();

    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();
}

void LogViewerWidget::scheduleLogEntriesViewColumnsResize()
{
    if (m_delayedSectionResizeTimer.isActive()) {
        // Already scheduled
        return;
    }

    m_delayedSectionResizeTimer.start(DELAY_SECTION_RESIZE_TIMER_PERIOD, this);
}

void LogViewerWidget::resizeLogEntriesViewColumns()
{
    m_pUi->logEntriesTableView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    m_pUi->logEntriesTableView->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
}

QString LogViewerWidget::displayedLogEntriesToString() const
{
    QString result;
    QTextStream strm(&result);

    int fromLine = m_pLogViewerFilterModel->filterOutBeforeRow();
    int numEntries = m_pLogViewerModel->rowCount();
    for(int i = fromLine; i < numEntries; ++i)
    {
        if (!m_pLogViewerFilterModel->filterAcceptsRow(i, QModelIndex())) {
            continue;
        }

        const LogViewerModel::Data * pDataEntry = m_pLogViewerModel->dataEntry(i);
        if (Q_UNLIKELY(!pDataEntry)) {
            continue;
        }

        strm << pDataEntry->m_timestamp.toString(
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
                                                 QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz t")
#else
                                                 QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz t")
#endif
                                          )
             << QStringLiteral(" ")
             << pDataEntry->m_sourceFileName
             << QStringLiteral(" @ ")
             << QString::number(pDataEntry->m_sourceFileLineNumber)
             << QStringLiteral(" [")
             << LogViewerModel::logLevelToString(pDataEntry->m_logLevel)
             << QStringLiteral("]: ")
             << pDataEntry->m_logEntry
             << QStringLiteral("\n");
    }

    strm.flush();
    return result;
}

void LogViewerWidget::timerEvent(QTimerEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return;
    }

    if (pEvent->timerId() == m_modelFetchingMoreTimer.timerId())
    {
        if (m_pLogViewerModel->canFetchMore(QModelIndex())) {
            m_pLogViewerModel->fetchMore(QModelIndex());
            scheduleLogEntriesViewColumnsResize();
        }
        else {
            m_modelFetchingMoreTimer.stop();
        }
    }
    else if (pEvent->timerId() == m_delayedSectionResizeTimer.timerId())
    {
        resizeLogEntriesViewColumns();
        m_delayedSectionResizeTimer.stop();
    }
}

} // namespace quentier
