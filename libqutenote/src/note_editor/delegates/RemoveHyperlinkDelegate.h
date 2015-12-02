#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATE__REMOVE_HYPERLINK_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATE__REMOVE_HYPERLINK_DELEGATE_H

#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/Note.h>
#include <QObject>
#include <QUuid>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(NoteEditorPage)

/**
 * @brief The RemoveHyperlinkDelegate class encapsulates a chain of callbacks
 * required for proper implementation of removing a hyperlink under cursor
 * considering the details of wrapping this action around undo stack and necessary switching
 * of note editor page during the process
 */
class RemoveHyperlinkDelegate: public QObject
{
    Q_OBJECT
public:
    explicit RemoveHyperlinkDelegate(NoteEditorPrivate & noteEditor, NoteEditorPage * pOriginalPage);

    // This delegate is not only used before the undo command is created but also by undo command itself, for redo operation;
    // In such a case the id of hyperlink to remove should be taken not from the current selection but it should be a known number
    void setHyperlinkId(const quint64 hyperlinkIdToRemove);

    void start();

Q_SIGNALS:
    void finished(quint64 removedHyperlinkId);
    void notifyError(QString error);

// private signals
    void writeFile(QString absoluteFilePath, QByteArray data, QUuid requestId);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onHyperlinkIdFound(const QVariant & data);

    void onNewPageInitialLoadFinished(bool ok);
    void onNewPageJavaScriptLoaded();

    void onNewPageModified();

private:
    void findIdOfHyperlinkUnderCursor();
    void removeHyperlink();

private:
    class JsResultCallbackFunctor
    {
    public:
        typedef void (RemoveHyperlinkDelegate::*Method)(const QVariant &);

        JsResultCallbackFunctor(RemoveHyperlinkDelegate & member, Method method) :
            m_member(member),
            m_method(method)
        {}

        void operator()(const QVariant & data) { (m_member.*m_method)(data); }

    private:
        RemoveHyperlinkDelegate &    m_member;
        Method                       m_method;
    };

private:
    NoteEditorPrivate &     m_noteEditor;
    NoteEditorPage *        m_pOriginalPage;
    quint64                 m_hyperlinkId;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATE__REMOVE_HYPERLINK_DELEGATE_H
