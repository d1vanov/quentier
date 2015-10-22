#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTION_DIALOG_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTION_DIALOG_H

#include <qute_note/utility/Qt4Helper.h>
#include <QDialog>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(EncryptionDialog)
}

namespace qute_note {

class EncryptionDialog: public QDialog
{
    Q_OBJECT
public:
    explicit EncryptionDialog(QWidget * parent = Q_NULLPTR);

private:
    Ui::EncryptionDialog * m_pUI;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTION_DIALOG_H
