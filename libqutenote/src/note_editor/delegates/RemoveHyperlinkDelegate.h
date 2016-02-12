#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATE__REMOVE_HYPERLINK_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATE__REMOVE_HYPERLINK_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <qute_note/types/Note.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)

/**
 * @brief The RemoveHyperlinkDelegate class encapsulates a chain of callbacks
 * required for proper implementation of removing a hyperlink under cursor
 * considering the details of wrapping this action around the undo stack
 */
class RemoveHyperlinkDelegate: public QObject
{
    Q_OBJECT
public:
    explicit RemoveHyperlinkDelegate(NoteEditorPrivate & noteEditor);

    void start();

Q_SIGNALS:
    void finished();
    void notifyError(QString error);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onHyperlinkIdFound(const QVariant & data);
    void onHyperlinkRemoved(const QVariant & data);

private:
    void findIdOfHyperlinkUnderCursor();
    void removeHyperlink(const quint64 hyperlinkId);

private:
    typedef JsResultCallbackFunctor<RemoveHyperlinkDelegate> JsCallback;

private:
    NoteEditorPrivate &     m_noteEditor;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATE__REMOVE_HYPERLINK_DELEGATE_H
