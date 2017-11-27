#include "WelcomeToQuentierDialog.h"
#include "ui_WelcomeToQuentierDialog.h"

namespace quentier {

WelcomeToQuentierDialog::WelcomeToQuentierDialog(QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::WelcomeToQuentierDialog)
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Welcome to Quentier"));
}

WelcomeToQuentierDialog::~WelcomeToQuentierDialog()
{
    delete m_pUi;
}

} // namespace quentier
