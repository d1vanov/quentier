#include "RenameResourceDialog.h"
#include "ui_RenameResourceDialog.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

RenameResourceDialog::RenameResourceDialog(const QString & initialResourceName,
                                           QWidget * parent) :
    QDialog(parent),
    m_pUI(new Ui::RenameResourceDialog)
{
    m_pUI->setupUi(this);
    m_pUI->lineEdit->setText(initialResourceName);
}

RenameResourceDialog::~RenameResourceDialog()
{
    delete m_pUI;
}

void RenameResourceDialog::accept()
{
    QNDEBUG("RenameResourceDialog::accept");
    emit accepted(m_pUI->lineEdit->text());
    QDialog::accept();
}

} // namespace quentier


