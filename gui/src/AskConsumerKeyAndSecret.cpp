#include "AskConsumerKeyAndSecret.h"
#include "ui_AskConsumerKeyAndSecret.h"
#include <QMessageBox>
#include <QKeyEvent>

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

    QWidget::setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
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
    QWidget::close();
}

void AskConsumerKeyAndSecret::onCancelButtonPressed()
{
    emit cancelled(QString(tr("User didn't provide consumer key and secret for application")));
    QWidget::close();
}

void AskConsumerKeyAndSecret::keyPressEvent(QKeyEvent * pEvent)
{
    if (!pEvent) {
        return;
    }

    int key = pEvent->key();
    if ((key == Qt::Key_Enter) || (key == Qt::Key_Return)) {
        onOkButtonPressed();
    }
    else if (key == Qt::Key_Escape) {
        onCancelButtonPressed();
    }
}
