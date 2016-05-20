#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_ENCRYPTED_AREA_PLUGIN_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_ENCRYPTED_AREA_PLUGIN_H

#include <qute_note/utility/Qt4Helper.h>
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
    explicit EncryptedAreaPlugin(NoteEditorPrivate & noteEditor, QWidget * parent = Q_NULLPTR);
    virtual ~EncryptedAreaPlugin();

    bool initialize(const QStringList & parameterNames, const QStringList & parameterValues,
                    const NoteEditorPluginFactory & pluginFactory, QString & errorDescription);
    QString name() const;
    QString description() const;

private Q_SLOTS:
    void decrypt();

private:
    Ui::EncryptedAreaPlugin *           m_pUI;
    NoteEditorPrivate &                 m_noteEditor;
    QString                             m_hint;
    QString                             m_cipher;
    QString                             m_encryptedText;
    QString                             m_keyLength;
    QString                             m_id;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_ENCRYPTED_AREA_PLUGIN_H
