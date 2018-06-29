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

#ifndef QUENTIER_LOG_VIEWER_WIDGET_H
#define QUENTIER_LOG_VIEWER_WIDGET_H

#include "../models/LogViewerModel.h"
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
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace quentier {

class LogViewerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogViewerWidget(QWidget * parent = Q_NULLPTR);
    virtual ~LogViewerWidget();

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

    void onSaveAllToFileButtonPressed();

    void onClearButtonPressed();
    void onResetButtonPressed();

    void onTraceButtonToggled(bool checked);

    void onModelError(ErrorString errorDescription);
    void onModelRowsInserted(const QModelIndex & parent, int first, int last);

    void onSaveModelEntriesToFileFinished(ErrorString errorDescription);
    void onSaveModelEntriesToFileProgress(double progressPercent);

    void onLogEntriesViewContextMenuRequested(const QPoint & pos);
    void onLogEntriesViewCopySelectedItemsAction();
    void onLogEntriesViewDeselectAction();

    void onWipeLogPushButtonPressed();

private:
    void clear();

    void scheduleLogEntriesViewColumnsResize();
    void resizeLogEntriesViewColumns();

    void copyStringToClipboard(const QString & text);
    void showLogFileIsLoadingLabel();

    void collectModelFilteringOptions(LogViewerModel::FilteringOptions & options) const;

private:
    virtual void timerEvent(QTimerEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void closeEvent(QCloseEvent * pEvent) Q_DECL_OVERRIDE;

private:
    Ui::LogViewerWidget *   m_pUi;
    FileSystemWatcher       m_logFilesFolderWatcher;

    LogViewerModel *        m_pLogViewerModel;

    QBasicTimer             m_delayedSectionResizeTimer;
    QBasicTimer             m_logViewerModelLoadingTimer;

    QCheckBox *             m_logLevelEnabledCheckboxPtrs[6];
    QMenu *                 m_pLogEntriesContextMenu;

    // Backups for tracing mode
    LogLevel::type          m_minLogLevelBeforeTracing;
    QString                 m_filterByContentBeforeTracing;
    bool                    m_filterByLogLevelBeforeTracing[6];
    qint64                  m_startLogFilePosBeforeTracing;
};

} // namespace quentier

#endif // QUENTIER_LOG_VIEWER_WIDGET_H
