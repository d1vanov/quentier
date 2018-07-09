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
#include "delegates/LogViewerDelegate.h"
#include "../SettingsNames.h"
#include <quentier/utility/StandardPaths.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/ApplicationSettings.h>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QDir>
#include <QFileDialog>
#include <QColor>
#include <QTextStream>
#include <QClipboard>
#include <QApplication>
#include <QMenu>
#include <QCloseEvent>
#include <set>
#include <cmath>

#define QUENTIER_NUM_LOG_LEVELS (5)
#define FETCHING_MORE_TIMER_PERIOD (200)
#define DELAY_SECTION_RESIZE_TIMER_PERIOD (100)

namespace quentier {

LogViewerWidget::LogViewerWidget(QWidget * parent) :
    QWidget(parent, Qt::Window),
    m_pUi(new Ui::LogViewerWidget),
    m_logFilesFolderWatcher(),
    m_pLogViewerModel(new LogViewerModel(this)),
    m_delayedSectionResizeTimer(),
    m_logLevelEnabledCheckboxPtrs(),
    m_pLogEntriesContextMenu(Q_NULLPTR),
    m_minLogLevelBeforeTracing(LogLevel::InfoLevel),
    m_filterByContentBeforeTracing(),
    m_filterByLogLevelBeforeTracing(),
    m_startLogFilePosBeforeTracing(-1)
{
    m_pUi->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    m_pUi->resetPushButton->setEnabled(false);
    m_pUi->tracePushButton->setCheckable(true);

    m_pUi->saveToFileProgressBar->hide();
    m_pUi->saveToFileCancelButton->hide();

    m_pUi->statusBarLineEdit->hide();

    m_pUi->logEntriesTableView->verticalHeader()->hide();
    m_pUi->logEntriesTableView->setWordWrap(true);

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

    m_pUi->logEntriesTableView->setModel(m_pLogViewerModel);

    LogViewerDelegate * pLogViewerDelegate = new LogViewerDelegate(m_pUi->logEntriesTableView);
    m_pUi->logEntriesTableView->setItemDelegate(pLogViewerDelegate);

    QObject::connect(m_pUi->saveToFilePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(LogViewerWidget,onSaveLogToFileButtonPressed));
    QObject::connect(m_pUi->clearPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(LogViewerWidget,onClearButtonPressed));
    QObject::connect(m_pUi->resetPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(LogViewerWidget,onResetButtonPressed));
    QObject::connect(m_pUi->tracePushButton, QNSIGNAL(QPushButton,toggled,bool),
                     this, QNSLOT(LogViewerWidget,onTraceButtonToggled,bool));
    QObject::connect(m_pUi->logFileWipePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(LogViewerWidget,onWipeLogPushButtonPressed));
    QObject::connect(m_pUi->filterByContentLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(LogViewerWidget,onFilterByContentEditingFinished));
    QObject::connect(m_pLogViewerModel, QNSIGNAL(LogViewerModel,notifyError,ErrorString),
                     this, QNSLOT(LogViewerWidget,onModelError,ErrorString));
    QObject::connect(m_pLogViewerModel, QNSIGNAL(LogViewerModel,rowsInserted,QModelIndex,int,int),
                     this, QNSLOT(LogViewerWidget,onModelRowsInserted,QModelIndex,int,int));
    QObject::connect(m_pUi->logEntriesTableView, QNSIGNAL(QTableView,customContextMenuRequested,QPoint),
                     this, QNSLOT(LogViewerWidget,onLogEntriesViewContextMenuRequested,QPoint));
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

    m_pUi->logLevelComboBox->setCurrentIndex(QuentierMinLogLevel());
    QObject::connect(m_pUi->logLevelComboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(onCurrentLogLevelChanged(int)), Qt::UniqueConnection);
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

    LogViewerModel::FilteringOptions filteringOptions;
    collectModelFilteringOptions(filteringOptions);
    m_pLogViewerModel->setLogFileName(logFileName, filteringOptions);

    if (m_pLogViewerModel->currentLogFileSize() != 0) {
        showLogFileIsLoadingLabel();
        scheduleLogEntriesViewColumnsResize();
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

    const QVector<LogLevel::type> & disabledLogLevels = m_pLogViewerModel->disabledLogLevels();

    for(size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i)
    {
        QTableWidgetItem * pItem = new QTableWidgetItem(LogViewerModel::logLevelToString(static_cast<LogLevel::type>(i)));
        pItem->setData(Qt::BackgroundRole, m_pLogViewerModel->backgroundColorForLogLevel(static_cast<LogLevel::type>(i)));
        m_pUi->filterByLogLevelTableWidget->setItem(static_cast<int>(i), 0, pItem);

        QWidget * pWidget = new QWidget;
        QCheckBox * pCheckbox = new QCheckBox;
        pCheckbox->setObjectName(QString::number(i));
        pCheckbox->setChecked(!disabledLogLevels.contains(static_cast<LogLevel::type>(i)));
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

    if (Q_UNLIKELY(m_pLogViewerModel->isSavingModelEntriesToFileInProgress())) {
        return;
    }

    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    if (Q_UNLIKELY((index < 0) || (index >= QUENTIER_NUM_LOG_LEVELS))) {
        return;
    }

    QuentierSetMinLogLevel(static_cast<LogLevel::type>(index));

    ApplicationSettings appSettings;
    appSettings.beginGroup(LOGGING_SETTINGS_GROUP);
    appSettings.setValue(CURRENT_MIN_LOG_LEVEL, index);
    appSettings.endGroup();
}

void LogViewerWidget::onFilterByContentEditingFinished()
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    m_pLogViewerModel->setLogEntryContentFilter(m_pUi->filterByContentLineEdit->text());

    showLogFileIsLoadingLabel();
    scheduleLogEntriesViewColumnsResize();
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

    QVector<LogLevel::type> disabledLogLevels = m_pLogViewerModel->disabledLogLevels();
    int rowIndex = disabledLogLevels.indexOf(static_cast<LogLevel::type>(checkboxRow));
    bool currentRowWasDisabled = (rowIndex >= 0);
    if (currentRowWasDisabled && (state == Qt::Checked)) {
        disabledLogLevels.remove(rowIndex);
    }
    else if (!currentRowWasDisabled && (state == Qt::Unchecked)) {
        disabledLogLevels << static_cast<LogLevel::type>(checkboxRow);
    }
    else {
        return;
    }

    m_pLogViewerModel->setDisabledLogLevels(disabledLogLevels);

    showLogFileIsLoadingLabel();
    scheduleLogEntriesViewColumnsResize();
}

void LogViewerWidget::onCurrentLogFileChanged(const QString & currentLogFile)
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    if (currentLogFile.isEmpty()) {
        return;
    }

    LogViewerModel::FilteringOptions filteringOptions;
    collectModelFilteringOptions(filteringOptions);
    m_pLogViewerModel->setLogFileName(currentLogFile);

    if (m_pLogViewerModel->currentLogFileSize() != 0) {
        showLogFileIsLoadingLabel();
    }
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

void LogViewerWidget::onSaveLogToFileButtonPressed()
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

    m_pUi->saveToFileLabel->setText(tr("Saving the log to file") + QStringLiteral("..."));
    m_pUi->saveToFileProgressBar->setMinimum(0);
    m_pUi->saveToFileProgressBar->setMaximum(100);
    m_pUi->saveToFileProgressBar->setValue(0);
    m_pUi->saveToFileProgressBar->show();

    // Disable UI elements controlling the selected file, filtering etc
    // in order to prevent screwing up the process of saving stuff to file
    m_pUi->saveToFilePushButton->setEnabled(false);
    m_pUi->clearPushButton->setEnabled(false);
    m_pUi->resetPushButton->setEnabled(false);
    m_pUi->tracePushButton->setEnabled(false);
    m_pUi->logFileWipePushButton->setEnabled(false);
    m_pUi->filterByContentLineEdit->setEnabled(false);
    m_pUi->filterByLogLevelTableWidget->setEnabled(false);
    m_pUi->logFileComboBox->setEnabled(false);
    m_pUi->logLevelComboBox->setEnabled(false);

    QObject::connect(m_pUi->saveToFileCancelButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(LogViewerWidget,onCancelSavingTheLogToFileButtonPressed));
    m_pUi->saveToFileCancelButton->show();

    QObject::connect(m_pLogViewerModel, QNSIGNAL(LogViewerModel,saveModelEntriesToFileFinished,ErrorString),
                     this, QNSLOT(LogViewerWidget,onSaveModelEntriesToFileFinished,ErrorString));
    QObject::connect(m_pLogViewerModel, QNSIGNAL(LogViewerModel,saveModelEntriesToFileProgress,double),
                     this, QNSLOT(LogViewerWidget,onSaveModelEntriesToFileProgress,double));
    m_pLogViewerModel->saveModelEntriesToFile(fileInfo.absoluteFilePath());
}

void LogViewerWidget::onCancelSavingTheLogToFileButtonPressed()
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    m_pLogViewerModel->cancelSavingModelEntriesToFile();

