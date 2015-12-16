#include "EditHyperlinkDelegate.h"
#include "../NoteEditor_p.h"
#include "../NoteEditorPage.h"
#include "../dialogs/EditHyperlinkDialog.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <QScopedPointer>
#include <QStringList>

namespace qute_note {

EditHyperlinkDelegate::EditHyperlinkDelegate(NoteEditorPrivate & noteEditor, const quint64 hyperlinkId) :
    QObject(&noteEditor),
    m_noteEditor(noteEditor),
    m_hyperlinkId(hyperlinkId),
    m_originalHyperlinkText(),
    m_originalHyperlinkUrl(),
    m_newHyperlinkText(),
    m_newHyperlinkUrl()
{}

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't edit hyperlink: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

void EditHyperlinkDelegate::start()
{
    QNDEBUG("EditHyperlinkDelegate::start");

    QString javascript = "getHyperlinkData(" + QString::number(m_hyperlinkId) + ");";

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &EditHyperlinkDelegate::onHyperlinkDataReceived));
}

void EditHyperlinkDelegate::onHyperlinkDataReceived(const QVariant & data)
{
    QNDEBUG("EditHyperlinkDelegate::onHyperlinkDataReceived: data = " << data);

    QStringList hyperlinkDataList = data.toStringList();
    if (hyperlinkDataList.isEmpty()) {
        QString error = QT_TR_NOOP("Can't edit hyperlink: can't find hyperlink text and link");
        emit notifyError(error);
        return;
    }

    if (hyperlinkDataList.size() != 2) {
        QString error = QT_TR_NOOP("Can't edit hyperlink: can't parse hyperlink text and link from JavaScript response");
        QNWARNING(error << "; hyperlink data: " << hyperlinkDataList.join(","));
        emit notifyError(error);
        return;
    }

    m_originalHyperlinkText = hyperlinkDataList[0];
    m_originalHyperlinkUrl = hyperlinkDataList[1];
    raiseEditHyperlinkDialog();
}

void EditHyperlinkDelegate::raiseEditHyperlinkDialog()
{
    QNDEBUG("EditHyperlinkDelegate::raiseEditHyperlinkDialog: original text = " << m_originalHyperlinkText
            << ", original url: " << m_originalHyperlinkUrl);

    QScopedPointer<EditHyperlinkDialog> pEditHyperlinkDialog(new EditHyperlinkDialog(&m_noteEditor, m_originalHyperlinkText, m_originalHyperlinkUrl));
    pEditHyperlinkDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pEditHyperlinkDialog.data(), QNSIGNAL(EditHyperlinkDialog,accepted,QString,QUrl,quint64,bool),
                     this, QNSLOT(EditHyperlinkDelegate,onHyperlinkDataEdited,QString,QUrl,quint64,bool));
    QNTRACE("Will exec edit hyperlink dialog now");
    int res = pEditHyperlinkDialog->exec();
    if (res == QDialog::Rejected) {
        QNTRACE("Cancelled editing the hyperlink");
        emit cancelled();
        return;
    }
}

void EditHyperlinkDelegate::onHyperlinkDataEdited(QString text, QUrl url, quint64 hyperlinkId, bool startupUrlWasEmpty)
{
    QNDEBUG("EditHyperlinkDelegate::onHyperlinkDataEdited: text = " << text << ", url = " << url);

    Q_UNUSED(hyperlinkId);
    Q_UNUSED(startupUrlWasEmpty);

    m_newHyperlinkText = text;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    m_newHyperlinkUrl = url.toString(QUrl::FullyEncoded);
#else
    m_newHyperlinkUrl = url.toString(QUrl::None);
#endif

    QString javascript = "setHyperlinkToSelection('" + m_newHyperlinkText + "', '" + m_newHyperlinkUrl +
                         "', " + QString::number(m_hyperlinkId) + ");";

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &EditHyperlinkDelegate::onHyperlinkModified));
}

void EditHyperlinkDelegate::onHyperlinkModified(const QVariant & data)
{
    QNDEBUG("EditHyperlinkDelegate::onHyperlinkModified");
    Q_UNUSED(data);

    emit finished(m_hyperlinkId, m_originalHyperlinkText, m_originalHyperlinkUrl, m_newHyperlinkText, m_newHyperlinkUrl);
}

} // namespace qute_note
