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
                                                         QSharedPointer<DecryptedTextManager> decryptedTextManager) :
    QObject(pNoteEditor),
    m_pNoteEditor(pNoteEditor),
    m_encryptionManager(encryptionManager),
    m_decryptedTextManager(decryptedTextManager),
    m_encryptedTextHtml(),
    m_selectionHtml(),
    m_encryptedText(),
    m_cipher(),
    m_keyLength(),
    m_hint(),
    m_rememberForSession(false)
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

    Q_UNUSED(selectedText)

    m_rememberForSession = rememberForSession;

    if (m_rememberForSession)
    {
        m_encryptedText = encryptedText;
        ENMLConverter::escapeString(m_encryptedText);

        m_cipher = cipher;
        ENMLConverter::escapeString(m_cipher);

        m_keyLength = QString::number(keyLength);

        m_hint = hint;
        ENMLConverter::escapeString(m_hint);
    }
    else
    {
        m_encryptedTextHtml = ENMLConverter::encryptedTextHtml(encryptedText, hint, cipher, keyLength, m_pNoteEditor->GetFreeEncryptedTextId());
        ENMLConverter::escapeString(m_encryptedTextHtml);
    }

    if (m_pNoteEditor->isModified()) {
        QObject::connect(m_pNoteEditor.data(), QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
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

    QObject::disconnect(m_pNoteEditor.data(), QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageConvertedToNote,Note));

    encryptSelectedText();
}

void EncryptSelectedTextDelegate::encryptSelectedText()
{
    QNDEBUG("EncryptSelectedTextDelegate::encryptSelectedText");

    GET_PAGE()

    QString javascript;
    if (m_rememberForSession)
    {
        QString id = QString::number(m_pNoteEditor->GetFreeDecryptedTextId());
        QString escapedDecryptedText = m_selectionHtml;
        ENMLConverter::escapeString(escapedDecryptedText);
        javascript = "encryptDecryptManager.replaceSelectionWithDecryptedText('" + id +
                     "', '" + escapedDecryptedText + "', '" + m_encryptedText + "', '" + m_hint +
                     "', '" + m_cipher + "', '" + m_keyLength + "');";
    }
    else
    {
        javascript = QString("encryptDecryptManager.encryptSelectedText('%1');").arg(m_encryptedTextHtml);
    }

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