    m_pUi->saveToFileLabel->setText(QString());

    QObject::disconnect(m_pUi->saveToFileCancelButton, QNSIGNAL(QPushButton,clicked),
                        this, QNSLOT(LogViewerWidget,onCancelSavingTheLogToFileButtonPressed));
    m_pUi->saveToFileCancelButton->hide();

    m_pUi->saveToFileProgressBar->setValue(0);
    m_pUi->saveToFileProgressBar->hide();

    enableUiElementsAfterSavingLogToFile();

#ifdef Q_OS_MAC
    // There are rendering issues on Mac, the progress bar sometimes is not properly hidden,
    // trying to workaround through scheduling the repaint
    repaint();
#endif
}

void LogViewerWidget::onClearButtonPressed()
{
    if (Q_UNLIKELY(m_pLogViewerModel->isSavingModelEntriesToFileInProgress())) {
        return;
    }

    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    m_pLogViewerModel->setStartLogFilePosAfterCurrentFileSize();
    m_pUi->resetPushButton->setEnabled(true);
}

void LogViewerWidget::onResetButtonPressed()
{
    if (Q_UNLIKELY(m_pLogViewerModel->isSavingModelEntriesToFileInProgress())) {
        return;
    }

    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    m_pLogViewerModel->setStartLogFilePos(0);
    m_pUi->resetPushButton->setEnabled(false);
    showLogFileIsLoadingLabel();
}

