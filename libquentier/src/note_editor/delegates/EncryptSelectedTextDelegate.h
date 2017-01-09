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

#ifndef LIB_QUENTIER_NOTE_EDITOR_DELEGATES_ENCRYPT_SELECTED_TEXT_DELEGATE_H
#define LIB_QUENTIER_NOTE_EDITOR_DELEGATES_ENCRYPT_SELECTED_TEXT_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <quentier/utility/Macros.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/types/Note.h>
#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(EncryptionManager)
QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

/**
 * @brief The EncryptSelectedTextDelegate class encapsulates a chain of callbacks
 * required for proper implementation of currently selected text encryption
 * considering the details of wrapping this action around the undo stack
 */
class EncryptSelectedTextDelegate: public QObject
{
    Q_OBJECT
public:
    explicit EncryptSelectedTextDelegate(NoteEditorPrivate * pNoteEditor, QSharedPointer<EncryptionManager> encryptionManager,
                                         QSharedPointer<DecryptedTextManager> decryptedTextManager);
    void start(const QString & selectionHtml);

Q_SIGNALS:
    void finished();
    void cancelled();
    void notifyError(QNLocalizedString error);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onSelectedTextEncrypted(QString selectedText, QString encryptedText, QString cipher,
                                 size_t keyLength, QString hint, bool rememberForSession);
    void onEncryptionScriptDone(const QVariant & data);

private:
    void raiseEncryptionDialog();
    void encryptSelectedText();

private:
    typedef JsResultCallbackFunctor<EncryptSelectedTextDelegate> JsCallback;

private:
    QPointer<NoteEditorPrivate>             m_pNoteEditor;
    QSharedPointer<EncryptionManager>       m_encryptionManager;
    QSharedPointer<DecryptedTextManager>    m_decryptedTextManager;

    QString                                 m_encryptedTextHtml;

    QString                                 m_selectionHtml;
    QString                                 m_encryptedText;
    QString                                 m_cipher;
    QString                                 m_keyLength;
    QString                                 m_hint;
    bool                                    m_rememberForSession;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DELEGATES_ENCRYPT_SELECTED_TEXT_DELEGATE_H
