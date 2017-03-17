#ifndef QUENTIER_DIALOGS_PREFERENCES_DIALOG_H
#define QUENTIER_DIALOGS_PREFERENCES_DIALOG_H

#include <quentier/utility/Macros.h>
#include <QDialog>
#include <QPointer>

namespace Ui {
class PreferencesDialog;
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AccountManager)

class PreferencesDialog: public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(AccountManager & accountManager,
                               QWidget * parent = Q_NULLPTR);
    virtual ~PreferencesDialog();

Q_SIGNALS:
    void showSystemTrayIconOptionChanged(bool shouldShow);

private Q_SLOTS:
    void onShowSystemTrayIconCheckboxToggled(bool checked);

private:
    void createConnections();

private:
    Ui::PreferencesDialog *         m_pUi;
    QPointer<AccountManager>        m_pAccountManager;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_PREFERENCES_DIALOG_H
