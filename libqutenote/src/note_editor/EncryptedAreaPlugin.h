#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H

#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <qute_note/utility/EncryptionManager.h>
#include <QWidget>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(EncryptedAreaPlugin)
}

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(NoteEditorPluginFactory)

class EncryptedAreaPlugin: public QWidget
{
    Q_OBJECT
public:
    explicit EncryptedAreaPlugin(NoteEditorPrivate & noteEditor,
                                 QSharedPointer<EncryptionManager> encryptionManager,
                                 DecryptedTextManager & decryptedTextManager,
                                 QWidget * parent = Q_NULLPTR);
    virtual ~EncryptedAreaPlugin();

    bool initialize(const QStringList & parameterNames, const QStringList & parameterValues,
                            const NoteEditorPluginFactory & pluginFactory, QString & errorDescription);
    QString name() const;
    QString description() const;

Q_SIGNALS:
    void decrypted(QString cipher, size_t keyLength, QString encryptedText, QString passphrase,
                   QString decryptedText, bool rememberForSession, bool decryptPermanently,
                   bool createDecryptUndoAction = true);

private Q_SLOTS:
    void decrypt();

private:
    void raiseNoteDecryptionDialog();

private:
    Ui::EncryptedAreaPlugin *           m_pUI;
    NoteEditorPrivate &                 m_noteEditor;
    QSharedPointer<EncryptionManager>   m_encryptionManager;
    DecryptedTextManager &              m_decryptedTextManager;
    QString                             m_hint;
    QString                             m_cipher;
    QString                             m_encryptedText;
    size_t                              m_keyLength;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H
