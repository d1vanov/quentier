#include "NoteDecryptionDialog.h"
#include "ui_NoteDecryptionDialog.h"
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <qute_note/utility/QuteNoteCheckPtr.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/ApplicationSettings.h>

namespace qute_note {

NoteDecryptionDialog::NoteDecryptionDialog(const QString & encryptedText,
                                           const QString & cipher,
                                           const QString & hint, const size_t keyLength,
                                           QSharedPointer<EncryptionManager> encryptionManager,
                                           DecryptedTextManager & decryptedTextManager,
                                           QWidget * parent) :
    QDialog(parent),
    m_pUI(new Ui::NoteDecryptionDialog),
    m_encryptedText(encryptedText),
    m_cipher(cipher),
    m_hint(hint),
    m_cachedDecryptedText(),
    m_encryptionManager(encryptionManager),
    m_decryptedTextManager(decryptedTextManager),
    m_keyLength(keyLength)
{
    m_pUI->setupUi(this);
    QUTE_NOTE_CHECK_PTR(encryptionManager.data())

    setHint(m_hint);

    bool rememberPassphraseForSessionDefault = false;
    ApplicationSettings appSettings;
    QVariant rememberPassphraseForSessionSetting = appSettings.value("General/rememberPassphraseForSession");
    if (!rememberPassphraseForSessionSetting.isNull()) {
        rememberPassphraseForSessionDefault = rememberPassphraseForSessionSetting.toBool();
    }

    setRememberPassphraseDefaultState(rememberPassphraseForSessionDefault);
    m_pUI->onErrorTextLabel->setVisible(false);

    QObject::connect(m_pUI->showPasswordCheckBox, QNSIGNAL(QCheckBox,stateChanged,int),
                     this, QNSLOT(NoteDecryptionDialog,onShowPasswordStateChanged,int));
    QObject::connect(m_pUI->rememberPasswordCheckBox, QNSIGNAL(QCheckBox,stateChanged,int),
                     this, QNSLOT(NoteDecryptionDialog,onRememberPassphraseStateChanged,int));
}

NoteDecryptionDialog::~NoteDecryptionDialog()
{
    delete m_pUI;
}

QString NoteDecryptionDialog::passphrase() const
{
    return m_pUI->passwordLineEdit->text();
}

bool NoteDecryptionDialog::rememberPassphrase() const
{
    return m_pUI->rememberPasswordCheckBox->isChecked();
}

QString NoteDecryptionDialog::decryptedText() const
{
    return m_cachedDecryptedText;
}

void NoteDecryptionDialog::setError(const QString & error)
{
    m_pUI->onErrorTextLabel->setText(error);
    m_pUI->onErrorTextLabel->setVisible(true);
}

void NoteDecryptionDialog::setHint(const QString & hint)
{
    m_pUI->hintLabel->setText(QObject::tr("Hint: ") +
                              (hint.isEmpty() ? QObject::tr("No hint available") : hint));
}

void NoteDecryptionDialog::setRememberPassphraseDefaultState(const bool checked)
{
    m_pUI->rememberPasswordCheckBox->setChecked(checked);
}

void NoteDecryptionDialog::onRememberPassphraseStateChanged(int checked)
{
    Q_UNUSED(checked)

    ApplicationSettings appSettings;
    if (!appSettings.isWritable()) {
        QNINFO("Can't persist remember passphrase for session setting: settings are not writable");
    }
    else {
        appSettings.setValue("General/rememberPassphraseForSession",
                             QVariant(m_pUI->rememberPasswordCheckBox->isChecked()));
    }
}

void NoteDecryptionDialog::onShowPasswordStateChanged(int checked)
{
    m_pUI->passwordLineEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    m_pUI->passwordLineEdit->setFocus();
}

void NoteDecryptionDialog::accept()
{
    QString passphrase = m_pUI->passwordLineEdit->text();

    QString errorDescription;
    bool res = m_encryptionManager->decrypt(m_encryptedText, passphrase, m_cipher,
                                            m_keyLength, m_cachedDecryptedText,
                                            errorDescription);
    if (!res && (m_cipher == "AES") && (m_keyLength == 128)) {
        QNDEBUG("The initial attempt to decrypt the text using AES cipher and 128 bit key has failed; "
                "checking whether it is old encrypted text area using RC2 encryption and 64 bit key");
        res = m_encryptionManager->decrypt(m_encryptedText, passphrase, "RC2", 64,
                                           m_cachedDecryptedText, errorDescription);
    }

    if (!res) {
        errorDescription.prepend(QT_TR_NOOP("Failed attempt to decrypt text: "));
        QNINFO(errorDescription);
        setError(errorDescription);
        return;
    }

    bool rememberForSession = m_pUI->rememberPasswordCheckBox->isChecked();
    m_decryptedTextManager.addEntry(m_encryptedText, m_cachedDecryptedText, rememberForSession,
                                    passphrase, m_cipher, m_keyLength);
    QNTRACE("Cached decrypted text for encryptedText: " << m_encryptedText
            << "; remember for session = " << (rememberForSession ? "true" : "false"));

    emit accepted(m_encryptedText, m_cachedDecryptedText, m_pUI->rememberPasswordCheckBox->isChecked());
    QDialog::accept();
}

} // namespace qute_note
