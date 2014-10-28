#ifndef __QUTE_NOTE__GUI__ASK_USER_LOGIN_AND_PASSWORD_H
#define __QUTE_NOTE__GUI__ASK_USER_LOGIN_AND_PASSWORD_H

#include <tools/qt4helper.h>
#include <QWidget>

namespace Ui {
class AskUserNameAndPassword;
}

class AskUserNameAndPassword: public QWidget
{
    Q_OBJECT
    
public:
    explicit AskUserNameAndPassword(QWidget * parent = nullptr);
    virtual ~AskUserNameAndPassword();
    
signals:
    void userNameAndPasswordEntered(QString name, QString password);
    void cancelled(QString);

public slots:
    void onOkButtonPressed();
    void onCancelButtonPressed();

protected:
    virtual void keyPressEvent(QKeyEvent * pEvent) Q_DECL_OVERRIDE;

private:
    Ui::AskUserNameAndPassword * m_pUI;
};

#endif // __QUTE_NOTE__GUI__ASK_USER_LOGIN_AND_PASSWORD_H
