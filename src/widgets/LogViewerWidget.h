#ifndef QUENTIER_LOG_VIEWER_WIDGET_H
#define QUENTIER_LOG_VIEWER_WIDGET_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/FileSystemWatcher.h>
#include <quentier/logging/QuentierLogger.h>
#include <QWidget>

namespace Ui {
class LogViewerWidget;
}

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
    void onFilterByLogLevelCheckboxToggled(int state);

    void onLogFileDirRemoved(const QString & path);
    void onLogFileDirChanged(const QString & path);

private:
    void clear();

private:
    Ui::LogViewerWidget *   m_pUi;
    LogLevel::type          m_logLevels[6];
    bool                    m_logLevelEnabledStates[6];
    FileSystemWatcher       m_logFilesFolderWatcher;

    LogViewerModel *        m_pLogViewerModel;
    LogViewerFilterModel *  m_pLogViewerFilterModel;
};

} // namespace quentier

#endif // QUENTIER_LOG_VIEWER_WIDGET_H
