#ifndef QUENTIER_WIDGETS_FILTER_BY_TAG_WIDGET_H
#define QUENTIER_WIDGETS_FILTER_BY_TAG_WIDGET_H

#include "AbstractFilterByModelItemWidget.h"
#include <quentier/types/Tag.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QUuid>
#include <QSet>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)

class FilterByTagWidget: public AbstractFilterByModelItemWidget
{
    Q_OBJECT
public:
    explicit FilterByTagWidget(LocalStorageManagerThreadWorker & localStorageManager,
                               QWidget * parent = Q_NULLPTR);

Q_SIGNALS:
    // private signals
    void findTag(Tag tag, QUuid requestId);

private Q_SLOTS:
    void onFindTagCompleted(Tag tag, QUuid requestId);
    void onFindTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);

    void onUpdateTagCompleted(Tag tag, QUuid requestId);
    void onExpungeTagCompleted(Tag tag, QUuid requestId);

    // TODO: should also account for the case of expunging the noteless tags from linked notebooks

private:
    virtual void findItemInLocalStorage(const QString & localUid) Q_DECL_OVERRIDE;

private:
    LocalStorageManagerThreadWorker &   m_localStorageManager;
    QSet<QUuid>                         m_findTagRequestIds;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_FILTER_BY_TAG_WIDGET_H
