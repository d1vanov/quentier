#ifndef QUENTIER_DIALOGS_ADD_ACCOUNT_DIALOG_H
#define QUENTIER_DIALOGS_ADD_ACCOUNT_DIALOG_H

#include <quentier/utility/Qt4Helper.h>
#include <QDialog>

namespace Ui {
class AddAccountDialog;
}

class AddAccountDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddAccountDialog(QWidget * parent = Q_NULLPTR);
    ~AddAccountDialog();

private:
    Ui::AddAccountDialog * m_pUi;
};

#endif // QUENTIER_DIALOGS_ADD_ACCOUNT_DIALOG_H
