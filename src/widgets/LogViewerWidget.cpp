#include "LogViewerWidget.h"
#include "ui_LogViewerWidget.h"
#include "models/LogViewerModel.h"
#include "models/LogViewerFilterModel.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QDir>

namespace quentier {

LogViewerWidget::LogViewerWidget(QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::LogViewerWidget),
    m_logLevels(),
    m_logLevelEnabledStates(),
    m_logFilesFolderWatcher(),
    m_pLogViewerModel(new LogViewerModel(this)),
    m_pLogViewerFilterModel(new LogViewerFilterModel(this))
{
    m_pUi->setupUi(this);

    setupLogLevels();
    setupLogFiles();
    setupFilterByLogLevelWidget();

    m_pLogViewerFilterModel->setSourceModel(m_pLogViewerModel);
    m_pUi->logEntriesTableView->setModel(m_pLogViewerFilterModel);
}

LogViewerWidget::~LogViewerWidget()
{
    delete m_pUi;
}

void LogViewerWidget::setupLogLevels()
{
    m_logLevels[0] = LogLevel::FatalLevel;
    m_logLevels[1] = LogLevel::ErrorLevel;
    m_logLevels[2] = LogLevel::WarnLevel;
    m_logLevels[3] = LogLevel::InfoLevel;
    m_logLevels[4] = LogLevel::DebugLevel;
    m_logLevels[5] = LogLevel::TraceLevel;

    LogLevel::type logLevel = QuentierMinLogLevel();

    m_logLevelEnabledStates[0] = (logLevel <= LogLevel::FatalLevel);
    m_logLevelEnabledStates[1] = (logLevel <= LogLevel::ErrorLevel);
    m_logLevelEnabledStates[2] = (logLevel <= LogLevel::WarnLevel);
    m_logLevelEnabledStates[3] = (logLevel <= LogLevel::InfoLevel);
    m_logLevelEnabledStates[4] = (logLevel <= LogLevel::DebugLevel);
    m_logLevelEnabledStates[5] = (logLevel <= LogLevel::TraceLevel);

    m_pUi->logLevelComboBox->addItem(QStringLiteral("Fatal"));
    m_pUi->logLevelComboBox->addItem(QStringLiteral("Error"));
    m_pUi->logLevelComboBox->addItem(QStringLiteral("Warning"));
    m_pUi->logLevelComboBox->addItem(QStringLiteral("Info"));
    m_pUi->logLevelComboBox->addItem(QStringLiteral("Debug"));
    m_pUi->logLevelComboBox->addItem(QStringLiteral("Trace"));

    switch(logLevel)
    {
    case LogLevel::FatalLevel:
        m_pUi->logLevelComboBox->setCurrentIndex(0);
        break;
    case LogLevel::ErrorLevel:
        m_pUi->logLevelComboBox->setCurrentIndex(1);
        break;
    case LogLevel::WarnLevel:
        m_pUi->logLevelComboBox->setCurrentIndex(2);
        break;
    case LogLevel::InfoLevel:
        m_pUi->logLevelComboBox->setCurrentIndex(3);
        break;
    case LogLevel::DebugLevel:
        m_pUi->logLevelComboBox->setCurrentIndex(4);
        break;
    case LogLevel::TraceLevel:
        m_pUi->logLevelComboBox->setCurrentIndex(5);
        break;
    default:
        m_pUi->logLevelComboBox->setCurrentIndex(3);
        break;
    }

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
        currentLogFileName = QStringLiteral("Quentier.log");
    }

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

    // TODO: subscribe for log file combobox's current item change and ensure the connection is unique
    // i.e. the signal would be emitted exactly once

    QString logFileName = entries.at(currentLogFileIndex).fileName();
    if (logFileName == originalLogFileName) {
        // The current log file didn't change, no need to set it to the model etc.
        return;
    }

    m_pLogViewerModel->setLogFileName(logFileName);
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
    m_pUi->filterByLogLevelTableWidget->setRowCount(sizeof(m_logLevels));
    m_pUi->filterByLogLevelTableWidget->setColumnCount(2);

    for(size_t i = 0; i < sizeof(m_logLevels); ++i)
    {
        QTableWidgetItem * pItem = new QTableWidgetItem(LogViewerModel::logLevelToString(m_logLevels[i]));
        m_pUi->filterByLogLevelTableWidget->setItem(static_cast<int>(i), 0, pItem);

        QWidget * pWidget = new QWidget;
        QCheckBox * pCheckbox = new QCheckBox;
        pCheckbox->setObjectName(QString::number(i));
        pCheckbox->setChecked(m_logLevelEnabledStates[i]);
        QHBoxLayout * pLayout = new QHBoxLayout(pWidget);
        pLayout->addWidget(pCheckbox);
        pLayout->setAlignment(Qt::AlignCenter);
        pLayout->setContentsMargins(0, 0, 0, 0);
        pWidget->setLayout(pLayout);
        m_pUi->filterByLogLevelTableWidget->setCellWidget(static_cast<int>(i), 1, pWidget);

        QObject::connect(pCheckbox, QNSIGNAL(QCheckBox,stateChanged,int),
                         this, QNSLOT(LogViewerWidget,onFilterByLogLevelCheckboxToggled,int));
    }
}

void LogViewerWidget::onCurrentLogLevelChanged(int index)
{
    QNDEBUG(QStringLiteral("LogViewerWidget::onCurrentLogLevelChanged: ") << index);

    if (Q_UNLIKELY((index < 0) || (index >= static_cast<int>(sizeof(m_logLevels))))) {
        return;
    }

    QuentierSetMinLogLevel(static_cast<LogLevel::type>(index));
}

void LogViewerWidget::onFilterByLogLevelCheckboxToggled(int state)
{
    QNDEBUG(QStringLiteral("LogViewerWidget::onFilterByLogLevelCheckboxToggled: state = ") << state);

    QCheckBox * pCheckbox = qobject_cast<QCheckBox*>(sender());
    if (Q_UNLIKELY(!pCheckbox)) {
        return;
    }

    bool parsedCheckboxRow = false;
    int checkboxRow = pCheckbox->objectName().toInt(&parsedCheckboxRow);
    if (Q_UNLIKELY(!parsedCheckboxRow)) {
        return;
    }

    if (Q_UNLIKELY((checkboxRow < 0) || (checkboxRow >= static_cast<int>(sizeof(m_logLevels))))) {
        return;
    }

    LogLevel::type minLogLevel = QuentierMinLogLevel();

    // FIXME: it feels it's better to delegate this kind of filtering to LogViewerFilterModel

    if ((state == Qt::Checked) && (minLogLevel < m_logLevels[checkboxRow]))
    {
        QuentierSetMinLogLevel(m_logLevels[checkboxRow]);
    }
    else if (state == Qt::Unchecked)
    {
        int currentMinLogLevel = -1;
        for(size_t i = 0; i < sizeof(m_logLevelEnabledStates); ++i)
        {
            if (m_logLevelEnabledStates[i] &&
                ((currentMinLogLevel < 0) || (currentMinLogLevel > m_logLevels[i])))
            {
                currentMinLogLevel = m_logLevels[i];
            }
        }

        if ((currentMinLogLevel >= 0) && (currentMinLogLevel < minLogLevel)) {
            QuentierSetMinLogLevel(static_cast<LogLevel::type>(currentMinLogLevel));
        }
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

void LogViewerWidget::clear()
{
    // TODO: implement
    // 1) clear the current log file
    // 2) clear the log viewer model
}

} // namespace quentier