void LogViewerWidget::onTraceButtonToggled(bool checked)
{
    if (Q_UNLIKELY(m_pLogViewerModel->isSavingModelEntriesToFileInProgress())) {
        return;
    }

    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    if (checked)
    {
        m_pUi->tracePushButton->setText(tr("Stop tracing"));

        m_startLogFilePosBeforeTracing = m_pLogViewerModel->startLogFilePos();

        m_pUi->resetPushButton->setEnabled(false);

        // Backup the settings used before tracing
        m_minLogLevelBeforeTracing = QuentierMinLogLevel();
        m_filterByContentBeforeTracing = m_pUi->filterByContentLineEdit->text();
        QVector<LogLevel::type> disabledLogLevels = m_pLogViewerModel->disabledLogLevels();
        for(size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i) {
            m_filterByLogLevelBeforeTracing[i] = !disabledLogLevels.contains(static_cast<LogLevel::type>(i));
        }

        // Enable tracing
        QuentierSetMinLogLevel(LogLevel::TraceLevel);

        QObject::disconnect(m_pUi->logLevelComboBox, SIGNAL(currentIndexChanged(int)),
                            this, SLOT(onCurrentLogLevelChanged(int)));
        m_pUi->logLevelComboBox->setCurrentIndex(0);
        QObject::connect(m_pUi->logLevelComboBox, SIGNAL(currentIndexChanged(int)),
                         this, SLOT(onCurrentLogLevelChanged(int)), Qt::UniqueConnection);

        m_pUi->logLevelComboBox->setEnabled(false);

        m_pUi->filterByContentLineEdit->setText(QString());

        for(size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i)
        {
            QCheckBox * pCheckbox = m_logLevelEnabledCheckboxPtrs[i];
            QObject::disconnect(pCheckbox, QNSIGNAL(QCheckBox,stateChanged,int),
                                this, QNSLOT(LogViewerWidget,onFilterByLogLevelCheckboxToggled,int));
            pCheckbox->setChecked(true);
            QObject::connect(pCheckbox, QNSIGNAL(QCheckBox,stateChanged,int),
                             this, QNSLOT(LogViewerWidget,onFilterByLogLevelCheckboxToggled,int));
        }

        m_pUi->filterByLogLevelTableWidget->setEnabled(false);
        m_pUi->logFileWipePushButton->setEnabled(false);

        LogViewerModel::FilteringOptions filteringOptions;
        filteringOptions.m_startLogFilePos = m_pLogViewerModel->currentLogFileSize();
        m_pLogViewerModel->setLogFileName(m_pLogViewerModel->logFileName(),
                                          filteringOptions);
    }
    else
    {
        m_pUi->tracePushButton->setText(tr("Trace"));

        // Restore the previously backed up settings
        QuentierSetMinLogLevel(m_minLogLevelBeforeTracing);

        QObject::disconnect(m_pUi->logLevelComboBox, SIGNAL(currentIndexChanged(int)),
                            this, SLOT(onCurrentLogLevelChanged(int)));
        m_pUi->logLevelComboBox->setCurrentIndex(m_minLogLevelBeforeTracing);
        QObject::connect(m_pUi->logLevelComboBox, SIGNAL(currentIndexChanged(int)),
                         this, SLOT(onCurrentLogLevelChanged(int)), Qt::UniqueConnection);

        m_pUi->logLevelComboBox->setEnabled(true);

        m_minLogLevelBeforeTracing = LogLevel::InfoLevel;

        LogViewerModel::FilteringOptions filteringOptions;

        if (m_startLogFilePosBeforeTracing > 0) {
            filteringOptions.m_startLogFilePos = m_startLogFilePosBeforeTracing;
            m_pUi->resetPushButton->setEnabled(true);
        }
        else {
            m_pUi->resetPushButton->setEnabled(false);
        }
        m_startLogFilePosBeforeTracing = -1;

        filteringOptions.m_logEntryContentFilter = m_filterByContentBeforeTracing;
        m_pUi->filterByContentLineEdit->setText(m_filterByContentBeforeTracing);
        m_filterByContentBeforeTracing.clear();

        filteringOptions.m_disabledLogLevels.reserve(QUENTIER_NUM_LOG_LEVELS);
        for(size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i)
        {
            if (!m_filterByLogLevelBeforeTracing[i]) {
                filteringOptions.m_disabledLogLevels << static_cast<LogLevel::type>(i);
            }

            QCheckBox * pCheckbox = m_logLevelEnabledCheckboxPtrs[i];
            QObject::disconnect(pCheckbox, QNSIGNAL(QCheckBox,stateChanged,int),
                                this, QNSLOT(LogViewerWidget,onFilterByLogLevelCheckboxToggled,int));
            m_logLevelEnabledCheckboxPtrs[i]->setChecked(m_filterByLogLevelBeforeTracing[i]);
            QObject::connect(pCheckbox, QNSIGNAL(QCheckBox,stateChanged,int),
                             this, QNSLOT(LogViewerWidget,onFilterByLogLevelCheckboxToggled,int));
        }

        m_pLogViewerModel->setLogFileName(m_pLogViewerModel->logFileName(), filteringOptions);

        m_pUi->filterByLogLevelTableWidget->setEnabled(true);
        m_pUi->logFileWipePushButton->setEnabled(true);
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
    m_pUi->logFilePendingLoadLabel->setText(QString());
}

void LogViewerWidget::onSaveModelEntriesToFileFinished(ErrorString errorDescription)
{
    QObject::disconnect(m_pLogViewerModel, QNSIGNAL(LogViewerModel,saveModelEntriesToFileFinished,ErrorString),
                        this, QNSLOT(LogViewerWidget,onSaveModelEntriesToFileFinished,ErrorString));
    QObject::disconnect(m_pLogViewerModel, QNSIGNAL(LogViewerModel,saveModelEntriesToFileProgress,double),
                        this, QNSLOT(LogViewerWidget,onSaveModelEntriesToFileProgress,double));

    m_pUi->saveToFileLabel->setText(QString());

    QObject::disconnect(m_pUi->saveToFileCancelButton, QNSIGNAL(QPushButton,clicked),
                        this, QNSLOT(LogViewerWidget,onCancelSavingTheLogToFileButtonPressed));
    m_pUi->saveToFileCancelButton->hide();

    m_pUi->saveToFileProgressBar->setValue(0);
    m_pUi->saveToFileProgressBar->hide();

    enableUiElementsAfterSavingLogToFile();

    if (!errorDescription.isEmpty()) {
        m_pUi->statusBarLineEdit->setText(errorDescription.localizedString());
        m_pUi->statusBarLineEdit->show();
    }
    else {
        m_pUi->statusBarLineEdit->setText(QString());
        m_pUi->statusBarLineEdit->hide();
    }

#ifdef Q_OS_MAC
    // There are rendering issues on Mac, the progress bar sometimes is not properly hidden,
    // trying to workaround through scheduling the repaint
    repaint();
#endif
}

void LogViewerWidget::onSaveModelEntriesToFileProgress(double progressPercent)
{
    int roundedPercent = static_cast<int>(std::floor(progressPercent + 0.5));
    if (roundedPercent > 100) {
        roundedPercent = 100;
    }

    m_pUi->saveToFileProgressBar->setValue(roundedPercent);
}

void LogViewerWidget::onLogEntriesViewContextMenuRequested(const QPoint & pos)
{
    QModelIndex index = m_pUi->logEntriesTableView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    delete m_pLogEntriesContextMenu;
    m_pLogEntriesContextMenu = new QMenu(this);

    QAction * pCopyAction = new QAction(tr("Copy"), m_pLogEntriesContextMenu);
    pCopyAction->setEnabled(true);
    QObject::connect(pCopyAction, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(LogViewerWidget,onLogEntriesViewCopySelectedItemsAction));
    m_pLogEntriesContextMenu->addAction(pCopyAction);

    QAction * pDeselectAction = new QAction(tr("Clear selection"), m_pLogEntriesContextMenu);
    pDeselectAction->setEnabled(true);
    QObject::connect(pDeselectAction, QNSIGNAL(QAction,triggered),
                     this, QNSLOT(LogViewerWidget,onLogEntriesViewDeselectAction));
    m_pLogEntriesContextMenu->addAction(pDeselectAction);

    m_pLogEntriesContextMenu->show();
    m_pLogEntriesContextMenu->exec(m_pUi->logEntriesTableView->mapToGlobal(pos));
}

void LogViewerWidget::onLogEntriesViewCopySelectedItemsAction()
{
    QString result;
    QTextStream strm(&result);

    QItemSelectionModel * pSelectionModel = m_pUi->logEntriesTableView->selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        return;
    }

    QModelIndexList selectedIndexes = pSelectionModel->selectedIndexes();
    std::set<int> processedRows;

    for(auto it = selectedIndexes.constBegin(),
        end = selectedIndexes.constEnd(); it != end; ++it)
    {
        const QModelIndex & modelIndex = *it;
        if (Q_UNLIKELY(!modelIndex.isValid())) {
            continue;
        }

        int row = modelIndex.row();
        if (processedRows.find(row) != processedRows.end()) {
            continue;
        }

        Q_UNUSED(processedRows.insert(row))

        const LogViewerModel::Data * pDataEntry = m_pLogViewerModel->dataEntry(row);
        if (Q_UNLIKELY(!pDataEntry)) {
            continue;
        }

        strm << m_pLogViewerModel->dataEntryToString(*pDataEntry);
    }

    strm.flush();
    copyStringToClipboard(result);
}

