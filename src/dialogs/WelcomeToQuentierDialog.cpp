#include "WelcomeToQuentierDialog.h"
#include "ui_WelcomeToQuentierDialog.h"

namespace quentier {

WelcomeToQuentierDialog::WelcomeToQuentierDialog(QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::WelcomeToQuentierDialog)
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Welcome to Quentier"));

    QObject::connect(m_pUi->continueWithLocalAccountPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(WelcomeToQuentierDialog,onContinueWithLocalAccountPushButtonPressed));
    QObject::connect(m_pUi->loginToEvernoteAccountPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(WelcomeToQuentierDialog,onLogInToEvernoteAccountPushButtonPressed));
}

WelcomeToQuentierDialog::~WelcomeToQuentierDialog()
{
    delete m_pUi;
}

void WelcomeToQuentierDialog::onContinueWithLocalAccountPushButtonPressed()
{
    QDialog::reject();
}

void WelcomeToQuentierDialog::onLogInToEvernoteAccountPushButtonPressed()
{
    QDialog::accept();
}

} // namespace quentier
