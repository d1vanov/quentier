#ifndef QUENTIER_WIDGETS_FILTER_BY_TAG_WIDGET_H
#define QUENTIER_WIDGETS_FILTER_BY_TAG_WIDGET_H

#include "AbstractFilterByModelItemWidget.h"
#include <quentier/types/Tag.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QUuid>
#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

class FilterByTagWidget: public AbstractFilterByModelItemWidget
{
    Q_OBJECT
public:
    explicit FilterByTagWidget(QWidget * parent = Q_NULLPTR);

    void setLocalStorageManager(LocalStorageManagerThreadWorker & localStorageManager);

private Q_SLOTS:
    void onUpdateTagCompleted(Tag tag, QUuid requestId);
    void onExpungeTagCompleted(Tag tag, QUuid requestId);
    void onExpungeNotelessTagsFromLinkedNotebooksCompleted(QUuid requestId);

private:
    QPointer<LocalStorageManagerThreadWorker>   m_pLocalStorageManager;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_FILTER_BY_TAG_WIDGET_H
