#include "AddHyperlinkToSelectedTextDelegate.h"
#include "ParsePageScrollData.h"
#include "../NoteEditor_p.h"
#include "../dialogs/EditHyperlinkDialog.h"
#include <quentier/utility/FileIOThreadWorker.h>
#include <quentier/logging/QuentierLogger.h>
#include <QScopedPointer>

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't add hyperlink to selected text: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

AddHyperlinkToSelectedTextDelegate::AddHyperlinkToSelectedTextDelegate(NoteEditorPrivate & noteEditor, const quint64 hyperlinkIdToAdd) :
    QObject(&noteEditor),
    m_noteEditor(noteEditor),
    m_shouldGetHyperlinkFromDialog(true),
    m_presetHyperlink(),
    m_initialTextWasEmpty(false),
    m_hyperlinkId(hyperlinkIdToAdd)
{}

void AddHyperlinkToSelectedTextDelegate::start()
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::start");

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        addHyperlinkToSelectedText();
    }
}

void AddHyperlinkToSelectedTextDelegate::startWithPresetHyperlink(const QString & presetHyperlink)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::startWithPresetHyperlink: preset hyperlink = " << presetHyperlink);

    m_shouldGetHyperlinkFromDialog = false;
    m_presetHyperlink = presetHyperlink;

    start();
}

void AddHyperlinkToSelectedTextDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onOriginalPageConvertedToNote,Note));

    addHyperlinkToSelectedText();
}

void AddHyperlinkToSelectedTextDelegate::addHyperlinkToSelectedText()
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::addHyperlinkToSelectedText");

    QString javascript = "getSelectionHtml();";
    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &AddHyperlinkToSelectedTextDelegate::onInitialHyperlinkDataReceived));
}

void AddHyperlinkToSelectedTextDelegate::onInitialHyperlinkDataReceived(const QVariant & data)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onInitialHyperlinkDataReceived: " << data);

    QString initialText = data.toString();
    m_initialTextWasEmpty = initialText.isEmpty();

    if (m_shouldGetHyperlinkFromDialog) {
        raiseAddHyperlinkDialog(initialText);
    }
    else {
        setHyperlinkToSelection(m_presetHyperlink, initialText);
    }
}

void AddHyperlinkToSelectedTextDelegate::raiseAddHyperlinkDialog(const QString & initialText)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::raiseAddHyperlinkDialog: initial text = " << initialText);

    QScopedPointer<EditHyperlinkDialog> pEditHyperlinkDialog(new EditHyperlinkDialog(&m_noteEditor, initialText));
    pEditHyperlinkDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pEditHyperlinkDialog.data(), QNSIGNAL(EditHyperlinkDialog,accepted,QString,QUrl,quint64,bool),
                     this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onAddHyperlinkDialogFinished,QString,QUrl,quint64,bool));
    QNTRACE("Will exec add hyperlink dialog now");
    int res = pEditHyperlinkDialog->exec();
    if (res == QDialog::Rejected) {
        QNTRACE("Cancelled add hyperlink dialog");
        emit cancelled();
    }
}

void AddHyperlinkToSelectedTextDelegate::onAddHyperlinkDialogFinished(QString text, QUrl url, quint64 hyperlinkId, bool startupUrlWasEmpty)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onAddHyperlinkDialogFinished: text = " << text << ", url = " << url);

    Q_UNUSED(hyperlinkId);
    Q_UNUSED(startupUrlWasEmpty);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString urlString = url.toString(QUrl::FullyEncoded);
#else
    QString urlString = url.toString(QUrl::None);
#endif

    setHyperlinkToSelection(urlString, text);
}

void AddHyperlinkToSelectedTextDelegate::setHyperlinkToSelection(const QString & url, const QString & text)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::setHyperlinkToSelection: url = " << url << ", text = " << text);

    QString javascript = "hyperlinkManager.setHyperlinkToSelection('" + text + "', '" + url +
                         "', " + QString::number(m_hyperlinkId) + ");";

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &AddHyperlinkToSelectedTextDelegate::onHyperlinkSetToSelection));
}

void AddHyperlinkToSelectedTextDelegate::onHyperlinkSetToSelection(const QVariant & data)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onHyperlinkSetToSelection");

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of the attempt to set the hyperlink to selection from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of the attempt to set the hyperlink to selection from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't set the hyperlink to selection: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit finished();
}

} // namespace quentier
