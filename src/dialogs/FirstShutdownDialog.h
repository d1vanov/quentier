#ifndef QUENTIER_DIALOGS_FIRST_SHUTDOWN_DIALOG_H
#define QUENTIER_DIALOGS_FIRST_SHUTDOWN_DIALOG_H

#include <quentier/utility/Macros.h>
#include <QDialog>

namespace Ui {
class FirstShutdownDialog;
}

namespace quentier {

class FirstShutdownDialog: public QDialog
{
    Q_OBJECT
public:
    explicit FirstShutdownDialog(QWidget * parent = Q_NULLPTR);
    ~FirstShutdownDialog();

private Q_SLOTS:
    void onCloseToTrayPushButtonPressed();
    void onClosePushButtonPressed();

private:
    Ui::FirstShutdownDialog *   m_pUi;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_FIRST_SHUTDOWN_DIALOG_H
