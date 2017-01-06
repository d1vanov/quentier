#ifndef QUENTIER_WIDGETS_FILTER_BY_NOTEBOOK_WIDGET_H
#define QUENTIER_WIDGETS_FILTER_BY_NOTEBOOK_WIDGET_H

#include "AbstractFilterByModelItemWidget.h"
#include <quentier/types/Notebook.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QUuid>
#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

class FilterByNotebookWidget: public AbstractFilterByModelItemWidget
{
    Q_OBJECT
public:
    explicit FilterByNotebookWidget(QWidget * parent = Q_NULLPTR);

    void setLocalStorageManager(LocalStorageManagerThreadWorker & localStorageManager);

private Q_SLOTS:
    void onUpdateNotebookCompleted(Notebook notebook, QUuid requestId);
    void onExpungeNotebookCompleted(Notebook notebook, QUuid requestId);

private:
    QPointer<LocalStorageManagerThreadWorker>   m_pLocalStorageManager;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_FILTER_BY_NOTEBOOK_WIDGET_H
