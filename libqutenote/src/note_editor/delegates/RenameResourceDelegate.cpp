#include "RenameResourceDelegate.h"
#include "../NoteEditor_p.h"
#include "../GenericResourceImageWriter.h"
#include "../dialogs/RenameResourceDialog.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <QScopedPointer>

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't rename resource: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

RenameResourceDelegate::RenameResourceDelegate(const ResourceWrapper & resource, NoteEditorPrivate & noteEditor,
                                               GenericResourceImageWriter * pGenericResourceImageWriter) :
    m_noteEditor(noteEditor),
    m_pGenericResourceImageWriter(pGenericResourceImageWriter),
    m_resource(resource),
    m_oldResourceName(),
    m_newResourceName()
{}

void RenameResourceDelegate::start()
{
    QNDEBUG("RenameResourceDelegate::start");

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(RenameResourceDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        doStart();
    }
}

void RenameResourceDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("RenameResourceDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(RenameResourceDelegate,onOriginalPageConvertedToNote,Note));

    doStart();
}

void RenameResourceDelegate::doStart()
{
    QNDEBUG("RenameResourceDelegate::doStart");

    if (Q_UNLIKELY(!m_resource.hasDataHash())) {
        QString error = QT_TR_NOOP("Can't rename resource: resource to rename doesn't have the data hash set");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString currentResourceDisplayName = m_resource.displayName();

    QScopedPointer<RenameResourceDialog> pRenameResourceDialog(new RenameResourceDialog(currentResourceDisplayName, &m_noteEditor));
    pRenameResourceDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pRenameResourceDialog.data(), QNSIGNAL(RenameResourceDialog,accepted,QString),
                     this, QNSLOT(RenameResourceDelegate,onRenameResourceDialogFinished,QString));
    QNTRACE("Will exec rename resource dialog now");
    int res = pRenameResourceDialog->exec();
    if (res == QDialog::Rejected) {
        QNTRACE("Cancelled renaming the resource");
        emit cancelled();
    }
}

void RenameResourceDelegate::onRenameResourceDialogFinished(QString newResourceName)
{
    QNDEBUG("RenameResourceDelegate::onRenameResourceDialogFinished: new resource name = " << newResourceName);

    if (newResourceName.isEmpty()) {
        QNTRACE("New resource name is empty, treating it as cancellation");
        emit cancelled();
        return;
    }

    if (newResourceName == m_oldResourceName) {
        QNTRACE("The new resource name is equal to the old one, treating it as cancellation");
        emit cancelled();
        return;
    }

    m_newResourceName = newResourceName;

    // TODO: make generic resource image writer update the image for this resource and wait for this event before finishing
}

} // namespace qute_note

