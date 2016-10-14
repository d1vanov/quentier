#ifndef QUENTIER_MANAGE_ACCOUNTS_DIALOG_H
#define QUENTIER_MANAGE_ACCOUNTS_DIALOG_H

#include "AvailableAccount.h"
#include <quentier/utility/Qt4Helper.h>
#include <QDialog>
#include <QVector>
#include <QStringListModel>

namespace Ui {
class ManageAccountsDialog;
}

class ManageAccountsDialog: public QDialog
{
    Q_OBJECT
public:
    explicit ManageAccountsDialog(QVector<AvailableAccount> availableAccounts,
                                  QWidget * parent = Q_NULLPTR);
    virtual ~ManageAccountsDialog();

private:
    Ui::ManageAccountsDialog *  m_pUi;
    QVector<AvailableAccount>   m_availableAccounts;
    QStringListModel            m_accountListModel;
};

#endif // QUENTIER_MANAGE_ACCOUNTS_DIALOG_H
