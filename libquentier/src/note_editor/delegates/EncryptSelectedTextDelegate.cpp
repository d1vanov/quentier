/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "EncryptSelectedTextDelegate.h"
#include "../NoteEditor_p.h"
#include "../dialogs/EncryptionDialog.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define CHECK_NOTE_EDITOR() \
    if (Q_UNLIKELY(m_pNoteEditor.isNull())) { \
        QNDEBUG(QStringLiteral("Note editor is null")); \
        return; \
    }

#define GET_PAGE() \
    CHECK_NOTE_EDITOR() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_pNoteEditor->page()); \
    if (Q_UNLIKELY(!page)) { \
        QNLocalizedString error = QT_TR_NOOP("can't encrypt the selected text: no note editor page"); \
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
    QNDEBUG(QStringLiteral("EncryptSelectedTextDelegate::start: selection html = ") << selectionHtml);

    CHECK_NOTE_EDITOR()

    if (Q_UNLIKELY(selectionHtml.isEmpty())) {
        QNDEBUG(QStringLiteral("No selection html, nothing to encrypt"));
        emit cancelled();
        return;
    }

    m_selectionHtml = selectionHtml;

    raiseEncryptionDialog();
}

void EncryptSelectedTextDelegate::raiseEncryptionDialog()
{
    QNDEBUG(QStringLiteral("EncryptSelectedTextDelegate::raiseEncryptionDialog"));

    QScopedPointer<EncryptionDialog> pEncryptionDialog(new EncryptionDialog(m_selectionHtml, m_encryptionManager,
                                                                            m_decryptedTextManager, m_pNoteEditor));
    pEncryptionDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pEncryptionDialog.data(), QNSIGNAL(EncryptionDialog,accepted,QString,QString,QString,size_t,QString,bool),
                     this, QNSLOT(EncryptSelectedTextDelegate,onSelectedTextEncrypted,QString,QString,QString,size_t,QString,bool));
    int res = pEncryptionDialog->exec();
    QNTRACE(QStringLiteral("Executed encryption dialog: ") << (res == QDialog::Accepted ? QStringLiteral("accepted") : QStringLiteral("rejected")));
    if (res == QDialog::Rejected) {
        emit cancelled();
        return;
    }
}

void EncryptSelectedTextDelegate::onSelectedTextEncrypted(QString selectedText, QString encryptedText,
                                                          QString cipher, size_t keyLength, QString hint, bool rememberForSession)
{
    QNDEBUG(QStringLiteral("EncryptSelectedTextDelegate::onSelectedTextEncrypted: encrypted text = ")
            << encryptedText << QStringLiteral(", hint = ") << hint << QStringLiteral(", remember for session = ")
            << (rememberForSession ? QStringLiteral("true") : QStringLiteral("false")));

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
    QNDEBUG(QStringLiteral("EncryptSelectedTextDelegate::onOriginalPageConvertedToNote"));

    CHECK_NOTE_EDITOR()

    Q_UNUSED(note)

    QObject::disconnect(m_pNoteEditor.data(), QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageConvertedToNote,Note));

    encryptSelectedText();
}

void EncryptSelectedTextDelegate::encryptSelectedText()
{
    QNDEBUG(QStringLiteral("EncryptSelectedTextDelegate::encryptSelectedText"));

    GET_PAGE()

    QString javascript;
    if (m_rememberForSession)
    {
        QString id = QString::number(m_pNoteEditor->GetFreeDecryptedTextId());
        QString escapedDecryptedText = m_selectionHtml;
        ENMLConverter::escapeString(escapedDecryptedText);
        javascript = QStringLiteral("encryptDecryptManager.replaceSelectionWithDecryptedText('") + id +
                     QStringLiteral("', '") + escapedDecryptedText + QStringLiteral("', '") + m_encryptedText +
                     QStringLiteral("', '") + m_hint + QStringLiteral("', '") + m_cipher +
                     QStringLiteral("', '") + m_keyLength + QStringLiteral("');");
    }
    else
    {
        javascript = QString("encryptDecryptManager.encryptSelectedText('%1');").arg(m_encryptedTextHtml);
    }

    page->executeJavaScript(javascript, JsCallback(*this, &EncryptSelectedTextDelegate::onEncryptionScriptDone));
}

void EncryptSelectedTextDelegate::onEncryptionScriptDone(const QVariant & data)
{
    QNDEBUG(QStringLiteral("EncryptSelectedTextDelegate::onEncryptionScriptDone: ") << data);

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QNLocalizedString error = QT_TR_NOOP("can't parse the result of text encryption script from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        QNLocalizedString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error = QT_TR_NOOP("can't parse the error of text encryption from JavaScript");
        }
        else {
            error = QT_TR_NOOP("can't encrypt the selected text");
            error += QStringLiteral(": ");
            error += errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit finished();
}

} // namespace quentier
