#ifndef QUENTIER_LOG_VIEWER_WIDGET_H
#define QUENTIER_LOG_VIEWER_WIDGET_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/FileSystemWatcher.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/ErrorString.h>
#include <QWidget>
#include <QBasicTimer>
#include <QModelIndex>

namespace Ui {
class LogViewerWidget;
}

QT_FORWARD_DECLARE_CLASS(QCheckBox)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LogViewerModel)
QT_FORWARD_DECLARE_CLASS(LogViewerFilterModel)

class LogViewerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogViewerWidget(QWidget * parent = Q_NULLPTR);
    ~LogViewerWidget();

private:
    void setupLogLevels();
    void setupLogFiles();
    void startWatchingForLogFilesFolderChanges();
    void setupFilterByLogLevelWidget();

private Q_SLOTS:
    void onCurrentLogLevelChanged(int index);
    void onFilterByContentEditingFinished();
    void onFilterByLogLevelCheckboxToggled(int state);

    void onCurrentLogFileChanged(const QString & currentLogFile);

    void onLogFileDirRemoved(const QString & path);
    void onLogFileDirChanged(const QString & path);

    void onCopyAllToClipboardButtonPressed();
    void onSaveAllToFileButtonPressed();

    void onClearButtonPressed();
    void onResetButtonPressed();

    void onTraceButtonToggled(bool checked);

    void onModelError(ErrorString errorDescription);

    void onModelRowsInserted(const QModelIndex & parent, int first, int last);

private:
    void clear();

    void scheduleLogEntriesViewColumnsResize();
    void resizeLogEntriesViewColumns();

    QString displayedLogEntriesToString() const;

private:
    virtual void timerEvent(QTimerEvent * pEvent) Q_DECL_OVERRIDE;

private:
    Ui::LogViewerWidget *   m_pUi;
    FileSystemWatcher       m_logFilesFolderWatcher;

    LogViewerModel *        m_pLogViewerModel;
    LogViewerFilterModel *  m_pLogViewerFilterModel;

    QBasicTimer             m_modelFetchingMoreTimer;
    QBasicTimer             m_delayedSectionResizeTimer;

    // Backups for tracing mode
    LogLevel::type          m_minLogLevelBeforeTracing;
    QString                 m_filterByContentBeforeTracing;
    bool                    m_filterByLogLevelBeforeTracing[6];
    QCheckBox *             m_logLevelEnabledCheckboxPtrs[6];
};

} // namespace quentier

#endif // QUENTIER_LOG_VIEWER_WIDGET_H
