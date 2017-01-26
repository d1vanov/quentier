#include "EditNoteDialogsManager.h"
#include "dialogs/EditNoteDialog.h"
#include "models/NotebookModel.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>
#include <QScopedPointer>

namespace quentier {

EditNoteDialogsManager::EditNoteDialogsManager(LocalStorageManagerThreadWorker & localStorageWorker,
                                               NoteCache & noteCache, NotebookModel * pNotebookModel, QWidget * parent) :
    QObject(parent),
    m_localStorageWorker(localStorageWorker),
    m_noteCache(noteCache),
    m_findNoteRequestIds(),
    m_updateNoteRequestIds(),
    m_pNotebookModel(pNotebookModel)
{
    createConnections();
}

void EditNoteDialogsManager::setNotebookModel(NotebookModel * pNotebookModel)
{
    QNDEBUG(QStringLiteral("EditNoteDialogsManager::setNotebookModel"));
    m_pNotebookModel = pNotebookModel;
}

void EditNoteDialogsManager::onEditNoteDialogRequested(QString noteLocalUid)
{
    QNDEBUG(QStringLiteral("EditNoteDialogsManager::onEditNoteDialogRequested: note local uid = ") << noteLocalUid);

    const Note * pCachedNote = m_noteCache.get(noteLocalUid);
    if (pCachedNote) {
        raiseEditNoteDialog(*pCachedNote);
        return;
    }

    // Otherwise need to find the note in the local storage
    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_findNoteRequestIds.insert(requestId))
    Note note;
    note.setLocalUid(noteLocalUid);
    QNTRACE(QStringLiteral("Emitting the request to find note: requets id = ") << requestId
            << QStringLiteral(", note local uid = ") << noteLocalUid);
    emit findNote(note, /* with resource binary data = */ false, requestId);
}

void EditNoteDialogsManager::onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId)
{
    auto it = m_findNoteRequestIds.find(requestId);
    if (it == m_findNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("EditNoteDialogsManager::onFindNoteComplete: request id = ") << requestId
            << QStringLiteral(", with resource binary data = ") << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", note: ") << note);

    Q_UNUSED(m_findNoteRequestIds.erase(it))

    m_noteCache.put(note.localUid(), note);
    raiseEditNoteDialog(note);
}

void EditNoteDialogsManager::onFindNoteFailed(Note note, bool withResourceBinaryData, QNLocalizedString errorDescription,
                                              QUuid requestId)
{
    auto it = m_findNoteRequestIds.find(requestId);
    if (it == m_findNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("EditNoteDialogsManager::onFindNoteFailed: request id = ") << requestId
            << QStringLiteral(", with resource binary data = ") << (withResourceBinaryData ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description = ") << errorDescription
            << QStringLiteral("; note: ") << note);

    Q_UNUSED(m_findNoteRequestIds.erase(it))

    QNLocalizedString error = QNLocalizedString("Can't edit note: note to edit was no found", this);
    error += QStringLiteral(": ");
    error += errorDescription;
    QNWARNING(error);
    emit notifyError(error);
}

void EditNoteDialogsManager::onUpdateNoteComplete(Note note, bool updateResources, bool updateTags, QUuid requestId)
{
    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("EditNoteDialogsManager::onUpdateNoteComplete: request id = ") << requestId
            << QStringLiteral(", update resources = ") << (updateResources ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", update tags = ") << (updateTags ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", note: ") << note);

    m_noteCache.put(note.localUid(), note);
}

void EditNoteDialogsManager::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                                QNLocalizedString errorDescription, QUuid requestId)
{
    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNWARNING(QStringLiteral("EditNoteDialogsManager::onUpdateNoteFailed: request id = ") << requestId
              << QStringLiteral(", update resources = ") << (updateResources ? QStringLiteral("true") : QStringLiteral("false"))
              << QStringLiteral(", update tags = ") << (updateTags ? QStringLiteral("true") : QStringLiteral("false"))
              << QStringLiteral(", error: ") << errorDescription << QStringLiteral("; note: ") << note);

    QNLocalizedString error = QNLocalizedString("Note edit failed");
    error += QStringLiteral(": ");
    error += errorDescription;
    emit notifyError(error);
}

void EditNoteDialogsManager::createConnections()
{
    QNDEBUG(QStringLiteral("EditNoteDialogsManager::createConnections"));

    QObject::connect(this, QNSIGNAL(EditNoteDialogsManager,findNote,Note,bool,QUuid),
                     &m_localStorageWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNoteRequest,Note,bool,QUuid));
    QObject::connect(this, QNSIGNAL(EditNoteDialogsManager,updateNote,Note,bool,bool,QUuid),
                     &m_localStorageWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,bool,bool,QUuid));

    QObject::connect(&m_localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteComplete,Note,bool,QUuid),
                     this, QNSLOT(EditNoteDialogsManager,onFindNoteComplete,Note,bool,QUuid));
    QObject::connect(&m_localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNoteFailed,Note,bool,QNLocalizedString,QUuid),
                     this, QNSLOT(EditNoteDialogsManager,onFindNoteFailed,Note,bool,QNLocalizedString,QUuid));
}

void EditNoteDialogsManager::raiseEditNoteDialog(const Note & note)
{
    QNDEBUG(QStringLiteral("EditNoteDialogsManager::raiseEditNoteDialog: note local uid = ") << note.localUid());

    QWidget * pWidget = qobject_cast<QWidget*>(parent());
    if (Q_UNLIKELY(!pWidget)) {
        QNLocalizedString error = QNLocalizedString("Can't raise edit note dialog: no parent widget", this);
        QNWARNING(error << QStringLiteral(", parent = ") << parent());
        emit notifyError(error);
        return;
    }

    QNTRACE(QStringLiteral("Note before raising the edit dialog: ") << note);

    QScopedPointer<EditNoteDialog> pEditNoteDialog(new EditNoteDialog(note, m_pNotebookModel.data(), pWidget));
    pEditNoteDialog->setWindowModality(Qt::WindowModal);
    int res = pEditNoteDialog->exec();
    if (res == QDialog::Rejected) {
        QNTRACE(QStringLiteral("Note editing rejected"));
        return;
    }

    Note editedNote = pEditNoteDialog->note();
    QNTRACE(QStringLiteral("Note after possible editing: ") << editedNote);

    if (editedNote == note) {
        QNTRACE(QStringLiteral("Note hasn't changed after the editing, nothing to do"));
        return;
    }

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_updateNoteRequestIds.insert(requestId))
    QNTRACE(QStringLiteral("Emitting the request to update note: request id = ") << requestId);
    emit updateNote(editedNote, /* update resources = */ false, /* update tags = */ false, requestId);
}

} // namespace quentier
