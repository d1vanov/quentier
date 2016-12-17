#include "AddOrEditTagDialog.h"
#include "ui_AddOrEditTagDialog.h"
#include <QStringListModel>

namespace quentier {

AddOrEditTagDialog::AddOrEditTagDialog(TagModel * pTagModel, QWidget * parent,
                                       const QString & editedTagLocalUid) :
    QDialog(parent),
    m_pUi(new Ui::AddOrEditTagDialog),
    m_pTagModel(pTagModel),
    m_pTagNamesModel(Q_NULLPTR),
    m_editedTagLocalUid(editedTagLocalUid)
{
    m_pUi->setupUi(this);
    m_pUi->statusBar->setHidden(true);

    QStringList tagNames;
    if (!m_pTagModel.isNull()) {
        tagNames = m_pTagModel->tagNames();
    }

    if (!tagNames.isEmpty()) {
        qSort(tagNames);
        m_pTagNamesModel = new QStringListModel(this);
        m_pTagNamesModel->setStringList(tagNames);
    }

    // TODO: continue from here

    m_pUi->tagNameLineEdit->setFocus();
}

AddOrEditTagDialog::~AddOrEditTagDialog()
{
    delete m_pUi;
}

} // namespace quentier
