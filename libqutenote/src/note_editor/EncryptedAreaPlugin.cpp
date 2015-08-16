#include "EncryptedAreaPlugin.h"
#include "ui_EncryptedAreaPlugin.h"
#include "NoteDecryptionDialog.h"
#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/types/IResource.h>
#include <qute_note/utility/QuteNoteCheckPtr.h>
#include <QIcon>
#include <QMouseEvent>

namespace qute_note {

EncryptedAreaPlugin::EncryptedAreaPlugin(QSharedPointer<EncryptionManager> encryptionManager,
                                         DecryptedTextCachePtr decryptedTextCache,
                                         QWidget * parent) :
    INoteEditorPlugin(parent),
    m_pUI(new Ui::EncryptedAreaPlugin),
    m_encryptionManager(encryptionManager),
    m_decryptedTextCache(decryptedTextCache),
    m_hint(),
    m_cipher(),
    m_keyLength(0)
{
    m_pUI->setupUi(this);

    QUTE_NOTE_CHECK_PTR(m_encryptionManager.data())
    QUTE_NOTE_CHECK_PTR(m_decryptedTextCache.data())

    if (!QIcon::hasThemeIcon("security-high")) {
        QIcon lockIcon;
        lockIcon.addFile(":/encrypted_area_icons/png/lock-16x16", QSize(16, 16));
        lockIcon.addFile(":/encrypted_area_icons/png/lock-24x24", QSize(24, 24));
        lockIcon.addFile(":/encrypted_area_icons/png/lock-32x32", QSize(32, 32));
        lockIcon.addFile(":/encrypted_area_icons/png/lock-48x48", QSize(48, 48));
        m_pUI->iconPushButton->setIcon(lockIcon);
    }

    QAction * showEncryptedTextAction = new QAction(this);
    showEncryptedTextAction->setText(QObject::tr("Show encrypted text") + "...");
    QObject::connect(showEncryptedTextAction, QNSIGNAL(QAction,triggered), this, QNSLOT(EncryptedAreaPlugin,decrypt));
    m_pUI->toolButton->addAction(showEncryptedTextAction);

    QObject::connect(m_pUI->iconPushButton, QNSIGNAL(QPushButton,released), this, QNSLOT(EncryptedAreaPlugin,decrypt));
}

EncryptedAreaPlugin::~EncryptedAreaPlugin()
{
    delete m_pUI;
}

EncryptedAreaPlugin * EncryptedAreaPlugin::clone() const
{
    return new EncryptedAreaPlugin(m_encryptionManager, m_decryptedTextCache,
                                   qobject_cast<QWidget*>(parent()));
}

bool EncryptedAreaPlugin::initialize(const QString & mimeType, const QUrl & url,
                                     const QStringList & parameterNames,
                                     const QStringList & parameterValues,
                                     const NoteEditorPluginFactory & pluginFactory,
                                     const IResource * resource, QString & errorDescription)
{
    QNDEBUG("EncryptedAreaPlugin::initialize: mime type = " << mimeType
            << ", url = " << url.toString() << ", parameter names = " << parameterNames.join(", ")
            << ", parameter values = " << parameterValues.join(", "));

    Q_UNUSED(resource);

    const int numParameterValues = parameterValues.size();

    int cipherIndex = parameterNames.indexOf("cipher");
    if (numParameterValues <= cipherIndex) {
        errorDescription = QT_TR_NOOP("no value was found for cipher attribute");
        return false;
    }

    int encryptedTextIndex = parameterNames.indexOf("encryptedText");
    if (encryptedTextIndex < 0) {
        errorDescription = QT_TR_NOOP("encrypted text parameter was not found within object with encrypted text");
        return false;
    }

    int keyLengthIndex = parameterNames.indexOf("length");
    if (numParameterValues <= keyLengthIndex) {
        errorDescription = QT_TR_NOOP("no value was found for length attribute");
        return false;
    }

    if (keyLengthIndex >= 0)
    {
        QString keyLengthString = parameterValues[keyLengthIndex];
        bool conversionResult = false;
        int keyLength = keyLengthString.toInt(&conversionResult);

        if (!conversionResult) {
            errorDescription = QT_TR_NOOP("can't extract integer value from length attribute: ") + keyLengthString;
            return false;
        }
        else if (keyLength < 0) {
            errorDescription = QT_TR_NOOP("key length is negative: ") + keyLengthString;
            return false;
        }

        m_keyLength = static_cast<size_t>(keyLength);
    }
    else
    {
        m_keyLength = 128;
        QNDEBUG("Using the default value of key length = " << m_keyLength << " instead of missing HTML attribute");
    }

    if (cipherIndex >= 0) {
        m_cipher = parameterValues[cipherIndex];
    }
    else {
        m_cipher = "AES";
        QNDEBUG("Using the default value of cipher = " << m_cipher << " instead of missing HTML attribute");
    }

    m_encryptedText = parameterValues[encryptedTextIndex];

    int hintIndex = parameterNames.indexOf("hint");
    if ((hintIndex < 0) || (numParameterValues <= hintIndex)) {
        m_hint.clear();
    }
    else {
        m_hint = parameterValues[hintIndex];
    }

    const NoteEditor & noteEditor = pluginFactory.noteEditor();
    QObject::connect(this, QNSIGNAL(EncryptedAreaPlugin,decrypted,QString,QString,bool),
                     &noteEditor, QNSLOT(NoteEditor,onEncryptedAreaDecryption,QString,QString,bool));

    QNTRACE("Initialized encrypted area plugin: cipher = " << m_cipher
            << ", length = " << m_keyLength << ", hint = " << m_hint
            << ", encrypted text = " << m_encryptedText);
    return true;
}

QStringList EncryptedAreaPlugin::mimeTypes() const
{
    return QStringList();
}

QHash<QString, QStringList> EncryptedAreaPlugin::fileExtensions() const
{
    return QHash<QString, QStringList>();
}

QList<QPair<QString, QString> > EncryptedAreaPlugin::specificParameters() const
{
    return QList<QPair<QString, QString> >() << QPair<QString, QString>("en-tag", "en-crypt");
}

QString EncryptedAreaPlugin::name() const
{
    return "EncryptedAreaPlugin";
}

QString EncryptedAreaPlugin::description() const
{
    return QObject::tr("Encrypted area plugin - note editor plugin used for the display and convenient work "
                       "with encrypted text within notes");
}

void EncryptedAreaPlugin::decrypt()
{
    raiseNoteDecryptionDialog();
}

void EncryptedAreaPlugin::raiseNoteDecryptionDialog()
{
    QScopedPointer<NoteDecryptionDialog> pDecryptionDialog(new NoteDecryptionDialog(m_encryptedText,
                                                                                    m_cipher, m_hint,
                                                                                    m_keyLength,
                                                                                    m_encryptionManager,
                                                                                    m_decryptedTextCache,
                                                                                    this));
    pDecryptionDialog->setWindowModality(Qt::WindowModal);

    int res = pDecryptionDialog->exec();
    if (res == QDialog::Accepted) {
        QNTRACE("Successfully decrypted text: " << pDecryptionDialog->decryptedText());
        emit decrypted(m_encryptedText, pDecryptionDialog->decryptedText(),
                       pDecryptionDialog->rememberPassphrase());
    }
}

} // namespace qute_note
