#ifndef QUENTIER_DIALOGS_WELCOME_TO_QUENTIER_DIALOG_H
#define QUENTIER_DIALOGS_WELCOME_TO_QUENTIER_DIALOG_H

#include <quentier/utility/Macros.h>
#include <QDialog>

namespace Ui {
class WelcomeToQuentierDialog;
}

namespace quentier {

class WelcomeToQuentierDialog: public QDialog
{
    Q_OBJECT
public:
    explicit WelcomeToQuentierDialog(QWidget * parent = Q_NULLPTR);
    ~WelcomeToQuentierDialog();

private:
    Ui::WelcomeToQuentierDialog *   m_pUi;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_WELCOME_TO_QUENTIER_DIALOG_H
