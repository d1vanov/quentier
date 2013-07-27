#include "AskConsumerKeyAndSecret.h"
#include "ui_AskConsumerKeyAndSecret.h"

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

    emit consumerKeyAndSecretEntered(key, secret);
}

void AskConsumerKeyAndSecret::onCancelButtonPressed()
{
    emit cancelled("User didn't provide consumer key and secret for application");
}
