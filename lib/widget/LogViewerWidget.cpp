/*
 * Copyright 2017-2024 Dmitry Ivanov
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
#include <quentier/utility/Compat.h>
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

#include <cmath>
#include <cstddef>
#include <set>
#include <utility>

namespace quentier {

namespace {

constexpr std::size_t gQuentierNumLogLevels = 5;

} // namespace

LogViewerWidget::LogViewerWidget(QWidget * parent) :
    QWidget{parent, Qt::Window}, m_ui{new Ui::LogViewerWidget},
    m_logViewerModel{new LogViewerModel{this}}
{
    Q_ASSERT(m_logLevelEnabledCheckboxPtrs.size() == gQuentierNumLogLevels);
    Q_ASSERT(m_filterByLogLevelBeforeTracing.size() == gQuentierNumLogLevels);

    m_ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->resetPushButton->setEnabled(false);
    m_ui->tracePushButton->setCheckable(true);

    m_ui->saveToFileProgressBar->hide();
    m_ui->saveToFileCancelButton->hide();

    m_ui->statusBarLineEdit->hide();

    m_ui->logEntriesTableView->verticalHeader()->hide();
    m_ui->logEntriesTableView->setWordWrap(true);

    m_ui->filterByLogLevelTableWidget->horizontalHeader()->hide();
    m_ui->filterByLogLevelTableWidget->verticalHeader()->hide();

    for (std::size_t i = 0; i < gQuentierNumLogLevels; ++i) {
        m_filterByLogLevelBeforeTracing[i] = true;
        m_logLevelEnabledCheckboxPtrs[i] = nullptr;
    }

    setupLogLevels();
    setupLogFiles();
    setupFilterByLogLevelWidget();
    setupFilterByComponent();
    startWatchingForLogFilesFolderChanges();

    m_ui->logEntriesTableView->setModel(m_logViewerModel);

    auto * logViewerDelegate = new LogViewerDelegate(m_ui->logEntriesTableView);
    m_ui->logEntriesTableView->setItemDelegate(logViewerDelegate);

    QObject::connect(
        m_ui->saveToFilePushButton, &QPushButton::clicked, this,
        &LogViewerWidget::onSaveLogToFileButtonPressed);

    QObject::connect(
        m_ui->clearPushButton, &QPushButton::clicked, this,
        &LogViewerWidget::onClearButtonPressed);

    QObject::connect(
        m_ui->resetPushButton, &QPushButton::clicked, this,
        &LogViewerWidget::onResetButtonPressed);

    QObject::connect(
        m_ui->tracePushButton, &QPushButton::toggled, this,
        &LogViewerWidget::onTraceButtonToggled);

    QObject::connect(
        m_ui->logFileWipePushButton, &QPushButton::clicked, this,
        &LogViewerWidget::onWipeLogPushButtonPressed);

    QObject::connect(
        m_ui->filterByContentLineEdit, &QLineEdit::editingFinished, this,
        &LogViewerWidget::onFilterByContentEditingFinished);

    QObject::connect(
        m_logViewerModel, &LogViewerModel::notifyError, this,
        &LogViewerWidget::onModelError);

    QObject::connect(
        m_logViewerModel, &LogViewerModel::rowsInserted, this,
        &LogViewerWidget::onModelRowsInserted);

    QObject::connect(
        m_logViewerModel, &LogViewerModel::notifyEndOfLogFileReached, this,
        &LogViewerWidget::onModelEndOfLogFileReached);

    QObject::connect(
        m_ui->logEntriesTableView, &QTableView::customContextMenuRequested,
        this, &LogViewerWidget::onLogEntriesViewContextMenuRequested);
}

LogViewerWidget::~LogViewerWidget()
{
    delete m_ui;
}

void LogViewerWidget::setupLogLevels()
{
    m_ui->logLevelComboBox->addItem(
        LogViewerModel::logLevelToString(LogLevel::Trace));

    m_ui->logLevelComboBox->addItem(
        LogViewerModel::logLevelToString(LogLevel::Debug));

    m_ui->logLevelComboBox->addItem(
        LogViewerModel::logLevelToString(LogLevel::Info));

    m_ui->logLevelComboBox->addItem(
        LogViewerModel::logLevelToString(LogLevel::Warning));

    m_ui->logLevelComboBox->addItem(
        LogViewerModel::logLevelToString(LogLevel::Error));

    m_ui->logLevelComboBox->setCurrentIndex(
        static_cast<int>(QuentierMinLogLevel()));

    QObject::connect(
        m_ui->logLevelComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &LogViewerWidget::onCurrentLogLevelChanged, Qt::UniqueConnection);
}

void LogViewerWidget::setupLogFiles()
{
    QDir dir{QuentierLogFilesDirPath()};
    if (Q_UNLIKELY(!dir.exists())) {
        clear();
        return;
    }

    const QFileInfoList entries = dir.entryInfoList(QDir::Files, QDir::Name);
    if (entries.isEmpty()) {
        clear();
        return;
    }

    const QString originalLogFileName = m_ui->logFileComboBox->currentText();
    QString currentLogFileName = originalLogFileName;
    if (currentLogFileName.isEmpty()) {
        currentLogFileName = QStringLiteral("Quentier-log.txt");
    }

    const int numCurrentLogFileComboBoxItems = m_ui->logFileComboBox->count();
    const int numEntries = entries.size();
    if (numCurrentLogFileComboBoxItems == numEntries) {
        // as the number of entries didn't change, assuming there's no need
        // to change anything
        return;
    }

    QObject::disconnect(
        m_ui->logFileComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
        this, &LogViewerWidget::onCurrentLogFileChanged);

    m_ui->logFileComboBox->clear();

    int currentLogFileIndex = -1;
    for (int i = 0, size = entries.size(); i < size; ++i) {
        const auto & entry = entries.at(i);
        const QString fileName = entry.fileName();

        if (fileName == currentLogFileName) {
            currentLogFileIndex = i;
        }

        m_ui->logFileComboBox->addItem(fileName);
    }

    if (currentLogFileIndex < 0) {
        currentLogFileIndex = 0;
    }

    m_ui->logFileComboBox->setCurrentIndex(currentLogFileIndex);

    QObject::connect(
        m_ui->logFileComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
        this, &LogViewerWidget::onCurrentLogFileChanged, Qt::UniqueConnection);

    const QString logFileName = entries.at(currentLogFileIndex).fileName();
    if (logFileName == originalLogFileName) {
        // The current log file didn't change, no need to set it to the model
        return;
    }

    LogViewerModel::FilteringOptions filteringOptions;
    collectModelFilteringOptions(filteringOptions);
    m_logViewerModel->setLogFileName(logFileName, filteringOptions);

    if (m_logViewerModel->currentLogFileSize() != 0) {
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
    m_ui->filterByLogLevelTableWidget->setRowCount(gQuentierNumLogLevels);
    m_ui->filterByLogLevelTableWidget->setColumnCount(2);

    const auto & disabledLogLevels = m_logViewerModel->disabledLogLevels();
    for (std::size_t i = 0; i < gQuentierNumLogLevels; ++i) {
        auto * item = new QTableWidgetItem(
            LogViewerModel::logLevelToString(static_cast<LogLevel>(i)));

        item->setData(
            Qt::BackgroundRole,
            m_logViewerModel->backgroundColorForLogLevel(
                static_cast<LogLevel>(i)));

        m_ui->filterByLogLevelTableWidget->setItem(
            static_cast<int>(i), 0, item);

        auto * widget = new QWidget;

        auto * checkbox = new QCheckBox;
        checkbox->setObjectName(QString::number(i));
        checkbox->setChecked(
            !disabledLogLevels.contains(static_cast<LogLevel>(i)));

        auto * layout = new QHBoxLayout(widget);
        layout->addWidget(checkbox);
        layout->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(0, 0, 0, 0);

        widget->setLayout(layout);

        m_ui->filterByLogLevelTableWidget->setCellWidget(
            static_cast<int>(i), 1, widget);

        m_logLevelEnabledCheckboxPtrs[i] = checkbox;

        QObject::connect(
            checkbox, &QCheckBox::stateChanged, this,
            &LogViewerWidget::onFilterByLogLevelCheckboxToggled);
    }
}

void LogViewerWidget::setupFilterByComponent()
{
    QNDEBUG(
        "widget::LogViewerWidget", "LogViewerWidget::setupFilterByComponent");

    QStringList presets;
    presets.reserve(5);
    presets << QStringLiteral("custom");
    presets << QStringLiteral("note_editor");
    presets << QStringLiteral("side_panels");
    presets << QStringLiteral("ui");
    presets << QStringLiteral("synchronization");

    auto * presetsModel =
        new QStringListModel(presets, m_ui->filterByComponentPresetsComboBox);

    m_ui->filterByComponentPresetsComboBox->setModel(presetsModel);

    m_ui->filterByComponentRegexLineEdit->setValidator(
        new QRegularExpressionValidator(m_ui->filterByComponentRegexLineEdit));

    restoreFilterByComponentState();

    QObject::connect(
        m_ui->filterByComponentPresetsComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &LogViewerWidget::onFilterByComponentPresetChanged);

    QObject::connect(
        m_ui->filterByComponentRegexLineEdit, &QLineEdit::editingFinished,
        this, &LogViewerWidget::onFilterByComponentEditingFinished);
}

void LogViewerWidget::onCurrentLogLevelChanged(int index)
{
    QNDEBUG(
        "widget::LogViewerWidget",
        "LogViewerWidget::onCurrentLogLevelChanged: " << index);

    if (Q_UNLIKELY(m_logViewerModel->isSavingModelEntriesToFileInProgress())) {
        return;
    }

    m_ui->statusBarLineEdit->clear();
    m_ui->statusBarLineEdit->hide();

    if (Q_UNLIKELY((index < 0) || (index >= gQuentierNumLogLevels))) {
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
    m_ui->statusBarLineEdit->clear();
    m_ui->statusBarLineEdit->hide();

    m_logViewerModel->setLogEntryContentFilter(
        m_ui->filterByContentLineEdit->text());

    showLogFileIsLoadingLabel();
    scheduleLogEntriesViewColumnsResize();
}

void LogViewerWidget::onFilterByLogLevelCheckboxToggled(int state)
{
    QNDEBUG(
        "widget::LogViewerWidget",
        "LogViewerWidget::onFilterByLogLevelCheckboxToggled: state = "
            << state);

    m_ui->statusBarLineEdit->clear();
    m_ui->statusBarLineEdit->hide();

    auto * checkbox = qobject_cast<QCheckBox *>(sender());
    if (Q_UNLIKELY(!checkbox)) {
        return;
    }

    bool parsedCheckboxRow = false;
    const int checkboxRow = checkbox->objectName().toInt(&parsedCheckboxRow);
    if (Q_UNLIKELY(!parsedCheckboxRow)) {
        return;
    }

    if (Q_UNLIKELY(
            (checkboxRow < 0) || (checkboxRow >= gQuentierNumLogLevels))) {
        return;
    }

    auto disabledLogLevels = m_logViewerModel->disabledLogLevels();

    const int rowIndex =
        disabledLogLevels.indexOf(static_cast<LogLevel>(checkboxRow));

    bool currentRowWasDisabled = (rowIndex >= 0);
    if (currentRowWasDisabled && (state == Qt::Checked)) {
        disabledLogLevels.removeAt(rowIndex);
    }
    else if (!currentRowWasDisabled && (state == Qt::Unchecked)) {
        disabledLogLevels << static_cast<LogLevel>(checkboxRow);
    }
    else {
        return;
    }

    m_logViewerModel->setDisabledLogLevels(disabledLogLevels);

    showLogFileIsLoadingLabel();
    scheduleLogEntriesViewColumnsResize();
}

void LogViewerWidget::onFilterByComponentPresetChanged(int index)
{
    QNDEBUG(
        "widget::LogViewerWidget",
        "LogViewerWidget::onFilterByComponentPresetChanged: index = "
            << index << ", text: "
            << m_ui->filterByComponentPresetsComboBox->itemText(index));

    switch (index) {
    case 1:
        // "note_editor"
        m_ui->filterByComponentRegexLineEdit->setText(
            QStringLiteral("^(enml|note_editor|widget:note_editor.*)(:.*)?$"));
        break;
    case 2:
        // "side panels"
        m_ui->filterByComponentRegexLineEdit->setText(
            QStringLiteral("^(model|view|delegate)(:.*)?$"));
        break;
    case 3:
        // "ui"
        m_ui->filterByComponentRegexLineEdit->setText(
            QStringLiteral("^(model|view|delegate|widget|note_editor)(:.*)?$"));
        break;
    case 4:
        // synchronization
        m_ui->filterByComponentRegexLineEdit->setText(QStringLiteral(
            "^(local_storage|synchronization|types|utility)(:.*)?$"));
        break;
    default:
        // "custom" or unknown
        m_ui->filterByComponentRegexLineEdit->clear();
        break;
    }

    m_ui->filterByComponentRegexLineEdit->setReadOnly(index != 0);

    onFilterByComponentEditingFinished();
}

void LogViewerWidget::onFilterByComponentEditingFinished()
{
    saveFilterByComponentState();

    QRegularExpression regex{m_ui->filterByComponentRegexLineEdit->text()};
    QuentierSetLogComponentFilter(regex);
}

void LogViewerWidget::onCurrentLogFileChanged(int currentLogFileIndex)
{
    QString currentLogFile =
        m_ui->logFileComboBox->itemText(currentLogFileIndex);

    m_ui->statusBarLineEdit->clear();
    m_ui->statusBarLineEdit->hide();

    if (currentLogFile.isEmpty()) {
        return;
    }

    LogViewerModel::FilteringOptions filteringOptions;
    collectModelFilteringOptions(filteringOptions);

    m_logViewerModel->setLogFileName(currentLogFile);

    if (m_logViewerModel->currentLogFileSize() != 0) {
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
    m_ui->statusBarLineEdit->clear();
    m_ui->statusBarLineEdit->hide();

    const QString absoluteFilePath = QFileDialog::getSaveFileName(
        this, tr("Save as") + QStringLiteral("..."), documentsPath());

    const QFileInfo fileInfo{absoluteFilePath};
    if (fileInfo.exists()) {
        if (Q_UNLIKELY(!fileInfo.isFile())) {
            ErrorString errorDescription(
                QT_TR_NOOP("Can't save the log to file: the selected entity "
                           "is not a file"));

            m_ui->statusBarLineEdit->setText(
                errorDescription.localizedString());

            m_ui->statusBarLineEdit->show();
            Q_UNUSED(errorDescription)
            return;
        }
    }
    else {
        QDir fileDir{fileInfo.absoluteDir()};
        if (!fileDir.exists() && !fileDir.mkpath(fileDir.absolutePath())) {
            ErrorString errorDescription{
                QT_TR_NOOP("Failed to create the folder to contain the file "
                           "with saved logs")};

            m_ui->statusBarLineEdit->setText(
                errorDescription.localizedString());

            m_ui->statusBarLineEdit->show();
            Q_UNUSED(errorDescription)
            return;
        }
    }

    m_ui->saveToFileLabel->setText(
        tr("Saving the log to file") + QStringLiteral("..."));

    m_ui->saveToFileProgressBar->setMinimum(0);
    m_ui->saveToFileProgressBar->setMaximum(100);
    m_ui->saveToFileProgressBar->setValue(0);
    m_ui->saveToFileProgressBar->show();

    // Disable UI elements controlling the selected file, filtering etc
    // in order to prevent screwing up the process of saving stuff to file
    m_ui->saveToFilePushButton->setEnabled(false);
    m_ui->clearPushButton->setEnabled(false);
    m_ui->resetPushButton->setEnabled(false);
    m_ui->tracePushButton->setEnabled(false);
    m_ui->logFileWipePushButton->setEnabled(false);
    m_ui->filterByContentLineEdit->setEnabled(false);
    m_ui->filterByLogLevelTableWidget->setEnabled(false);
    m_ui->logFileComboBox->setEnabled(false);
    m_ui->logLevelComboBox->setEnabled(false);

    QObject::connect(
        m_ui->saveToFileCancelButton, &QPushButton::clicked, this,
        &LogViewerWidget::onCancelSavingTheLogToFileButtonPressed);

    m_ui->saveToFileCancelButton->show();

    QObject::connect(
        m_logViewerModel, &LogViewerModel::saveModelEntriesToFileFinished,
        this, &LogViewerWidget::onSaveModelEntriesToFileFinished);

    QObject::connect(
        m_logViewerModel, &LogViewerModel::saveModelEntriesToFileProgress,
        this, &LogViewerWidget::onSaveModelEntriesToFileProgress);

    m_logViewerModel->saveModelEntriesToFile(fileInfo.absoluteFilePath());
}

void LogViewerWidget::onCancelSavingTheLogToFileButtonPressed()
{
    m_ui->statusBarLineEdit->clear();
    m_ui->statusBarLineEdit->hide();

    m_logViewerModel->cancelSavingModelEntriesToFile();

    m_ui->saveToFileLabel->setText(QString());

    QObject::disconnect(
        m_ui->saveToFileCancelButton, &QPushButton::clicked, this,
        &LogViewerWidget::onCancelSavingTheLogToFileButtonPressed);

    m_ui->saveToFileCancelButton->hide();

    m_ui->saveToFileProgressBar->setValue(0);
    m_ui->saveToFileProgressBar->hide();

    enableUiElementsAfterSavingLogToFile();

#ifdef Q_OS_MAC
    // There are rendering issues on Mac, the progress bar sometimes is not
    // properly hidden, trying to workaround through scheduling the repaint
    repaint();
#endif
}

void LogViewerWidget::onClearButtonPressed()
{
    if (Q_UNLIKELY(m_logViewerModel->isSavingModelEntriesToFileInProgress())) {
        return;
    }

    m_ui->statusBarLineEdit->clear();
    m_ui->statusBarLineEdit->hide();

    m_logViewerModel->setStartLogFilePosAfterCurrentFileSize();
    m_ui->resetPushButton->setEnabled(true);
}

void LogViewerWidget::onResetButtonPressed()
{
    if (Q_UNLIKELY(m_logViewerModel->isSavingModelEntriesToFileInProgress())) {
        return;
    }

    m_ui->statusBarLineEdit->clear();
    m_ui->statusBarLineEdit->hide();

    m_logViewerModel->setStartLogFilePos(0);
    m_ui->resetPushButton->setEnabled(false);
    showLogFileIsLoadingLabel();
}

void LogViewerWidget::onTraceButtonToggled(const bool checked)
{
    if (Q_UNLIKELY(m_logViewerModel->isSavingModelEntriesToFileInProgress())) {
        return;
    }

    m_ui->statusBarLineEdit->clear();
    m_ui->statusBarLineEdit->hide();

    if (checked) {
        m_ui->tracePushButton->setText(tr("Stop tracing"));

        m_startLogFilePosBeforeTracing = m_logViewerModel->startLogFilePos();

        m_ui->resetPushButton->setEnabled(false);

        // Backup the settings used before tracing
        m_minLogLevelBeforeTracing = QuentierMinLogLevel();
        m_filterByContentBeforeTracing = m_ui->filterByContentLineEdit->text();

        auto disabledLogLevels = m_logViewerModel->disabledLogLevels();
        for (std::size_t i = 0; i < gQuentierNumLogLevels; ++i) {
            m_filterByLogLevelBeforeTracing[i] =
                !disabledLogLevels.contains(static_cast<LogLevel>(i));
        }

        // Enable tracing
        QuentierSetMinLogLevel(LogLevel::Trace);

        QObject::disconnect(
            m_ui->logLevelComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &LogViewerWidget::onCurrentLogLevelChanged);

        m_ui->logLevelComboBox->setCurrentIndex(0);

        QObject::connect(
            m_ui->logLevelComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &LogViewerWidget::onCurrentLogLevelChanged, Qt::UniqueConnection);

        m_ui->logLevelComboBox->setEnabled(false);
        m_ui->filterByContentLineEdit->setText(QString());

        for (std::size_t i = 0; i < gQuentierNumLogLevels; ++i) {
            auto * checkbox = m_logLevelEnabledCheckboxPtrs[i];

            QObject::disconnect(
                checkbox, &QCheckBox::stateChanged, this,
                &LogViewerWidget::onFilterByLogLevelCheckboxToggled);

            checkbox->setChecked(true);

            QObject::connect(
                checkbox, &QCheckBox::stateChanged, this,
                &LogViewerWidget::onFilterByLogLevelCheckboxToggled);
        }

        m_ui->filterByLogLevelTableWidget->setEnabled(false);
        m_ui->logFileWipePushButton->setEnabled(false);

        LogViewerModel::FilteringOptions filteringOptions;

        filteringOptions.m_startLogFilePos =
            m_logViewerModel->currentLogFileSize();

        m_logViewerModel->setLogFileName(
            m_logViewerModel->logFileName(), filteringOptions);
    }
    else {
        m_ui->tracePushButton->setText(tr("Trace"));

        // Restore the previously backed up settings
        QuentierSetMinLogLevel(m_minLogLevelBeforeTracing);

        QObject::disconnect(
            m_ui->logLevelComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &LogViewerWidget::onCurrentLogLevelChanged);

        m_ui->logLevelComboBox->setCurrentIndex(
            static_cast<int>(m_minLogLevelBeforeTracing));

        QObject::connect(
            m_ui->logLevelComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            &LogViewerWidget::onCurrentLogLevelChanged, Qt::UniqueConnection);

        m_ui->logLevelComboBox->setEnabled(true);
        m_minLogLevelBeforeTracing = LogLevel::Info;

        LogViewerModel::FilteringOptions filteringOptions;

        if (m_startLogFilePosBeforeTracing > 0) {
            filteringOptions.m_startLogFilePos = m_startLogFilePosBeforeTracing;
            m_ui->resetPushButton->setEnabled(true);
        }
        else {
            m_ui->resetPushButton->setEnabled(false);
        }
        m_startLogFilePosBeforeTracing = -1;

        filteringOptions.m_logEntryContentFilter =
            m_filterByContentBeforeTracing;

        m_ui->filterByContentLineEdit->setText(m_filterByContentBeforeTracing);
        m_filterByContentBeforeTracing.clear();

        filteringOptions.m_disabledLogLevels.reserve(gQuentierNumLogLevels);
        for (std::size_t i = 0; i < gQuentierNumLogLevels; ++i) {
            if (!m_filterByLogLevelBeforeTracing[i]) {
                filteringOptions.m_disabledLogLevels
                    << static_cast<LogLevel>(i);
            }

            auto * checkbox = m_logLevelEnabledCheckboxPtrs[i];

            QObject::disconnect(
                checkbox, &QCheckBox::stateChanged, this,
                &LogViewerWidget::onFilterByLogLevelCheckboxToggled);

            m_logLevelEnabledCheckboxPtrs[i]->setChecked(
                m_filterByLogLevelBeforeTracing[i]);

            QObject::connect(
                checkbox, &QCheckBox::stateChanged, this,
                &LogViewerWidget::onFilterByLogLevelCheckboxToggled);
        }

        m_logViewerModel->setLogFileName(
            m_logViewerModel->logFileName(), filteringOptions);

        m_ui->filterByLogLevelTableWidget->setEnabled(true);
        m_ui->logFileWipePushButton->setEnabled(true);
    }

    scheduleLogEntriesViewColumnsResize();
}

void LogViewerWidget::onModelError(ErrorString errorDescription)
{
    m_ui->logFilePendingLoadLabel->setText(QString());
    m_ui->statusBarLineEdit->setText(errorDescription.localizedString());
    m_ui->statusBarLineEdit->show();
}

void LogViewerWidget::onModelRowsInserted(
    const QModelIndex & parent, const int first, const int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    scheduleLogEntriesViewColumnsResize();
    m_ui->logFilePendingLoadLabel->setText(QString{});
}

void LogViewerWidget::onModelEndOfLogFileReached()
{
    m_ui->logFilePendingLoadLabel->setText(QString{});
}

void LogViewerWidget::onSaveModelEntriesToFileFinished(
    ErrorString errorDescription)
{
    QObject::disconnect(
        m_logViewerModel, &LogViewerModel::saveModelEntriesToFileFinished,
        this, &LogViewerWidget::onSaveModelEntriesToFileFinished);

    QObject::disconnect(
        m_logViewerModel, &LogViewerModel::saveModelEntriesToFileProgress,
        this, &LogViewerWidget::onSaveModelEntriesToFileProgress);

    m_ui->saveToFileLabel->setText(QString());

    QObject::disconnect(
        m_ui->saveToFileCancelButton, &QPushButton::clicked, this,
        &LogViewerWidget::onCancelSavingTheLogToFileButtonPressed);

    m_ui->saveToFileCancelButton->hide();

    m_ui->saveToFileProgressBar->setValue(0);
    m_ui->saveToFileProgressBar->hide();

    enableUiElementsAfterSavingLogToFile();

    if (!errorDescription.isEmpty()) {
        m_ui->statusBarLineEdit->setText(errorDescription.localizedString());
        m_ui->statusBarLineEdit->show();
    }
    else {
        m_ui->statusBarLineEdit->setText(QString());
        m_ui->statusBarLineEdit->hide();
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

    m_ui->saveToFileProgressBar->setValue(roundedPercent);
}

void LogViewerWidget::onLogEntriesViewContextMenuRequested(const QPoint & pos)
{
    const auto index = m_ui->logEntriesTableView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    delete m_logEntriesContextMenu;
    m_logEntriesContextMenu = new QMenu(this);

    auto * copyAction = new QAction(tr("Copy"), m_logEntriesContextMenu);
    copyAction->setEnabled(true);

    QObject::connect(
        copyAction, &QAction::triggered, this,
        &LogViewerWidget::onLogEntriesViewCopySelectedItemsAction);

    m_logEntriesContextMenu->addAction(copyAction);

    auto * deselectAction =
        new QAction(tr("Clear selection"), m_logEntriesContextMenu);

    deselectAction->setEnabled(true);

    QObject::connect(
        deselectAction, &QAction::triggered, this,
        &LogViewerWidget::onLogEntriesViewDeselectAction);

    m_logEntriesContextMenu->addAction(deselectAction);
    m_logEntriesContextMenu->show();

    m_logEntriesContextMenu->exec(
        m_ui->logEntriesTableView->mapToGlobal(pos));
}

void LogViewerWidget::onLogEntriesViewCopySelectedItemsAction()
{
    QString result;
    QTextStream strm{&result};

    auto * selectionModel = m_ui->logEntriesTableView->selectionModel();
    if (Q_UNLIKELY(!selectionModel)) {
        return;
    }

    auto selectedIndexes = selectionModel->selectedIndexes();
    std::set<int> processedRows;

    for (const auto & modelIndex: std::as_const(selectedIndexes)) {
        if (Q_UNLIKELY(!modelIndex.isValid())) {
            continue;
        }

        const int row = modelIndex.row();
        if (processedRows.find(row) != processedRows.end()) {
            continue;
        }

        Q_UNUSED(processedRows.insert(row))

        const LogViewerModel::Data * dataEntry =
            m_logViewerModel->dataEntry(row);
        if (Q_UNLIKELY(!dataEntry)) {
            continue;
        }

        strm << m_logViewerModel->dataEntryToString(*dataEntry);
    }

    strm.flush();
    copyStringToClipboard(result);
}

void LogViewerWidget::onLogEntriesViewDeselectAction()
{
    m_ui->logEntriesTableView->clearSelection();
}

void LogViewerWidget::onWipeLogPushButtonPressed()
{
    const QString currentLogFileName = m_logViewerModel->logFileName();
    if (Q_UNLIKELY(currentLogFileName.isEmpty())) {
        return;
    }

    QDir dir{QuentierLogFilesDirPath()};
    if (Q_UNLIKELY(!dir.exists())) {
        return;
    }

    QFileInfo currentLogFileInfo{dir.absoluteFilePath(currentLogFileName)};
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
    if (!m_logViewerModel->wipeCurrentLogFile(errorDescription)) {
        Q_UNUSED(warningMessageBox(
            this, tr("Failed to wipe the log file"),
            tr("Error wiping out the contents of the log file"),
            errorDescription.localizedString()))
    }
}

void LogViewerWidget::clear()
{
    m_ui->logFileComboBox->clear();
    m_ui->logFilePendingLoadLabel->clear();
    m_logViewerModel->clear();

    m_ui->statusBarLineEdit->clear();
    m_ui->statusBarLineEdit->hide();
}

void LogViewerWidget::scheduleLogEntriesViewColumnsResize()
{
    if (m_delayedSectionResizeTimer.isActive()) {
        // Already scheduled
        return;
    }

    m_delayedSectionResizeTimer.start(100, this);
}

void LogViewerWidget::resizeLogEntriesViewColumns()
{
    auto * horizontalHeader = m_ui->logEntriesTableView->horizontalHeader();
    horizontalHeader->resizeSections(QHeaderView::ResizeToContents);

    std::array<LogViewerModel::Column, 2> columns{
        LogViewerModel::Column::SourceFileName,
        LogViewerModel::Column::Component};

    for (const auto column: columns) {
        if (horizontalHeader->sectionSize(static_cast<int>(column)) >
            LogViewerDelegate::maxSourceFileNameColumnWidth())
        {
            horizontalHeader->resizeSection(
                static_cast<int>(column),
                LogViewerDelegate::maxSourceFileNameColumnWidth());
        }
    }

    m_ui->logEntriesTableView->verticalHeader()->resizeSections(
        QHeaderView::ResizeToContents);
}

void LogViewerWidget::copyStringToClipboard(const QString & text)
{
    auto * clipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!clipboard)) {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't copy data to clipboard: got null pointer to "
                       "clipboard from app")};

        QNWARNING("widget::LogViewerWidget", errorDescription);
        m_ui->statusBarLineEdit->setText(errorDescription.localizedString());
        m_ui->statusBarLineEdit->show();
        return;
    }

    clipboard->setText(text);
}

void LogViewerWidget::showLogFileIsLoadingLabel()
{
    m_ui->logFilePendingLoadLabel->setText(
        tr("Loading, please wait") + QStringLiteral("..."));
}

void LogViewerWidget::collectModelFilteringOptions(
    LogViewerModel::FilteringOptions & options) const
{
    options.clear();
    options.m_logEntryContentFilter = m_ui->filterByContentLineEdit->text();

    for (std::size_t i = 0; i < gQuentierNumLogLevels; ++i) {
        auto * checkbox = m_logLevelEnabledCheckboxPtrs[i];
        if (Q_UNLIKELY(!checkbox)) {
            continue;
        }

        if (checkbox->isChecked()) {
            continue;
        }

        options.m_disabledLogLevels << static_cast<LogLevel>(i);
    }
}

void LogViewerWidget::enableUiElementsAfterSavingLogToFile()
{
    m_ui->saveToFilePushButton->setEnabled(true);
    m_ui->clearPushButton->setEnabled(true);

    if (m_logViewerModel->startLogFilePos() > 0) {
        m_ui->resetPushButton->setEnabled(true);
    }

    m_ui->tracePushButton->setEnabled(true);
    m_ui->logFileWipePushButton->setEnabled(true);
    m_ui->filterByContentLineEdit->setEnabled(true);
    m_ui->filterByLogLevelTableWidget->setEnabled(true);

    m_ui->logFileComboBox->setEnabled(true);
    m_ui->logLevelComboBox->setEnabled(true);
}

void LogViewerWidget::saveFilterByComponentState()
{
    QNDEBUG("widget::LogViewerWidget", "LogViewerWidget::saveFilterByComponentState");

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::loggingGroup);

    appSettings.setValue(
        preferences::keys::loggingFilterByComponentPreset,
        m_ui->filterByComponentPresetsComboBox->currentIndex());

    appSettings.setValue(
        preferences::keys::loggingFilterByComponentRegex,
        m_ui->filterByComponentRegexLineEdit->text());

    appSettings.endGroup();
}

void LogViewerWidget::restoreFilterByComponentState()
{
    QNDEBUG(
        "widget::LogViewerWidget", "LogViewerWidget::restoreFilterByComponentState");

    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::loggingGroup);

    auto presetIndex =
        appSettings.value(preferences::keys::loggingFilterByComponentPreset);

    auto filter =
        appSettings.value(preferences::keys::loggingFilterByComponentRegex)
            .toString();

    appSettings.endGroup();

    bool conversionResult = false;
    int presetIndexInt = presetIndex.toInt(&conversionResult);
    if (!conversionResult) {
        presetIndexInt = 0;
    }

    m_ui->filterByComponentPresetsComboBox->setCurrentIndex(presetIndexInt);
    m_ui->filterByComponentRegexLineEdit->setText(filter);
}

void LogViewerWidget::timerEvent(QTimerEvent * event)
{
    if (Q_UNLIKELY(!event)) {
        return;
    }

    if (event->timerId() == m_delayedSectionResizeTimer.timerId()) {
        resizeLogEntriesViewColumns();
        m_delayedSectionResizeTimer.stop();
    }
}

void LogViewerWidget::closeEvent(QCloseEvent * event)
{
    if (m_ui->tracePushButton->isChecked()) {
        // Restore the previously backed up log level
        QuentierSetMinLogLevel(m_minLogLevelBeforeTracing);
    }

    QWidget::closeEvent(event);
}

} // namespace quentier
