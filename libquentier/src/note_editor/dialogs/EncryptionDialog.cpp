#include "EncryptionDialog.h"
#include "ui_EncryptionDialog.h"
#include <quentier/note_editor/DecryptedTextManager.h>
#include <quentier/utility/QuentierCheckPtr.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <QLineEdit>

namespace quentier {

EncryptionDialog::EncryptionDialog(const QString & textToEncrypt,
                                   QSharedPointer<EncryptionManager> encryptionManager,
                                   QSharedPointer<DecryptedTextManager> decryptedTextManager,
                                   QWidget * parent) :
    QDialog(parent),
    m_pUI(new Ui::EncryptionDialog),
    m_textToEncrypt(textToEncrypt),
    m_cachedEncryptedText(),
    m_encryptionManager(encryptionManager),
    m_decryptedTextManager(decryptedTextManager)
{
    m_pUI->setupUi(this);
    QUENTIER_CHECK_PTR(encryptionManager.data())

    bool rememberPassphraseForSessionDefault = false;
    ApplicationSettings appSettings;
    QVariant rememberPassphraseForSessionSetting = appSettings.value("General/rememberPassphraseForSession");
    if (!rememberPassphraseForSessionSetting.isNull()) {
        rememberPassphraseForSessionDefault = rememberPassphraseForSessionSetting.toBool();
    }

    setRememberPassphraseDefaultState(rememberPassphraseForSessionDefault);
    m_pUI->onErrorTextLabel->setVisible(false);

    QObject::connect(m_pUI->rememberPasswordForSessionCheckBox, QNSIGNAL(QCheckBox,stateChanged,int),
                     this, QNSLOT(EncryptionDialog,onRememberPassphraseStateChanged,int));
}

EncryptionDialog::~EncryptionDialog()
{
    delete m_pUI;
}

QString EncryptionDialog::passphrase() const
{
    return m_pUI->encryptionPasswordLineEdit->text();
}

bool EncryptionDialog::rememberPassphrase() const
{
    return m_pUI->rememberPasswordForSessionCheckBox->isChecked();
}

QString EncryptionDialog::encryptedText() const
{
    return m_cachedEncryptedText;
}

QString EncryptionDialog::hint() const
{
    return m_pUI->hintLineEdit->text();
}

void EncryptionDialog::setRememberPassphraseDefaultState(const bool checked)
{
    m_pUI->rememberPasswordForSessionCheckBox->setChecked(checked);
}

void EncryptionDialog::onRememberPassphraseStateChanged(int checked)
{
    Q_UNUSED(checked)

    ApplicationSettings appSettings;
    if (!appSettings.isWritable()) {
        QNINFO("Can't persist remember passphrase for session setting: settings are not writable");
    }
    else {
        appSettings.setValue("General/rememberPassphraseForSession",
                             QVariant(m_pUI->rememberPasswordForSessionCheckBox->isChecked()));
    }
}

void EncryptionDialog::accept()
{
    QString passphrase = m_pUI->encryptionPasswordLineEdit->text();
    QString repeatedPassphrase = m_pUI->repeatEncryptionPasswordLineEdit->text();

    if (passphrase.isEmpty()) {
        QNINFO("Attempted to press OK in EncryptionDialog without having a password set");
        QString error = tr("Please choose the encryption password");
        setError(error);
        return;
    }

    if (passphrase != repeatedPassphrase) {
        QString error = QT_TR_NOOP("Can't encrypt: password and repeated password do not match");
        QNINFO(error);
        setError(error);
        return;
    }

    m_cachedEncryptedText.resize(0);
    QString errorDescription;
    QString cipher = "AES";
    size_t keyLength = 128;
    bool res = m_encryptionManager->encrypt(m_textToEncrypt, passphrase, cipher, keyLength,
                                            m_cachedEncryptedText, errorDescription);
    if (!res) {
        errorDescription.prepend(QT_TR_NOOP("Can't encrypt: "));
        QNINFO(errorDescription);
        setError(errorDescription);
        return;
    }

    bool rememberForSession = m_pUI->rememberPasswordForSessionCheckBox->isChecked();

    m_decryptedTextManager->addEntry(m_cachedEncryptedText, m_textToEncrypt, rememberForSession,
                                     passphrase, cipher, keyLength);

    emit accepted(m_textToEncrypt, m_cachedEncryptedText, cipher, keyLength,
                  m_pUI->hintLineEdit->text(), rememberForSession);
    QDialog::accept();
}

void EncryptionDialog::setError(const QString & error)
{
    m_pUI->onErrorTextLabel->setText(error);
    m_pUI->onErrorTextLabel->setVisible(true);
}

} // namespace quentier
