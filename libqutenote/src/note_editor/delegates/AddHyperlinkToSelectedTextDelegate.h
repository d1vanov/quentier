#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_HYPERLINK_TO_SELECTED_TEXT_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_HYPERLINK_TO_SELECTED_TEXT_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/Note.h>
#include <QObject>
#include <QUuid>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(NoteEditorPage)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

/**
 * @brief The AddHyperlinkToSelectedTextDelegate class encapsulates a chain of callbacks
 * required for proper implementation of adding a hyperlink to the currently selected text
 * considering the details of wrapping this action around undo stack and necessary switching
 * of note editor page during the process
 */
class AddHyperlinkToSelectedTextDelegate: public QObject
{
    Q_OBJECT
public:
    explicit AddHyperlinkToSelectedTextDelegate(NoteEditorPrivate & noteEditor,
                                                NoteEditorPage * pOriginalPage,
                                                FileIOThreadWorker * pFileIOThreadWorker,
                                                const quint64 hyperlinkIdToAdd);
    void start();

Q_SIGNALS:
    void finished(QString htmlWithAddedHyperlink, int pageXOffset, int pageYOffset);
    void cancelled();
    void notifyError(QString error);

// private signals
    void writeFile(QString absoluteFilePath, QByteArray data, QUuid requestId);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onInitialHyperlinkDataReceived(const QVariant & data);

    void onAddHyperlinkDialogFinished(QString text, QUrl url, quint64 hyperlinkId, bool startupUrlWasEmpty);

    void onOriginalPageModified(const QVariant & data);
    void onPageScrollReceived(const QVariant & data);
    void onOriginalPageModificationUndone(const QVariant & data);

    void onModifiedPageHtmlReceived(const QString & html);
    void onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId);
    void onModifiedPageLoaded();

private:
    void requestPageScroll();
    void addHyperlinkToSelectedText();
    void raiseAddHyperlinkDialog(const QString & initialText);

private:
    typedef JsResultCallbackFunctor<AddHyperlinkToSelectedTextDelegate> JsCallback;

    class HtmlCallbackFunctor
    {
    public:
        typedef void (AddHyperlinkToSelectedTextDelegate::*Method)(const QString &);

        HtmlCallbackFunctor(AddHyperlinkToSelectedTextDelegate & member, Method method) :
            m_member(member),
            m_method(method)
        {}

        void operator()(const QString & html) { (m_member.*m_method)(html); }

    private:
        AddHyperlinkToSelectedTextDelegate &    m_member;
        Method                                  m_method;
    };

private:
    NoteEditorPrivate &     m_noteEditor;
    NoteEditorPage *        m_pOriginalPage;
    FileIOThreadWorker *    m_pFileIOThreadWorker;

    const quint64           m_hyperlinkId;
    QString                 m_modifiedHtml;
    QUuid                   m_writeModifiedHtmlToPageSourceRequestId;

    int                     m_pageXOffset;
    int                     m_pageYOffset;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_HYPERLINK_TO_SELECTED_TEXT_DELEGATE_H
