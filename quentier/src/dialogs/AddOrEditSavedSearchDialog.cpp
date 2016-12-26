#include "AddOrEditSavedSearchDialog.h"
#include "ui_AddOrEditSavedSearchDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/local_storage/NoteSearchQuery.h>
#include <QPushButton>

namespace quentier {

AddOrEditSavedSearchDialog::AddOrEditSavedSearchDialog(SavedSearchModel * pSavedSearchModel,
                                                       QWidget * parent,
                                                       const QString & editedSavedSearchLocalUid) :
    QDialog(parent),
    m_pUi(new Ui::AddOrEditSavedSearchDialog),
    m_pSavedSearchModel(pSavedSearchModel),
    m_pSearchQuery(new NoteSearchQuery),
    m_editedSavedSearchLocalUid(editedSavedSearchLocalUid)
{
    m_pUi->setupUi(this);
    m_pUi->statusBar->setHidden(true);

    // TODO: implement further
}

AddOrEditSavedSearchDialog::~AddOrEditSavedSearchDialog()
{
    delete m_pUi;
}

void AddOrEditSavedSearchDialog::accept()
{
    // TODO: implement
}

void AddOrEditSavedSearchDialog::onSearchQueryEdited(const QString & searchQuery)
{
    QNDEBUG(QStringLiteral("AddOrEditSavedSearchDialog::onSearchQueryEdited: ") << searchQuery);

    m_pUi->statusBar->clear();
    m_pUi->statusBar->setHidden(true);
    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    QNLocalizedString parseError;
    bool res = m_pSearchQuery->setQueryString(searchQuery, parseError);
    if (!res)
    {
        QNLocalizedString error = QNLocalizedString("Can't parse search query string", this);
        error += QStringLiteral(": ");
        error += parseError;
        QNDEBUG(error);
        m_pUi->statusBar->setText(error.localizedString());
        m_pUi->statusBar->setHidden(false);
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    QNTRACE(QStringLiteral("Successfully parsed the note search query: ") << *m_pSearchQuery);
}

} // namespace quentier
