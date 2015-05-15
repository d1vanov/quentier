#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H

#include "INoteEditorPlugin.h"
#include <tools/qt4helper.h>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(EncryptedAreaPlugin)
}

namespace qute_note {

class EncryptedAreaPlugin: public INoteEditorPlugin
{
    Q_OBJECT
public:
    explicit EncryptedAreaPlugin(QWidget * parent = nullptr);
    virtual ~EncryptedAreaPlugin();

Q_SIGNALS:
    void rememberPassphraseForSession(QString cipher, QString password);

private:
    // INoteEditorPlugin interface
    virtual EncryptedAreaPlugin * clone() const Q_DECL_OVERRIDE;
    virtual bool initialize(const QString & mimeType, const QUrl & url,
                            const QStringList & parameterNames,
                            const QStringList & parameterValues,
                            const IResource * resource, QString & errorDescription) Q_DECL_OVERRIDE;
    virtual QStringList mimeTypes() const Q_DECL_OVERRIDE;
    virtual QHash<QString, QStringList> fileExtensions() const Q_DECL_OVERRIDE;
    virtual QStringList specificAttributes() const Q_DECL_OVERRIDE;
    virtual QString name() const Q_DECL_OVERRIDE;
    virtual QString description() const Q_DECL_OVERRIDE;

private:
    virtual void mouseReleaseEvent(QMouseEvent * mouseEvent) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void decrypt();
    void decryptAndRemember();

private:
    void raiseNoteDecryptionDialog(const bool shouldRememberPassphrase);

private:
    Ui::EncryptedAreaPlugin *   m_pUI;
    QString                     m_hint;
    QString                     m_cipher;
    size_t                      m_keyLength;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H
