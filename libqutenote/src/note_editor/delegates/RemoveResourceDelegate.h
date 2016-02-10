#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__REMOVE_RESOURCE_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__REMOVE_RESOURCE_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/Note.h>
#include <qute_note/types/ResourceWrapper.h>
#include <QObject>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)

class RemoveResourceDelegate: public QObject
{
    Q_OBJECT
public:
    explicit RemoveResourceDelegate(const ResourceWrapper & resourceToRemove, NoteEditorPrivate & noteEditor);

    void start();

Q_SIGNALS:
    void finished(ResourceWrapper removedResource);
    void notifyError(QString error);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);

    void onResourceReferenceRemovedFromNoteContent(const QVariant & data);

private:
    void doStart();

private:
    typedef JsResultCallbackFunctor<RemoveResourceDelegate> JsCallback;

private:
    NoteEditorPrivate &             m_noteEditor;
    ResourceWrapper                 m_resource;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__REMOVE_RESOURCE_DELEGATE_H
