#ifndef QUENTIER_DIALOGS_ADD_OR_EDIT_SAVED_SEARCH_DIALOG_H
#define QUENTIER_DIALOGS_ADD_OR_EDIT_SAVED_SEARCH_DIALOG_H

#include "../models/SavedSearchModel.h"
#include <quentier/utility/Qt4Helper.h>
#include <QDialog>
#include <QPointer>
#include <QScopedPointer>

namespace Ui {
class AddOrEditSavedSearchDialog;
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteSearchQuery)

class AddOrEditSavedSearchDialog: public QDialog
{
    Q_OBJECT
public:
    explicit AddOrEditSavedSearchDialog(SavedSearchModel * pSavedSearchModel,
                                        QWidget * parent = Q_NULLPTR,
                                        const QString & editedSavedSearchLocalUid = QString());
    ~AddOrEditSavedSearchDialog();

private Q_SLOTS:
    virtual void accept() Q_DECL_OVERRIDE;
    void onSearchQueryEdited(const QString & searchQuery);

private:
    Ui::AddOrEditSavedSearchDialog *    m_pUi;
    QPointer<SavedSearchModel>          m_pSavedSearchModel;
    QScopedPointer<NoteSearchQuery>     m_pSearchQuery;
    QString                             m_editedSavedSearchLocalUid;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_ADD_OR_EDIT_SAVED_SEARCH_DIALOG_H
