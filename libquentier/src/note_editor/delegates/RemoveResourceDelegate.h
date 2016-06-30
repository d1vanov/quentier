#ifndef LIB_QUENTIER_NOTE_EDITOR_DELEGATES_REMOVE_RESOURCE_DELEGATE_H
#define LIB_QUENTIER_NOTE_EDITOR_DELEGATES_REMOVE_RESOURCE_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <quentier/utility/Qt4Helper.h>
#include <quentier/types/Note.h>
#include <quentier/types/ResourceWrapper.h>
#include <QObject>

namespace quentier {

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

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DELEGATES_REMOVE_RESOURCE_DELEGATE_H
