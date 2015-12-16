#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__EDIT_HYPERLINK_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__EDIT_HYPERLINK_DELEGATE_H

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
    void finished(quint64 hyperlinkId, QString previousText, QString prevousUrl, QString newText, QString newUrl);
    void cancelled();
    void notifyError(QString error);

private Q_SLOTS:
    void onHyperlinkDataReceived(const QVariant & data);
    void onHyperlinkDataEdited(QString text, QUrl url, quint64 hyperlinkId, bool startupUrlWasEmpty);
    void onHyperlinkModified(const QVariant & data);

private:
    void raiseEditHyperlinkDialog();

private:
    typedef JsResultCallbackFunctor<EditHyperlinkDelegate> JsCallback;

private:
    NoteEditorPrivate &     m_noteEditor;
    const quint64           m_hyperlinkId;
    QString                 m_originalHyperlinkText;
    QString                 m_originalHyperlinkUrl;
    QString                 m_newHyperlinkText;
    QString                 m_newHyperlinkUrl;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__EDIT_HYPERLINK_DELEGATE_H
