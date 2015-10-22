#include "EncryptionDialog.h"
#include "ui_EncryptionDialog.h"

namespace qute_note {

EncryptionDialog::EncryptionDialog(QWidget * parent) :
    QDialog(parent),
    m_pUI(new Ui::EncryptionDialog)
{
    m_pUI->setupUi(this);
}

} // namespace qute_note
