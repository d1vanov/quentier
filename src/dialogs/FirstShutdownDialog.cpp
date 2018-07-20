#include "FirstShutdownDialog.h"
#include "ui_FirstShutdownDialog.h"

namespace quentier {

FirstShutdownDialog::FirstShutdownDialog(QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::FirstShutdownDialog)
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Keep the app running or quit?"));

    QObject::connect(m_pUi->closeToTrayPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(FirstShutdownDialog,onCloseToTrayPushButtonPressed));
    QObject::connect(m_pUi->closePushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(FirstShutdownDialog,onClosePushButtonPressed));
}

FirstShutdownDialog::~FirstShutdownDialog()
{
    delete m_pUi;
}

void FirstShutdownDialog::onCloseToTrayPushButtonPressed()
{
    QDialog::accept();
}

void FirstShutdownDialog::onClosePushButtonPressed()
{
    QDialog::reject();
}

} // namespace quentier
