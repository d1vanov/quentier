#include "FilterByTagWidget.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

FilterByTagWidget::FilterByTagWidget(QWidget * parent) :
    AbstractFilterByModelItemWidget(QStringLiteral("Tag"), parent),
    m_pLocalStorageManager()
{}

void FilterByTagWidget::setLocalStorageManager(LocalStorageManagerThreadWorker & localStorageManager)
{
    m_pLocalStorageManager = &localStorageManager;

    QObject::connect(m_pLocalStorageManager.data(), QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(FilterByTagWidget,onUpdateTagCompleted,Tag,QUuid));
    QObject::connect(m_pLocalStorageManager.data(), QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                     this, QNSLOT(FilterByTagWidget,onExpungeTagCompleted,Tag,QUuid));
    QObject::connect(m_pLocalStorageManager.data(), QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotelessTagsFromLinkedNotebooksComplete,QUuid),
                     this, QNSLOT(FilterByTagWidget,onExpungeNotelessTagsFromLinkedNotebooksCompleted,QUuid));
}

void FilterByTagWidget::onUpdateTagCompleted(Tag tag, QUuid requestId)
{
    QNDEBUG(QStringLiteral("FilterByTagWidget::onUpdateTagCompleted: request id = ")
            << requestId << QStringLiteral(", tag = ") << tag);

    if (Q_UNLIKELY(!tag.hasName())) {
        QNWARNING(QStringLiteral("Found tag without a name: ") << tag);
        onItemRemovedFromLocalStorage(tag.localUid());
        return;
    }

    onItemUpdatedInLocalStorage(tag.localUid(), tag.name());
}

void FilterByTagWidget::onExpungeTagCompleted(Tag tag, QUuid requestId)
{
    QNDEBUG(QStringLiteral("FilterByTagWidget::onExpungeTagCompleted: request id = ")
            << requestId << QStringLiteral(", tag = ") << tag);

    onItemRemovedFromLocalStorage(tag.localUid());
}

void FilterByTagWidget::onExpungeNotelessTagsFromLinkedNotebooksCompleted(QUuid requestId)
{
    QNDEBUG(QStringLiteral("FilterByTagWidget::onExpungeNotelessTagsFromLinkedNotebooksCompleted: request id = ")
            << requestId);

    update();
}

} // namespace quentier
