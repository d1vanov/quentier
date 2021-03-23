/*
 * Copyright 2017-2021 Dmitry Ivanov
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

#include <lib/delegate/LogViewerDelegate.h>
#include <lib/preferences/keys/Logging.h>

#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/StandardPaths.h>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QColor>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenu>
#include <QRegularExpressionValidator>
#include <QStringListModel>
#include <QTextStream>

#include <array>
#include <cmath>
#include <set>

#define QUENTIER_NUM_LOG_LEVELS           (5)
#define FETCHING_MORE_TIMER_PERIOD        (200)
#define DELAY_SECTION_RESIZE_TIMER_PERIOD (100)

namespace quentier {

LogViewerWidget::LogViewerWidget(QWidget * parent) :
    QWidget(parent, Qt::Window), m_pUi(new Ui::LogViewerWidget),
    m_pLogViewerModel(new LogViewerModel(this))
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

    for (size_t i = 0; i < m_filterByLogLevelBeforeTracing.size(); ++i) {
        m_filterByLogLevelBeforeTracing[i] = true;
        m_logLevelEnabledCheckboxPtrs[i] = nullptr;
    }

    setupLogLevels();
    setupLogFiles();
    setupFilterByLogLevelWidget();
    setupFilterByComponent();
    startWatchingForLogFilesFolderChanges();

    m_pUi->logEntriesTableView->setModel(m_pLogViewerModel);

    auto * pLogViewerDelegate =
        new LogViewerDelegate(m_pUi->logEntriesTableView);

    m_pUi->logEntriesTableView->setItemDelegate(pLogViewerDelegate);

    QObject::connect(
        m_pUi->saveToFilePushButton, &QPushButton::clicked, this,
        &LogViewerWidget::onSaveLogToFileButtonPressed);

    QObject::connect(
        m_pUi->clearPushButton, &QPushButton::clicked, this,
        &LogViewerWidget::onClearButtonPressed);

    QObject::connect(
        m_pUi->resetPushButton, &QPushButton::clicked, this,
        &LogViewerWidget::onResetButtonPressed);

    QObject::connect(
        m_pUi->tracePushButton, &QPushButton::toggled, this,
        &LogViewerWidget::onTraceButtonToggled);

    QObject::connect(
        m_pUi->logFileWipePushButton, &QPushButton::clicked, this,
        &LogViewerWidget::onWipeLogPushButtonPressed);

    QObject::connect(
        m_pUi->filterByContentLineEdit, &QLineEdit::editingFinished, this,
        &LogViewerWidget::onFilterByContentEditingFinished);

    QObject::connect(
        m_pLogViewerModel, &LogViewerModel::notifyError, this,
        &LogViewerWidget::onModelError);

    QObject::connect(
        m_pLogViewerModel, &LogViewerModel::rowsInserted, this,
        &LogViewerWidget::onModelRowsInserted);

    QObject::connect(
        m_pLogViewerModel, &LogViewerModel::notifyEndOfLogFileReached, this,
        &LogViewerWidget::onModelEndOfLogFileReached);

    QObject::connect(
        m_pUi->logEntriesTableView, &QTableView::customContextMenuRequested,
        this, &LogViewerWidget::onLogEntriesViewContextMenuRequested);
}

LogViewerWidget::~LogViewerWidget()
{
    delete m_pUi;
}

void LogViewerWidget::setupLogLevels()
{
    m_pUi->logLevelComboBox->addItem(
        LogViewerModel::logLevelToString(LogLevel::Trace));

    m_pUi->logLevelComboBox->addItem(
        LogViewerModel::logLevelToString(LogLevel::Debug));

    m_pUi->logLevelComboBox->addItem(
        LogViewerModel::logLevelToString(LogLevel::Info));

    m_pUi->logLevelComboBox->addItem(
        LogViewerModel::logLevelToString(LogLevel::Warning));

    m_pUi->logLevelComboBox->addItem(
        LogViewerModel::logLevelToString(LogLevel::Error));

    m_pUi->logLevelComboBox->setCurrentIndex(
        static_cast<int>(QuentierMinLogLevel()));

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->logLevelComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &LogViewerWidget::onCurrentLogLevelChanged, Qt::UniqueConnection);
#else
    QObject::connect(
        m_pUi->logLevelComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onCurrentLogLevelChanged(int)), Qt::UniqueConnection);
#endif
}

void LogViewerWidget::setupLogFiles()
{
    const QDir dir{QuentierLogFilesDirPath()};
    if (Q_UNLIKELY(!dir.exists())) {
        clear();
        return;
    }

    const QFileInfoList entries = dir.entryInfoList(QDir::Files, QDir::Name);
    if (entries.isEmpty()) {
        clear();
        return;
    }

    const QString originalLogFileName = m_pUi->logFileComboBox->currentText();
    QString currentLogFileName = originalLogFileName;
    if (currentLogFileName.isEmpty()) {
        currentLogFileName = QStringLiteral("Quentier-log.txt");
    }

    const int numCurrentLogFileComboBoxItems = m_pUi->logFileComboBox->count();
    const int numEntries = entries.size();
    if (numCurrentLogFileComboBoxItems == numEntries) {
        // as the number of entries didn't change, assuming there's no need
        // to change anything
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::disconnect(
        m_pUi->logFileComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
        this, &LogViewerWidget::onCurrentLogFileChanged);
#else
    QObject::disconnect(
        m_pUi->logFileComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onCurrentLogFileChanged(int)));
#endif

    m_pUi->logFileComboBox->clear();

    int currentLogFileIndex = -1;
    for (int i = 0, size = entries.size(); i < size; ++i) {
        const auto & entry = entries.at(i);
        const QString fileName = entry.fileName();

        if (fileName == currentLogFileName) {
            currentLogFileIndex = i;
        }

        m_pUi->logFileComboBox->addItem(fileName);
    }

    if (currentLogFileIndex < 0) {
        currentLogFileIndex = 0;
    }

    m_pUi->logFileComboBox->setCurrentIndex(currentLogFileIndex);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->logFileComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
        this, &LogViewerWidget::onCurrentLogFileChanged, Qt::UniqueConnection);
#else
    QObject::connect(
        m_pUi->logFileComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onCurrentLogFileChanged(int)), Qt::UniqueConnection);
#endif

    const QString logFileName = entries.at(currentLogFileIndex).fileName();
    if (logFileName == originalLogFileName) {
        // The current log file didn't change, no need to set it to the model
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

    QObject::connect(
        &m_logFilesFolderWatcher, &FileSystemWatcher::directoryRemoved, this,
        &LogViewerWidget::onLogFileDirRemoved);

    QObject::connect(
        &m_logFilesFolderWatcher, &FileSystemWatcher::directoryChanged, this,
        &LogViewerWidget::onLogFileDirChanged);
}

void LogViewerWidget::setupFilterByLogLevelWidget()
{
    m_pUi->filterByLogLevelTableWidget->setRowCount(QUENTIER_NUM_LOG_LEVELS);
    m_pUi->filterByLogLevelTableWidget->setColumnCount(2);

    const auto & disabledLogLevels = m_pLogViewerModel->disabledLogLevels();
    for (size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i) {
        auto * pItem = new QTableWidgetItem(
            LogViewerModel::logLevelToString(static_cast<LogLevel>(i)));

        pItem->setData(
            Qt::BackgroundRole,
            m_pLogViewerModel->backgroundColorForLogLevel(
                static_cast<LogLevel>(i)));

        m_pUi->filterByLogLevelTableWidget->setItem(
            static_cast<int>(i), 0, pItem);

        auto * pWidget = new QWidget;

        auto * pCheckbox = new QCheckBox;
        pCheckbox->setObjectName(QString::number(i));
        pCheckbox->setChecked(
            !disabledLogLevels.contains(static_cast<LogLevel>(i)));

        auto * pLayout = new QHBoxLayout(pWidget);
        pLayout->addWidget(pCheckbox);
        pLayout->setAlignment(Qt::AlignCenter);
        pLayout->setContentsMargins(0, 0, 0, 0);

        pWidget->setLayout(pLayout);

        m_pUi->filterByLogLevelTableWidget->setCellWidget(
            static_cast<int>(i), 1, pWidget);

        m_logLevelEnabledCheckboxPtrs[i] = pCheckbox;

        QObject::connect(
            pCheckbox, &QCheckBox::stateChanged, this,
            &LogViewerWidget::onFilterByLogLevelCheckboxToggled);
    }
}

void LogViewerWidget::setupFilterByComponent()
{
    QNDEBUG("widget:log_viewer", "LogViewerWidget::setupFilterByComponent");

    QStringList presets;
    presets.reserve(5);
    presets << QStringLiteral("custom");
    presets << QStringLiteral("note_editor");
    presets << QStringLiteral("side_panels");
    presets << QStringLiteral("ui");
    presets << QStringLiteral("synchronization");

    auto * pPresetsModel =
        new QStringListModel(presets, m_pUi->filterByComponentPresetsComboBox);

    m_pUi->filterByComponentPresetsComboBox->setModel(pPresetsModel);

    m_pUi->filterByComponentRegexLineEdit->setValidator(
        new QRegularExpressionValidator(m_pUi->filterByComponentRegexLineEdit));

    restoreFilterByComponentState();

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->filterByComponentPresetsComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &LogViewerWidget::onFilterByComponentPresetChanged);
#else
    QObject::connect(
        m_pUi->filterByComponentPresetsComboBox,
        SIGNAL(currentIndexChanged(int)), this,
        SLOT(onFilterByComponentPresetChanged(int)));
#endif

    QObject::connect(
        m_pUi->filterByComponentRegexLineEdit, &QLineEdit::editingFinished,
        this, &LogViewerWidget::onFilterByComponentEditingFinished);
}

void LogViewerWidget::onCurrentLogLevelChanged(int index)
{
    QNDEBUG(
        "widget:log_viewer",
        "LogViewerWidget::onCurrentLogLevelChanged: " << index);

    if (Q_UNLIKELY(m_pLogViewerModel->isSavingModelEntriesToFileInProgress())) {
        return;
    }

    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    if (Q_UNLIKELY((index < 0) || (index >= QUENTIER_NUM_LOG_LEVELS))) {
        return;
    }

    QuentierSetMinLogLevel(static_cast<LogLevel>(index));

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::loggingGroup);
    appSettings.setValue(preferences::keys::minLogLevel, index);
    appSettings.endGroup();
}

void LogViewerWidget::onFilterByContentEditingFinished()
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    m_pLogViewerModel->setLogEntryContentFilter(
        m_pUi->filterByContentLineEdit->text());

    showLogFileIsLoadingLabel();
    scheduleLogEntriesViewColumnsResize();
}

void LogViewerWidget::onFilterByLogLevelCheckboxToggled(int state)
{
    QNDEBUG(
        "widget:log_viewer",
        "LogViewerWidget::onFilterByLogLevelCheckboxToggled: state = "
            << state);

    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    auto * pCheckbox = qobject_cast<QCheckBox *>(sender());
    if (Q_UNLIKELY(!pCheckbox)) {
        return;
    }

    bool parsedCheckboxRow = false;
    int checkboxRow = pCheckbox->objectName().toInt(&parsedCheckboxRow);
    if (Q_UNLIKELY(!parsedCheckboxRow)) {
        return;
    }

    if (Q_UNLIKELY(
            (checkboxRow < 0) || (checkboxRow >= QUENTIER_NUM_LOG_LEVELS))) {
        return;
    }

    auto disabledLogLevels = m_pLogViewerModel->disabledLogLevels();

    const int rowIndex =
        disabledLogLevels.indexOf(static_cast<LogLevel>(checkboxRow));

    const bool currentRowWasDisabled = (rowIndex >= 0);
    if (currentRowWasDisabled && (state == Qt::Checked)) {
        disabledLogLevels.remove(rowIndex);
    }
    else if (!currentRowWasDisabled && (state == Qt::Unchecked)) {
        disabledLogLevels << static_cast<LogLevel>(checkboxRow);
    }
    else {
        return;
    }

    m_pLogViewerModel->setDisabledLogLevels(disabledLogLevels);

    showLogFileIsLoadingLabel();
    scheduleLogEntriesViewColumnsResize();
}

void LogViewerWidget::onFilterByComponentPresetChanged(int index)
{
    QNDEBUG(
        "widget:log_viewer",
        "LogViewerWidget::onFilterByComponentPresetChanged: index = "
            << index << ", text: "
            << m_pUi->filterByComponentPresetsComboBox->itemText(index));

    switch (index) {
    case 1:
        // "note_editor"
        m_pUi->filterByComponentRegexLineEdit->setText(
            QStringLiteral("^(enml|note_editor|widget:note_editor.*)(:.*)?$"));
        break;
    case 2:
        // "side panels"
        m_pUi->filterByComponentRegexLineEdit->setText(
            QStringLiteral("^(model|view|delegate)(:.*)?$"));
        break;
    case 3:
        // "ui"
        m_pUi->filterByComponentRegexLineEdit->setText(
            QStringLiteral("^(model|view|delegate|widget|note_editor)(:.*)?$"));
        break;
    case 4:
        // synchronization
        m_pUi->filterByComponentRegexLineEdit->setText(QStringLiteral(
            "^(local_storage|synchronization|types|utility)(:.*)?$"));
        break;
    default:
        // "custom" or unknown
        m_pUi->filterByComponentRegexLineEdit->clear();
        break;
    }

    m_pUi->filterByComponentRegexLineEdit->setReadOnly(index != 0);

    onFilterByComponentEditingFinished();
}

void LogViewerWidget::onFilterByComponentEditingFinished()
{
    saveFilterByComponentState();

    QRegularExpression regex(m_pUi->filterByComponentRegexLineEdit->text());
    QuentierSetLogComponentFilter(regex);
}

void LogViewerWidget::onCurrentLogFileChanged(int currentLogFileIndex)
{
    const QString currentLogFile =
        m_pUi->logFileComboBox->itemText(currentLogFileIndex);

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

    const QString absoluteFilePath = QFileDialog::getSaveFileName(
        this, tr("Save as") + QStringLiteral("..."), documentsPath());

    const QFileInfo fileInfo{absoluteFilePath};
    if (fileInfo.exists()) {
        if (Q_UNLIKELY(!fileInfo.isFile())) {
            ErrorString errorDescription(
                QT_TR_NOOP("Can't save the log to file: the selected entity "
                           "is not a file"));

            m_pUi->statusBarLineEdit->setText(
                errorDescription.localizedString());

            m_pUi->statusBarLineEdit->show();
            Q_UNUSED(errorDescription)
            return;
        }
    }
    else {
        QDir fileDir{fileInfo.absoluteDir()};
        if (!fileDir.exists() && !fileDir.mkpath(fileDir.absolutePath())) {
            ErrorString errorDescription(
                QT_TR_NOOP("Failed to create the folder to contain the file "
                           "with saved logs"));

            m_pUi->statusBarLineEdit->setText(
                errorDescription.localizedString());

            m_pUi->statusBarLineEdit->show();
            Q_UNUSED(errorDescription)
            return;
        }
    }

    m_pUi->saveToFileLabel->setText(
        tr("Saving the log to file") + QStringLiteral("..."));

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

    QObject::connect(
        m_pUi->saveToFileCancelButton, &QPushButton::clicked, this,
        &LogViewerWidget::onCancelSavingTheLogToFileButtonPressed);

    m_pUi->saveToFileCancelButton->show();

    QObject::connect(
        m_pLogViewerModel, &LogViewerModel::saveModelEntriesToFileFinished,
        this, &LogViewerWidget::onSaveModelEntriesToFileFinished);

    QObject::connect(
        m_pLogViewerModel, &LogViewerModel::saveModelEntriesToFileProgress,
        this, &LogViewerWidget::onSaveModelEntriesToFileProgress);

    m_pLogViewerModel->saveModelEntriesToFile(fileInfo.absoluteFilePath());
}

void LogViewerWidget::onCancelSavingTheLogToFileButtonPressed()
{
    m_pUi->statusBarLineEdit->clear();
    m_pUi->statusBarLineEdit->hide();

    m_pLogViewerModel->cancelSavingModelEntriesToFile();

    m_pUi->saveToFileLabel->setText(QString());

    QObject::disconnect(
        m_pUi->saveToFileCancelButton, &QPushButton::clicked, this,
        &LogViewerWidget::onCancelSavingTheLogToFileButtonPressed);

    m_pUi->saveToFileCancelButton->hide();

    m_pUi->saveToFileProgressBar->setValue(0);
    m_pUi->saveToFileProgressBar->hide();

    enableUiElementsAfterSavingLogToFile();

#ifdef Q_OS_MAC
    // There are rendering issues on Mac, the progress bar sometimes is not
    // properly hidden, trying to workaround through scheduling the repaint
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

    if (checked) {
        m_pUi->tracePushButton->setText(tr("Stop tracing"));

        m_startLogFilePosBeforeTracing = m_pLogViewerModel->startLogFilePos();

        m_pUi->resetPushButton->setEnabled(false);

        // Backup the settings used before tracing
        m_minLogLevelBeforeTracing = QuentierMinLogLevel();
        m_filterByContentBeforeTracing = m_pUi->filterByContentLineEdit->text();

        auto disabledLogLevels = m_pLogViewerModel->disabledLogLevels();
        for (size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i) {
            m_filterByLogLevelBeforeTracing[i] =
                !disabledLogLevels.contains(static_cast<LogLevel>(i));
        }

        // Enable tracing
        QuentierSetMinLogLevel(LogLevel::Trace);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        QObject::disconnect(
            m_pUi->logLevelComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &LogViewerWidget::onCurrentLogLevelChanged);
#else
        QObject::disconnect(
            m_pUi->logLevelComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onCurrentLogLevelChanged(int)));
#endif

        m_pUi->logLevelComboBox->setCurrentIndex(0);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        QObject::connect(
            m_pUi->logLevelComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &LogViewerWidget::onCurrentLogLevelChanged, Qt::UniqueConnection);
#else
        QObject::connect(
            m_pUi->logLevelComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onCurrentLogLevelChanged(int)), Qt::UniqueConnection);
#endif

        m_pUi->logLevelComboBox->setEnabled(false);
        m_pUi->filterByContentLineEdit->setText(QString());

        for (size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i) {
            auto * pCheckbox = m_logLevelEnabledCheckboxPtrs[i];

            QObject::disconnect(
                pCheckbox, &QCheckBox::stateChanged, this,
                &LogViewerWidget::onFilterByLogLevelCheckboxToggled);

            pCheckbox->setChecked(true);

            QObject::connect(
                pCheckbox, &QCheckBox::stateChanged, this,
                &LogViewerWidget::onFilterByLogLevelCheckboxToggled);
        }

        m_pUi->filterByLogLevelTableWidget->setEnabled(false);
        m_pUi->logFileWipePushButton->setEnabled(false);

        LogViewerModel::FilteringOptions filteringOptions;

        filteringOptions.m_startLogFilePos =
            m_pLogViewerModel->currentLogFileSize();

        m_pLogViewerModel->setLogFileName(
            m_pLogViewerModel->logFileName(), filteringOptions);
    }
    else {
        m_pUi->tracePushButton->setText(tr("Trace"));

        // Restore the previously backed up settings
        QuentierSetMinLogLevel(m_minLogLevelBeforeTracing);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        QObject::disconnect(
            m_pUi->logLevelComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &LogViewerWidget::onCurrentLogLevelChanged);
#else
        QObject::disconnect(
            m_pUi->logLevelComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onCurrentLogLevelChanged(int)));
#endif

        m_pUi->logLevelComboBox->setCurrentIndex(
            static_cast<int>(m_minLogLevelBeforeTracing));

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        QObject::connect(
            m_pUi->logLevelComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &LogViewerWidget::onCurrentLogLevelChanged, Qt::UniqueConnection);
#else
        QObject::connect(
            m_pUi->logLevelComboBox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onCurrentLogLevelChanged(int)), Qt::UniqueConnection);
#endif

        m_pUi->logLevelComboBox->setEnabled(true);
        m_minLogLevelBeforeTracing = LogLevel::Info;

        LogViewerModel::FilteringOptions filteringOptions;

        if (m_startLogFilePosBeforeTracing > 0) {
            filteringOptions.m_startLogFilePos = m_startLogFilePosBeforeTracing;
            m_pUi->resetPushButton->setEnabled(true);
        }
        else {
            m_pUi->resetPushButton->setEnabled(false);
        }
        m_startLogFilePosBeforeTracing = -1;

        filteringOptions.m_logEntryContentFilter =
            m_filterByContentBeforeTracing;

        m_pUi->filterByContentLineEdit->setText(m_filterByContentBeforeTracing);
        m_filterByContentBeforeTracing.clear();

        filteringOptions.m_disabledLogLevels.reserve(QUENTIER_NUM_LOG_LEVELS);
        for (size_t i = 0; i < QUENTIER_NUM_LOG_LEVELS; ++i) {
            if (!m_filterByLogLevelBeforeTracing[i]) {
                filteringOptions.m_disabledLogLevels
                    << static_cast<LogLevel>(i);
            }

            auto * pCheckbox = m_logLevelEnabledCheckboxPtrs[i];

            QObject::disconnect(
                pCheckbox, &QCheckBox::stateChanged, this,
                &LogViewerWidget::onFilterByLogLevelCheckboxToggled);

            m_logLevelEnabledCheckboxPtrs[i]->setChecked(
                m_filterByLogLevelBeforeTracing[i]);

            QObject::connect(
                pCheckbox, &QCheckBox::stateChanged, this,
                &LogViewerWidget::onFilterByLogLevelCheckboxToggled);
        }

        m_pLogViewerModel->setLogFileName(
            m_pLogViewerModel->logFileName(), filteringOptions);

        m_pUi->filterByLogLevelTableWidget->setEnabled(true);
        m_pUi->logFileWipePushButton->setEnabled(true);
    }

    scheduleLogEntriesViewColumnsResize();
}

void LogViewerWidget::onModelError(ErrorString errorDescription) // NOLINT
{
    m_pUi->logFilePendingLoadLabel->setText(QString());
    m_pUi->statusBarLineEdit->setText(errorDescription.localizedString());
    m_pUi->statusBarLineEdit->show();
}

void LogViewerWidget::onModelRowsInserted(
    const QModelIndex & parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    scheduleLogEntriesViewColumnsResize();
    m_pUi->logFilePendingLoadLabel->setText(QString());
}

void LogViewerWidget::onModelEndOfLogFileReached()
{
    m_pUi->logFilePendingLoadLabel->setText(QString());
}

void LogViewerWidget::onSaveModelEntriesToFileFinished(
    ErrorString errorDescription) // NOLINT
{
    QObject::disconnect(
        m_pLogViewerModel, &LogViewerModel::saveModelEntriesToFileFinished,
        this, &LogViewerWidget::onSaveModelEntriesToFileFinished);

    QObject::disconnect(
        m_pLogViewerModel, &LogViewerModel::saveModelEntriesToFileProgress,
        this, &LogViewerWidget::onSaveModelEntriesToFileProgress);

    m_pUi->saveToFileLabel->setText(QString());

    QObject::disconnect(
        m_pUi->saveToFileCancelButton, &QPushButton::clicked, this,
        &LogViewerWidget::onCancelSavingTheLogToFileButtonPressed);

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
    // There are rendering issues on Mac, the progress bar sometimes is not
    // properly hidden, trying to workaround through scheduling the repaint
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
    const auto index = m_pUi->logEntriesTableView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    delete m_pLogEntriesContextMenu;
    m_pLogEntriesContextMenu = new QMenu(this);

    auto * pCopyAction = new QAction(tr("Copy"), m_pLogEntriesContextMenu);
    pCopyAction->setEnabled(true);

    QObject::connect(
        pCopyAction, &QAction::triggered, this,
        &LogViewerWidget::onLogEntriesViewCopySelectedItemsAction);

    m_pLogEntriesContextMenu->addAction(pCopyAction);

    auto * pDeselectAction =
        new QAction(tr("Clear selection"), m_pLogEntriesContextMenu);

    pDeselectAction->setEnabled(true);

    QObject::connect(
        pDeselectAction, &QAction::triggered, this,
        &LogViewerWidget::onLogEntriesViewDeselectAction);

    m_pLogEntriesContextMenu->addAction(pDeselectAction);
    m_pLogEntriesContextMenu->show();

    m_pLogEntriesContextMenu->exec(
        m_pUi->logEntriesTableView->mapToGlobal(pos));
}

void LogViewerWidget::onLogEntriesViewCopySelectedItemsAction()
{
    QString result;
    QTextStream strm(&result);

    auto * pSelectionModel = m_pUi->logEntriesTableView->selectionModel();
    if (Q_UNLIKELY(!pSelectionModel)) {
        return;
    }

    const auto selectedIndexes = pSelectionModel->selectedIndexes();
    std::set<int> processedRows;

    for (const auto & modelIndex: qAsConst(selectedIndexes)) {
        if (Q_UNLIKELY(!modelIndex.isValid())) {
            continue;
        }

        int row = modelIndex.row();
        if (processedRows.find(row) != processedRows.end()) {
            continue;
        }

        Q_UNUSED(processedRows.insert(row))

        const LogViewerModel::Data * pDataEntry =
            m_pLogViewerModel->dataEntry(row);
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
    const QString currentLogFileName = m_pLogViewerModel->logFileName();
    if (Q_UNLIKELY(currentLogFileName.isEmpty())) {
        return;
    }

    const QDir dir{QuentierLogFilesDirPath()};
    if (Q_UNLIKELY(!dir.exists())) {
        return;
    }

    const QFileInfo currentLogFileInfo{
        dir.absoluteFilePath(currentLogFileName)};

    if (Q_UNLIKELY(
            !currentLogFileInfo.exists() || !currentLogFileInfo.isFile() ||
            !currentLogFileInfo.isWritable()))
    {
        Q_UNUSED(warningMessageBox(
            this, tr("Can't wipe log file"), tr("Found no log file to wipe"),
            tr("Current log file doesn't seem to exist or be a writable file") +
                QStringLiteral(": ") +
                QDir::toNativeSeparators(
                    currentLogFileInfo.absoluteFilePath())))

        return;
    }

    const int confirm = questionMessageBox(
        this, tr("Confirm wiping out the log file"),
        tr("Are you sure you want to wipe out the log file?"),
        tr("The log file's contents would be removed without "
           "the possibility to restore them. Are you sure you want "
           "to wipe out the contents of the log file?"));

    if (confirm != QMessageBox::Ok) {
        return;
    }

    ErrorString errorDescription;
    if (!m_pLogViewerModel->wipeCurrentLogFile(errorDescription)) {
        Q_UNUSED(warningMessageBox(
            this, tr("Failed to wipe the log file"),
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
    auto * pHorizontalHeader = m_pUi->logEntriesTableView->horizontalHeader();
    pHorizontalHeader->resizeSections(QHeaderView::ResizeToContents);

    std::array<LogViewerModel::Column, 2> columns{
        LogViewerModel::Column::SourceFileName,
        LogViewerModel::Column::Component};

    for (const auto column: columns) {
        if (pHorizontalHeader->sectionSize(static_cast<int>(column)) >
            MAX_SOURCE_FILE_NAME_COLUMN_WIDTH)
        {
            pHorizontalHeader->resizeSection(
                static_cast<int>(column), MAX_SOURCE_FILE_NAME_COLUMN_WIDTH);
        }
    }

    m_pUi->logEntriesTableView->verticalHeader()->resizeSections(
        QHeaderView::ResizeToContents);
}

void LogViewerWidget::copyStringToClipboard(const QString & text)
{
    auto * pClipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!pClipboard)) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't copy data to clipboard: got null pointer to "
                       "clipboard from app"));

        QNWARNING("widget:log_viewer", errorDescription);
        m_pUi->statusBarLineEdit->setText(errorDescription.localizedString());
        m_pUi->statusBarLineEdit->show();
        return;
    }

    pClipboard->setText(text);
}

void LogViewerWidget::showLogFileIsLoadingLabel()
{
    m_pUi->logFilePendingLoadLabel->setText(
        tr("Loading, please wait") + QStringLiteral("..."));
}

void LogViewerWidget::collectModelFilteringOptions(
    LogViewerModel::FilteringOptions & options) const
{
    options.clear();
    options.m_logEntryContentFilter = m_pUi->filterByContentLineEdit->text();

    for (std::size_t i = 0; i < m_logLevelEnabledCheckboxPtrs.size(); ++i) {
        auto * pCheckbox = m_logLevelEnabledCheckboxPtrs[i];
        if (Q_UNLIKELY(!pCheckbox)) {
            continue;
        }

        if (pCheckbox->isChecked()) {
            continue;
        }

        options.m_disabledLogLevels << static_cast<LogLevel>(i);
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

void LogViewerWidget::saveFilterByComponentState()
{
    QNDEBUG("widget:log_viewer", "LogViewerWidget::saveFilterByComponentState");

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::loggingGroup);

    appSettings.setValue(
        preferences::keys::loggingFilterByComponentPreset,
        m_pUi->filterByComponentPresetsComboBox->currentIndex());

    appSettings.setValue(
        preferences::keys::loggingFilterByComponentRegex,
        m_pUi->filterByComponentRegexLineEdit->text());

    appSettings.endGroup();
}

void LogViewerWidget::restoreFilterByComponentState()
{
    QNDEBUG(
        "widget:log_viewer", "LogViewerWidget::restoreFilterByComponentState");

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::loggingGroup);

    const auto presetIndex =
        appSettings.value(preferences::keys::loggingFilterByComponentPreset);

    const auto filter =
        appSettings.value(preferences::keys::loggingFilterByComponentRegex)
            .toString();

    appSettings.endGroup();

    bool conversionResult = false;
    int presetIndexInt = presetIndex.toInt(&conversionResult);
    if (!conversionResult) {
        presetIndexInt = 0;
    }

    m_pUi->filterByComponentPresetsComboBox->setCurrentIndex(presetIndexInt);

    m_pUi->filterByComponentRegexLineEdit->setText(filter);
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
