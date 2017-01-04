#ifndef QUENTIER_WIDGETS_FILTER_BY_NOTEBOOK_WIDGET_H
#define QUENTIER_WIDGETS_FILTER_BY_NOTEBOOK_WIDGET_H

#include "AbstractFilterByModelItemWidget.h"
#include <quentier/types/Notebook.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QUuid>
#include <QSet>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

class FilterByNotebookWidget: public AbstractFilterByModelItemWidget
{
    Q_OBJECT
public:
    explicit FilterByNotebookWidget(LocalStorageManagerThreadWorker & localStorageManager,
                                    QWidget * parent = Q_NULLPTR);

Q_SIGNALS:
    // private signals
    void findNotebook(Notebook notebook, QUuid requestId);

private Q_SLOTS:
    void onFindNotebookCompleted(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId);

    void onUpdateNotebookCompleted(Notebook notebook, QUuid requestId);
    void onExpungeNotebookCompleted(Notebook notebook, QUuid requestId);

private:
    virtual void findItemInLocalStorage(const QString & localUid) Q_DECL_OVERRIDE;

private:
    LocalStorageManagerThreadWorker &   m_localStorageManager;
    QSet<QUuid>                         m_findNotebookRequestIds;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_FILTER_BY_NOTEBOOK_WIDGET_H