void LogViewerWidget::onLogEntriesViewDeselectAction()
{
    m_pUi->logEntriesTableView->clearSelection();
}

void LogViewerWidget::onWipeLogPushButtonPressed()
{
    QString currentLogFileName = m_pLogViewerModel->logFileName();
    if (Q_UNLIKELY(currentLogFileName.isEmpty())) {
        return;
    }

    QDir dir(QuentierLogFilesDirPath());
    if (Q_UNLIKELY(!dir.exists())) {
        return;
    }

    QFileInfo currentLogFileInfo(dir.absoluteFilePath(currentLogFileName));
    if (Q_UNLIKELY(!currentLogFileInfo.exists() ||
                   !currentLogFileInfo.isFile() ||
                   !currentLogFileInfo.isWritable()))
    {
        Q_UNUSED(warningMessageBox(this, tr("Can't wipe log file"),
                                   tr("Found no log file to wipe"),
                                   tr("Current log file doesn't seem to exist or be a writable file") +
                                   QStringLiteral(": ") + QDir::toNativeSeparators(currentLogFileInfo.absoluteFilePath())))
        return;
    }

    int confirm = questionMessageBox(this, tr("Confirm wiping out the log file"),
                                     tr("Are you sure you want to wipe out the log file?"),
                                     tr("The log file's contents would be removed without "
                                        "the possibility to restore them. Are you sure you want "
                                        "to wipe out the contents of the log file?"));
    if (confirm != QMessageBox::Ok) {
        return;
    }

    ErrorString errorDescription;
    bool res = m_pLogViewerModel->wipeCurrentLogFile(errorDescription);
    if (!res) {
        Q_UNUSED(warningMessageBox(this, tr("Failed to wipe the log file"),
                                   tr("Error wiping out the contents of the log file"),
                                   errorDescription.localizedString()))
    }
}

