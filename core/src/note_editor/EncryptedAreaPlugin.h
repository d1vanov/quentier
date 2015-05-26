#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H

#include "INoteEditorPlugin.h"
#include <tools/qt4helper.h>
#include <tools/EncryptionManager.h>
#include <QSharedPointer>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(EncryptedAreaPlugin)
}

namespace qute_note {

class EncryptedAreaPlugin: public INoteEditorPlugin
{
    Q_OBJECT
public:
    explicit EncryptedAreaPlugin(const QSharedPointer<EncryptionManager> & encryptionManager,
                                 QWidget * parent = nullptr);
    virtual ~EncryptedAreaPlugin();

Q_SIGNALS:
    void rememberPassphraseForSession(QString cipher, QString password, bool remember);
    void notifyDecryptionError(QString errorDescription);

private:
    // INoteEditorPlugin interface
    virtual EncryptedAreaPlugin * clone() const Q_DECL_OVERRIDE;
    virtual bool initialize(const QString & mimeType, const QUrl & url,
                            const QStringList & parameterNames,
                            const QStringList & parameterValues,
                            const IResource * resource, QString & errorDescription) Q_DECL_OVERRIDE;
    virtual QStringList mimeTypes() const Q_DECL_OVERRIDE;
    virtual QHash<QString, QStringList> fileExtensions() const Q_DECL_OVERRIDE;
    virtual QList<QPair<QString, QString> > specificParameters() const Q_DECL_OVERRIDE;
    virtual QString name() const Q_DECL_OVERRIDE;
    virtual QString description() const Q_DECL_OVERRIDE;

private:
    virtual void mouseReleaseEvent(QMouseEvent * mouseEvent) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void decrypt();

private:
    void raiseNoteDecryptionDialog();

private:
    Ui::EncryptedAreaPlugin *           m_pUI;
    QSharedPointer<EncryptionManager>   m_encryptionManager;
    QString                             m_hint;
    QString                             m_cipher;
    QString                             m_encryptedText;
    size_t                              m_keyLength;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__ENCRYPTED_AREA_PLUGIN_H
