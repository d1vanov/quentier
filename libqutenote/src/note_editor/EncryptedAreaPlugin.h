#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H

#include "INoteEditorEncryptedAreaPlugin.h"
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/utility/EncryptionManager.h>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(EncryptedAreaPlugin)
}

namespace qute_note {

class EncryptedAreaPlugin: public INoteEditorEncryptedAreaPlugin
{
    Q_OBJECT
public:
    explicit EncryptedAreaPlugin(QSharedPointer<EncryptionManager> encryptionManager,
                                 DecryptedTextManager & decryptedTextManager,
                                 QWidget * parent = Q_NULLPTR);
    virtual ~EncryptedAreaPlugin();

Q_SIGNALS:
    void decrypted(QString cipher, size_t keyLength, QString encryptedText, QString passphrase,
                   QString decryptedText, bool rememberForSession, bool decryptPermanently,
                   bool createDecryptUndoAction = true);

private:
    // INoteEditorEncryptedAreaPlugin interface
    virtual EncryptedAreaPlugin * clone() const Q_DECL_OVERRIDE;
    virtual bool initialize(const QStringList & parameterNames, const QStringList & parameterValues,
                            const NoteEditorPluginFactory & pluginFactory, QString & errorDescription) Q_DECL_OVERRIDE;
    virtual QString name() const Q_DECL_OVERRIDE;
    virtual QString description() const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void decrypt();

private:
    void raiseNoteDecryptionDialog();

private:
    Ui::EncryptedAreaPlugin *           m_pUI;
    QSharedPointer<EncryptionManager>   m_encryptionManager;
    DecryptedTextManager &              m_decryptedTextManager;
    QString                             m_hint;
    QString                             m_cipher;
    QString                             m_encryptedText;
    size_t                              m_keyLength;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H
