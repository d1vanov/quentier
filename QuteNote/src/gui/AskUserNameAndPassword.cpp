#include "AskUserNameAndPassword.h"
#include "ui_AskUserNameAndPassword.h"
#include <QMessageBox>

AskUserNameAndPassword::AskUserNameAndPassword(QWidget * parent) :
    QWidget(parent),
    m_pUI(new Ui::AskUserNameAndPassword)
{
    m_pUI->setupUi(this);

    QObject::connect(m_pUI->okButton, SIGNAL(clicked()),
                     this, SLOT(onOkButtonPressed()));
    QObject::connect(m_pUI->cancelButton, SIGNAL(clicked()),
                     this, SLOT(onCancelButtonPressed()));

    if (parent == nullptr) {
        QWidget::setAttribute(Qt::WA_DeleteOnClose);
    }
    QWidget::setWindowFlags(Qt::Window);
}

AskUserNameAndPassword::~AskUserNameAndPassword()
{
    if (m_pUI != nullptr) {
        delete m_pUI;
        m_pUI = nullptr;
    }
}

void AskUserNameAndPassword::onOkButtonPressed()
{
    QString name = m_pUI->userNameEnterForm->text();
    QString password = m_pUI->userPasswordEnterForm->text();

    if (name.isEmpty()) {
        QMessageBox::information(0, tr("Error"),
                                 tr("User name is empty, please enter a valid user name"));
        return;
    }

    if (password.isEmpty()) {
        QMessageBox::information(0, tr("Error"),
                                 tr("Passord is empty, please enter a valid password"));
        return;
    }

    emit userNameAndPasswordEntered(name, password);
}

void AskUserNameAndPassword::onCancelButtonPressed()
{
    emit cancelled(QString(tr("User didn't provide user name and password for authentication")));
    QWidget::close();
}
