#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_EDIT_HYPERLINK_DELEGATE_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_EDIT_HYPERLINK_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/Note.h>
#include <QObject>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(NoteEditorPage)

class EditHyperlinkDelegate: public QObject
{
    Q_OBJECT
public:
    explicit EditHyperlinkDelegate(NoteEditorPrivate & noteEditor, const quint64 hyperlinkId);

    void start();

Q_SIGNALS:
    void finished();
    void cancelled();
    void notifyError(QString error);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onHyperlinkDataReceived(const QVariant & data);
    void onHyperlinkDataEdited(QString text, QUrl url, quint64 hyperlinkId, bool startupUrlWasEmpty);
    void onHyperlinkModified(const QVariant & data);

private:
    void doStart();
    void raiseEditHyperlinkDialog(const QString & startupHyperlinkText, const QString & startupHyperlinkUrl);

private:
    typedef JsResultCallbackFunctor<EditHyperlinkDelegate> JsCallback;

private:
    NoteEditorPrivate &     m_noteEditor;
    const quint64           m_hyperlinkId;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_EDIT_HYPERLINK_DELEGATE_H
