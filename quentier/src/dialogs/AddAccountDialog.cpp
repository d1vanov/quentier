#include "AddAccountDialog.h"
#include "ui_AddAccountDialog.h"

AddAccountDialog::AddAccountDialog(QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::AddAccountDialog)
{
    m_pUi->setupUi(this);
}

AddAccountDialog::~AddAccountDialog()
{
    delete m_pUi;
}
