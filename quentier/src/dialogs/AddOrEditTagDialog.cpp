#include "AddOrEditTagDialog.h"
#include "ui_AddOrEditTagDialog.h"

namespace quentier {

AddOrEditTagDialog::AddOrEditTagDialog(QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::AddOrEditTagDialog)
{
    m_pUi->setupUi(this);
}

AddOrEditTagDialog::~AddOrEditTagDialog()
{
    delete m_pUi;
}

} // namespace quentier
