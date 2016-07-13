#ifndef LIB_QUENTIER_NOTE_EDITOR_DELEGATES_EDIT_HYPERLINK_DELEGATE_H
#define LIB_QUENTIER_NOTE_EDITOR_DELEGATES_EDIT_HYPERLINK_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/types/Note.h>
#include <QObject>

namespace quentier {

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
    void notifyError(QNLocalizedString error);

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

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DELEGATES_EDIT_HYPERLINK_DELEGATE_H
