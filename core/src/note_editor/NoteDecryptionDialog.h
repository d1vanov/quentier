#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_DECRYPTION_DIALOG_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_DECRYPTION_DIALOG_H

#include <QDialog>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(NoteDecryptionDialog)
}

namespace qute_note {

class NoteDecryptionDialog: public QDialog
{
    Q_OBJECT
public:
    explicit NoteDecryptionDialog(QWidget * parent = nullptr);
    ~NoteDecryptionDialog();

    QString passphrase() const;
    bool rememberPassphrase() const;

public Q_SLOTS:
    void setHint(const QString & hint);
    void setRememberPassphraseDefaultState(const bool checked);

private:
    Ui::NoteDecryptionDialog * m_pUI;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_DECRYPTION_DIALOG_H