void LogViewerWidget::clear()
{
    m_pUi->logFileComboBox->clear();
    m_pUi->logFilePendingLoadLabel->clear();
    m_pLogViewerModel->clear();

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
    QHeaderView * pHorizontalHeader = m_pUi->logEntriesTableView->horizontalHeader();
    pHorizontalHeader->resizeSections(QHeaderView::ResizeToContents);

    if (pHorizontalHeader->sectionSize(LogViewerModel::Columns::SourceFileName) > MAX_SOURCE_FILE_NAME_COLUMN_WIDTH) {
        pHorizontalHeader->resizeSection(LogViewerModel::Columns::SourceFileName,
                                         MAX_SOURCE_FILE_NAME_COLUMN_WIDTH);
    }

    m_pUi->logEntriesTableView->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
}

void LogViewerWidget::copyStringToClipboard(const QString & text)
{
    QClipboard * pClipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!pClipboard)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't copy data to clipboard: got null pointer to clipboard from app"));
        QNWARNING(errorDescription);
        m_pUi->statusBarLineEdit->setText(errorDescription.localizedString());
        m_pUi->statusBarLineEdit->show();
        return;
    }

    pClipboard->setText(text);
}

void LogViewerWidget::showLogFileIsLoadingLabel()
{
    m_pUi->logFilePendingLoadLabel->setText(tr("Loading, please wait") + QStringLiteral("..."));
}

