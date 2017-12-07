#ifndef QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H
#define QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H

#include "../models/TagModel.h"
#include <quentier/utility/Macros.h>
#include <quentier/utility/StringUtils.h>
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

private Q_SLOTS:
    virtual void accept() Q_DECL_OVERRIDE;
    void onTagNameEdited(const QString & tagName);
    void onParentTagNameChanged(const QString & parentTagName);

private:
    void createConnections();
    bool setupEditedTagItem(QStringList & tagNames, int & currentIndex);

private:
    Ui::AddOrEditTagDialog *    m_pUi;
    QPointer<TagModel>          m_pTagModel;
    QStringListModel *          m_pTagNamesModel;
    QString                     m_editedTagLocalUid;

    // The name specified at any given moment in the line editor
    QString                     m_currentTagName;

    StringUtils                 m_stringUtils;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_ADD_OR_EDIT_TAG_DIALOG_H
