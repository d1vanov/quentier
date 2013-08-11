#include "AskConsumerKeyAndSecret.h"
#include "ui_AskConsumerKeyAndSecret.h"
#include <QMessageBox>

AskConsumerKeyAndSecret::AskConsumerKeyAndSecret(QWidget * parent) :
    QWidget(parent),
    m_pUI(new Ui::AskConsumerKeyAndSecret)
{
    Q_CHECK_PTR(m_pUI);
    m_pUI->setupUi(this);

    QObject::connect(m_pUI->okButton, SIGNAL(clicked()),
                     this, SLOT(onOkButtonPressed()));
    QObject::connect(m_pUI->cancelButton, SIGNAL(clicked()),
                     this, SLOT(onCancelButtonPressed()));

    if (parent == nullptr) {
        QWidget::setAttribute(Qt::WA_DeleteOnClose);
    }
}

AskConsumerKeyAndSecret::~AskConsumerKeyAndSecret()
{
    if (m_pUI != nullptr) {
        delete m_pUI;
        m_pUI = nullptr;
    }
}

void AskConsumerKeyAndSecret::onOkButtonPressed()
{
    QString key = m_pUI->consumerKeyEnterForm->text();
    QString secret = m_pUI->consumerSecretEnterForm->text();

    if (key.isEmpty()) {
        QMessageBox::information(0, tr("Error"),
                                 tr("Consumer key is empty, please enter a valid key"));
        return;
    }

    if (secret.isEmpty()) {
        QMessageBox::information(0, tr("Error"),
                                 tr("Consumer secret is empty, please enter a valid secret"));
        return;
    }

    emit consumerKeyAndSecretEntered(key, secret);
}

void AskConsumerKeyAndSecret::onCancelButtonPressed()
{
    emit cancelled("User didn't provide consumer key and secret for application");
    QWidget::close();
}
