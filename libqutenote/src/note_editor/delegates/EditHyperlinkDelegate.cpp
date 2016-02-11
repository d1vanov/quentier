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
    m_hyperlinkId(hyperlinkId)
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

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(EditHyperlinkDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        doStart();
    }
}

void EditHyperlinkDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("EditHyperlinkDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(EditHyperlinkDelegate,onOriginalPageConvertedToNote,Note));

    doStart();
}

void EditHyperlinkDelegate::onHyperlinkDataReceived(const QVariant & data)
{
    QNDEBUG("EditHyperlinkDelegate::onHyperlinkDataReceived: data = " << data);

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of hyperlink data request from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        QString error;

        auto errorIt = resultMap.find("error");
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error = QT_TR_NOOP("Internal error: can't parse the error of hyperlink data request from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't get hyperlink data from JavaScript: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    auto dataIt = resultMap.find("data");
    if (Q_UNLIKELY(dataIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: no hyperlink data received from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QStringList hyperlinkDataList = dataIt.value().toStringList();
    if (hyperlinkDataList.isEmpty()) {
        QString error = QT_TR_NOOP("Can't edit hyperlink: can't find hyperlink text and link");
        emit notifyError(error);
        return;
    }

    if (hyperlinkDataList.size() != 2) {
        QString error = QT_TR_NOOP("Can't edit hyperlink: can't parse hyperlink text and link");
        QNWARNING(error << "; hyperlink data: " << hyperlinkDataList.join(","));
        emit notifyError(error);
        return;
    }

    raiseEditHyperlinkDialog(hyperlinkDataList[0], hyperlinkDataList[1]);
}

void EditHyperlinkDelegate::doStart()
{
    QNDEBUG("EditHyperlinkDelegate::doStart");

    QString javascript = "hyperlinkManager.getHyperlinkData(" + QString::number(m_hyperlinkId) + ");";

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &EditHyperlinkDelegate::onHyperlinkDataReceived));
}

void EditHyperlinkDelegate::raiseEditHyperlinkDialog(const QString & startupHyperlinkText, const QString & startupHyperlinkUrl)
{
    QNDEBUG("EditHyperlinkDelegate::raiseEditHyperlinkDialog: original text = " << startupHyperlinkText
            << ", original url: " << startupHyperlinkUrl);

    QScopedPointer<EditHyperlinkDialog> pEditHyperlinkDialog(new EditHyperlinkDialog(&m_noteEditor, startupHyperlinkText, startupHyperlinkUrl));
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
    QNDEBUG("EditHyperlinkDelegate::onHyperlinkDataEdited: text = " << text << ", url = " << url << ", hyperlink id = " << hyperlinkId);

    Q_UNUSED(hyperlinkId)
    Q_UNUSED(startupUrlWasEmpty)

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString urlString = url.toString(QUrl::FullyEncoded);
#else
    QString urlString = url.toString(QUrl::None);
#endif

    QString javascript = "hyperlinkManager.setHyperlinkData('" + text + "', '" + urlString +
                         "', " + QString::number(m_hyperlinkId) + ");";

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &EditHyperlinkDelegate::onHyperlinkModified));
}

void EditHyperlinkDelegate::onHyperlinkModified(const QVariant & data)
{
    QNDEBUG("EditHyperlinkDelegate::onHyperlinkModified");

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of hyperlink edit from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        QString error;

        auto errorIt = resultMap.find("error");
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error = QT_TR_NOOP("Internal error: can't parse the error of hyperlink editing from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't edit hyperlink: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit finished();
}

} // namespace qute_note
