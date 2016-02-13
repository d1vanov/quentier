#include "EncryptSelectedTextDelegate.h"
#include "../NoteEditor_p.h"
#include "../dialogs/EncryptionDialog.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

#define CHECK_NOTE_EDITOR() \
    if (Q_UNLIKELY(m_pNoteEditor.isNull())) { \
        QNDEBUG("Note editor is null"); \
        return; \
    }

#define GET_PAGE() \
    CHECK_NOTE_EDITOR() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_pNoteEditor->page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't encrypt the selected text: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

EncryptSelectedTextDelegate::EncryptSelectedTextDelegate(NoteEditorPrivate * pNoteEditor,
                                                         QSharedPointer<EncryptionManager> encryptionManager,
                                                         QSharedPointer<DecryptedTextManager> decryptedTextManager,
                                                         const quint64 encryptedTextId) :
    QObject(pNoteEditor),
    m_pNoteEditor(pNoteEditor),
    m_encryptionManager(encryptionManager),
    m_decryptedTextManager(decryptedTextManager),
    m_encryptedTextId(encryptedTextId),
    m_selectionHtml(),
    m_encryptedTextHtml()
{}

void EncryptSelectedTextDelegate::start(const QString & selectionHtml)
{
    QNDEBUG("EncryptSelectedTextDelegate::start: selection html = " << selectionHtml);

    CHECK_NOTE_EDITOR()

    if (selectionHtml.isEmpty()) {
        QNDEBUG("No selection html, nothing to encrypt");
        emit cancelled();
        return;
    }

    m_selectionHtml = selectionHtml;

    raiseEncryptionDialog();
}

void EncryptSelectedTextDelegate::raiseEncryptionDialog()
{
    QNDEBUG("EncryptSelectedTextDelegate::raiseEncryptionDialog");

    QScopedPointer<EncryptionDialog> pEncryptionDialog(new EncryptionDialog(m_selectionHtml, m_encryptionManager,
                                                                            m_decryptedTextManager, m_pNoteEditor));
    pEncryptionDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pEncryptionDialog.data(), QNSIGNAL(EncryptionDialog,accepted,QString,QString,QString,size_t,QString,bool),
                     this, QNSLOT(EncryptSelectedTextDelegate,onSelectedTextEncrypted,QString,QString,QString,size_t,QString,bool));
    int res = pEncryptionDialog->exec();
    QNTRACE("Executed encryption dialog: " << (res == QDialog::Accepted ? "accepted" : "rejected"));
    if (res == QDialog::Rejected) {
        emit cancelled();
        return;
    }
}

void EncryptSelectedTextDelegate::onSelectedTextEncrypted(QString selectedText, QString encryptedText,
                                                          QString cipher, size_t keyLength, QString hint, bool rememberForSession)
{
    QNDEBUG("EncryptSelectedTextDelegate::onSelectedTextEncrypted: "
            << "encrypted text = " << encryptedText << ", hint = " << hint
            << ", remember for session = " << (rememberForSession ? "true" : "false"));

    CHECK_NOTE_EDITOR()

    m_encryptedTextHtml = (rememberForSession
                           ? ENMLConverter::decryptedTextHtml(selectedText, encryptedText, hint, cipher, keyLength, m_encryptedTextId)
                           : ENMLConverter::encryptedTextHtml(encryptedText, hint, cipher, keyLength, m_encryptedTextId));

    ENMLConverter::escapeString(m_encryptedTextHtml);

    if (rememberForSession) {
        // NOTE: workarounding both QtWebKit and QWebEngine always omitting the div element from the html inserted with
        // execCommand & insertHTML
        m_encryptedTextHtml.prepend("<br>");
        m_encryptedTextHtml.append("<br>");
    }

    if (m_pNoteEditor->isModified()) {
        QObject::connect(m_pNoteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageConvertedToNote,Note));
        m_pNoteEditor->convertToNote();
    }
    else {
        encryptSelectedText();
    }
}

void EncryptSelectedTextDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("EncryptSelectedTextDelegate::onOriginalPageConvertedToNote");

    CHECK_NOTE_EDITOR()

    Q_UNUSED(note)

    QObject::disconnect(m_pNoteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageConvertedToNote,Note));

    encryptSelectedText();
}

void EncryptSelectedTextDelegate::encryptSelectedText()
{
    QNDEBUG("EncryptSelectedTextDelegate::encryptSelectedText");

    QString javascript = QString("encryptDecryptManager.encryptSelectedText('%1');").arg(m_encryptedTextHtml);
    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &EncryptSelectedTextDelegate::onEncryptionScriptDone));
}

void EncryptSelectedTextDelegate::onEncryptionScriptDone(const QVariant & data)
{
    QNDEBUG("EncryptSelectedTextDelegate::onEncryptionScriptDone: " << data);

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of text encryption script from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of text encryption from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't encrypt the selected text: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit finished();
}

} // namespace qute_note
