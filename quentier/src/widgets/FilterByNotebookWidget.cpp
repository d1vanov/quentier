#include "FilterByNotebookWidget.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

FilterByNotebookWidget::FilterByNotebookWidget(LocalStorageManagerThreadWorker & localStorageManager,
                                               QWidget * parent) :
    AbstractFilterByModelItemWidget(QStringLiteral("Notebook"), parent),
    m_localStorageManager(localStorageManager),
    m_findNotebookRequestIds()
{
    QObject::connect(this, QNSIGNAL(FilterByNotebookWidget,findNotebook,Notebook,QUuid),
                     &m_localStorageManager, QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));
    QObject::connect(&m_localStorageManager, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(FilterByNotebookWidget,onFindNotebookCompleted,Notebook,QUuid));
    QObject::connect(&m_localStorageManager, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(FilterByNotebookWidget,onFindNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(&m_localStorageManager, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(FilterByNotebookWidget,onUpdateNotebookCompleted,Notebook,QUuid));
    QObject::connect(&m_localStorageManager, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(FilterByNotebookWidget,onExpungeNotebookCompleted,Notebook,QUuid));
}

void FilterByNotebookWidget::onFindNotebookCompleted(Notebook notebook, QUuid requestId)
{
    auto it = m_findNotebookRequestIds.find(requestId);
    if (it == m_findNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("FilterByNotebookWidget::onFindNotebookCompleted: request id = ")
            << requestId << QStringLiteral(", notebook: ") << notebook);

    Q_UNUSED(m_findNotebookRequestIds.erase(it))

    if (Q_UNLIKELY(!notebook.hasName())) {
        QNWARNING(QStringLiteral("Found notebook without a name: ") << notebook);
        onItemNotFoundInLocalStorage(notebook.localUid());
        return;
    }

    onItemFoundInLocalStorage(notebook.localUid(), notebook.name());
}

void FilterByNotebookWidget::onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_findNotebookRequestIds.find(requestId);
    if (it == m_findNotebookRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("FilterByNotebookWidget::onFindNotebookFailed: request id = ")
            << requestId << QStringLiteral(", error description = ") << errorDescription
            << QStringLiteral(", notebook: ") << notebook);

    Q_UNUSED(m_findNotebookRequestIds.erase(it))
    onItemNotFoundInLocalStorage(notebook.localUid());
}

void FilterByNotebookWidget::onUpdateNotebookCompleted(Notebook notebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("FilterByNotebookWidget::onUpdateNotebookCompleted: request id = ")
            << requestId << QStringLiteral(", notebook = ") << notebook);

    if (Q_UNLIKELY(!notebook.hasName())) {
        QNWARNING(QStringLiteral("Found notebook without a name: ") << notebook);
        onItemRemovedFromLocalStorage(notebook.localUid());
        return;
    }

    onItemUpdatedInLocalStorage(notebook.localUid(), notebook.name());
}

void FilterByNotebookWidget::onExpungeNotebookCompleted(Notebook notebook, QUuid requestId)
{
    QNDEBUG(QStringLiteral("FilterByNotebookWidget::onExpungeNotebookCompleted: request id = ")
            << requestId << QStringLiteral(", notebook = ") << notebook);

    onItemRemovedFromLocalStorage(notebook.localUid());
}

void FilterByNotebookWidget::findItemInLocalStorage(const QString & localUid)
{
    QNDEBUG(QStringLiteral("FilterByNotebookWidget::findItemInLocalStorage: local uid = ") << localUid);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findNotebookRequestIds.insert(requestId))
    Notebook dummy;
    dummy.setLocalUid(localUid);
    QNTRACE(QStringLiteral("Emitting the request to find a notebook in the local storage: local uid = ")
            << localUid << QStringLiteral(", request id = ") << requestId);
    emit findNotebook(dummy, requestId);
}

} // namespace quentier
