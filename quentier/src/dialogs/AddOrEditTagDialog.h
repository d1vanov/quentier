#ifndef QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H
#define QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H

#include "../models/TagModel.h"
#include <quentier/utility/Qt4Helper.h>
#include <QDialog>
#include <QPointer>

namespace Ui {
class AddOrEditTagDialog;
}

QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace quentier {

class AddOrEditTagDialog: public QDialog
{
    Q_OBJECT
public:
    explicit AddOrEditTagDialog(TagModel * pTagModel, QWidget * parent = Q_NULLPTR,
                                const QString & editedTagLocalUid = QString());
    virtual ~AddOrEditTagDialog();

private:
    Ui::AddOrEditTagDialog *    m_pUi;
    QPointer<TagModel>          m_pTagModel;
    QStringListModel *          m_pTagNamesModel;
    QString                     m_editedTagLocalUid;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H