void LogViewerWidget::collectModelFilteringOptions(LogViewerModel::FilteringOptions & options) const
{
    options.clear();

    options.m_logEntryContentFilter = m_pUi->filterByContentLineEdit->text();

    for(int i = 0; i < 6; ++i)
    {
        QCheckBox * pCheckbox = m_logLevelEnabledCheckboxPtrs[i];
        if (Q_UNLIKELY(!pCheckbox)) {
            continue;
        }

        if (pCheckbox->isChecked()) {
            continue;
        }

        options.m_disabledLogLevels << static_cast<LogLevel::type>(i);
    }
}

void LogViewerWidget::enableUiElementsAfterSavingLogToFile()
{
    m_pUi->saveToFilePushButton->setEnabled(true);
    m_pUi->clearPushButton->setEnabled(true);

    if (m_pLogViewerModel->startLogFilePos() > 0) {
        m_pUi->resetPushButton->setEnabled(true);
    }

    m_pUi->tracePushButton->setEnabled(true);
    m_pUi->logFileWipePushButton->setEnabled(true);
    m_pUi->filterByContentLineEdit->setEnabled(true);
    m_pUi->filterByLogLevelTableWidget->setEnabled(true);

    m_pUi->logFileComboBox->setEnabled(true);
    m_pUi->logLevelComboBox->setEnabled(true);
}

void LogViewerWidget::timerEvent(QTimerEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return;
    }

    if (pEvent->timerId() == m_delayedSectionResizeTimer.timerId()) {
        resizeLogEntriesViewColumns();
        m_delayedSectionResizeTimer.stop();
    }
}

void LogViewerWidget::closeEvent(QCloseEvent * pEvent)
{
    if (m_pUi->tracePushButton->isChecked()) {
        // Restore the previously backed up log level
        QuentierSetMinLogLevel(m_minLogLevelBeforeTracing);
    }

    QWidget::closeEvent(pEvent);
}

} // namespace quentier
