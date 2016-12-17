#ifndef QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H
#define QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H

#include <quentier/utility/Qt4Helper.h>
#include <QDialog>

namespace Ui {
class AddOrEditTagDialog;
}

namespace quentier {

class AddOrEditTagDialog: public QDialog
{
    Q_OBJECT
public:
    explicit AddOrEditTagDialog(QWidget * parent = Q_NULLPTR);
    virtual ~AddOrEditTagDialog();

private:
    Ui::AddOrEditTagDialog * m_pUi;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H
