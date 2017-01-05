#include "FilterByTagWidget.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

FilterByTagWidget::FilterByTagWidget(LocalStorageManagerThreadWorker & localStorageManager,
                                     QWidget * parent) :
    AbstractFilterByModelItemWidget(QStringLiteral("Tag"), parent),
    m_localStorageManager(localStorageManager),
    m_findTagRequestIds()
{
    QObject::connect(this, QNSIGNAL(FilterByTagWidget,findTag,Tag,QUuid),
                     &m_localStorageManager, QNSLOT(LocalStorageManagerThreadWorker,onFindTagRequest,Tag,QUuid));
    QObject::connect(&m_localStorageManager, QNSIGNAL(LocalStorageManagerThreadWorker,findTagComplete,Tag,QUuid),
                     this, QNSLOT(FilterByTagWidget,onFindTagCompleted,Tag,QUuid));
    QObject::connect(&m_localStorageManager, QNSIGNAL(LocalStorageManagerThreadWorker,findTagFailed,Tag,QNLocalizedString,QUuid),
                     this, QNSLOT(FilterByTagWidget,onFindTagFailed,Tag,QNLocalizedString,QUuid));
    QObject::connect(&m_localStorageManager, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(FilterByTagWidget,onUpdateTagCompleted,Tag,QUuid));
    QObject::connect(&m_localStorageManager, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                     this, QNSLOT(FilterByTagWidget,onExpungeTagCompleted,Tag,QUuid));
}

void FilterByTagWidget::onFindTagCompleted(Tag tag, QUuid requestId)
{
    auto it = m_findTagRequestIds.find(requestId);
    if (it == m_findTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("FilterByTagWidget::onFindTagCompleted: request id = ")
            << requestId << QStringLiteral(", tag: ") << tag);

    Q_UNUSED(m_findTagRequestIds.erase(it))

    if (Q_UNLIKELY(!tag.hasName())) {
        QNWARNING(QStringLiteral("Found tag without a name: ") << tag);
        onItemNotFoundInLocalStorage(tag.localUid());
        return;
    }

    onItemFoundInLocalStorage(tag.localUid(), tag.name());
}

void FilterByTagWidget::onFindTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_findTagRequestIds.find(requestId);
    if (it == m_findTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("FilterByTagWidget::onFindTagFailed: request id = ")
            << requestId << QStringLiteral(", tag = ") << tag
            << QStringLiteral("\nError description: ") << errorDescription);

    Q_UNUSED(m_findTagRequestIds.erase(it))
    onItemNotFoundInLocalStorage(tag.localUid());
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

void FilterByTagWidget::findItemInLocalStorage(const QString & localUid)
{
    QNDEBUG(QStringLiteral("FilterByTagWidget::findItemInLocalStorage: local uid = ") << localUid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findTagRequestIds.insert(requestId))
    Tag dummy;
    dummy.setLocalUid(localUid);
    QNTRACE(QStringLiteral("Emitting the request to find a tag in the local storage: local uid = ")
            << localUid << QStringLiteral(", request id = ") << requestId);
    emit findTag(dummy, requestId);
}

} // namespace quentier
