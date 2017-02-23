#include "AddOrEditNotebookDialog.h"
#include "ui_AddOrEditNotebookDialog.h"
#include "../SettingsNames.h"
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/logging/QuentierLogger.h>
#include <QStringListModel>
#include <QPushButton>

#define LAST_SELECTED_STACK QStringLiteral("LastSelectedNotebookStack")

namespace quentier {

AddOrEditNotebookDialog::AddOrEditNotebookDialog(NotebookModel * pNotebookModel,
                                                 QWidget * parent,
                                                 const QString & editedNotebookLocalUid) :
    QDialog(parent),
    m_pUi(new Ui::AddOrEditNotebookDialog),
    m_pNotebookModel(pNotebookModel),
    m_pNotebookStacksModel(Q_NULLPTR),
    m_editedNotebookLocalUid(editedNotebookLocalUid),
    m_stringUtils()
{
    m_pUi->setupUi(this);
    m_pUi->statusBar->setHidden(true);

    QStringList stacks;
    if (!m_pNotebookModel.isNull()) {
        stacks = m_pNotebookModel->stacks();
    }

    if (!stacks.isEmpty()) {
        m_pNotebookStacksModel = new QStringListModel(this);
        m_pNotebookStacksModel->setStringList(stacks);
        m_pUi->notebookStackComboBox->setModel(m_pNotebookStacksModel);
    }

    createConnections();

    if (!m_editedNotebookLocalUid.isEmpty() && !m_pNotebookModel.isNull())
    {
        QModelIndex editedNotebookIndex = m_pNotebookModel->indexForLocalUid(m_editedNotebookLocalUid);
        const NotebookModelItem * pModelItem = m_pNotebookModel->itemForIndex(editedNotebookIndex);
        if (!pModelItem)
        {
            m_pUi->statusBar->setText(tr("Can't find the edited notebook within the model"));
            m_pUi->statusBar->setHidden(false);
        }
        else if (pModelItem->type() != NotebookModelItem::Type::Notebook)
        {
            m_pUi->statusBar->setText(tr("Internal error: the edited item is not a notebook"));
            m_pUi->statusBar->setHidden(false);
        }
        else if (!pModelItem->notebookItem())
        {
            m_pUi->statusBar->setText(tr("Internal error: the edited item's pointer "
                                         "to notebook item is null"));
            m_pUi->statusBar->setHidden(false);
        }
        else
        {
            const NotebookItem * pNotebookItem = pModelItem->notebookItem();
            m_pUi->notebookNameLineEdit->setText(pNotebookItem->name());
            const QString & stack = pNotebookItem->stack();
            if (!stack.isEmpty())
            {
                int index = stacks.indexOf(stack);
                if (index >= 0) {
                    m_pUi->notebookStackComboBox->setCurrentIndex(index);
                }
            }
        }
    }
    else if (!stacks.isEmpty() && !m_pNotebookModel.isNull())
    {
        ApplicationSettings appSettings(m_pNotebookModel->account(), QUENTIER_UI_SETTINGS);
        appSettings.beginGroup(QStringLiteral("AddOrEditNotebookDialog"));
        QString lastSelectedStack = appSettings.value(LAST_SELECTED_STACK).toString();
        appSettings.endGroup();

        int index = stacks.indexOf(lastSelectedStack);
        if (index >= 0) {
            m_pUi->notebookStackComboBox->setCurrentIndex(index);
        }
    }

    m_pUi->notebookNameLineEdit->setFocus();
}

AddOrEditNotebookDialog::~AddOrEditNotebookDialog()
{
    delete m_pUi;
}

void AddOrEditNotebookDialog::accept()
{
    QString notebookName = m_pUi->notebookNameLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(notebookName);

    QString stack = m_pUi->notebookStackComboBox->currentText().trimmed();
    m_stringUtils.removeNewlines(stack);

    QNDEBUG(QStringLiteral("AddOrEditNotebookDialog::accept: notebook name = ")
            << notebookName << QStringLiteral(", stack: ") << stack);

#define REPORT_ERROR(error) \
    m_pUi->statusBar->setText(tr(error)); \
    QNWARNING(error); \
    m_pUi->statusBar->setHidden(false)

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        REPORT_ERROR("Can't accept new notebook or edit existing one: "
                     "notebook model is gone");
        return;
    }

    if (m_editedNotebookLocalUid.isEmpty())
    {
        QNDEBUG(QStringLiteral("Edited notebook local uid is empty, adding new notebook to the model"));

        ErrorString errorDescription;
        QModelIndex index = m_pNotebookModel->createNotebook(notebookName, stack, errorDescription);
        if (!index.isValid()) {
            m_pUi->statusBar->setText(errorDescription.localizedString());
            QNWARNING(errorDescription);
            m_pUi->statusBar->setHidden(false);
            return;
        }
    }
    else
    {
        QNDEBUG(QStringLiteral("Edited notebook local uid is not empty, editing "
                               "the existing notebook within the model"));

        QModelIndex index = m_pNotebookModel->indexForLocalUid(m_editedNotebookLocalUid);
        const NotebookModelItem * pModelItem = m_pNotebookModel->itemForIndex(index);
        if (Q_UNLIKELY(!pModelItem)) {
            REPORT_ERROR("Can't edit notebook: notebook was not found in the model");
            return;
        }

        if (Q_UNLIKELY(pModelItem->type() != NotebookModelItem::Type::Notebook)) {
            REPORT_ERROR("Can't edit notebook: the edited model item is not a notebook");
            return;
        }

        const NotebookItem * pNotebookItem = pModelItem->notebookItem();
        if (Q_UNLIKELY(!pNotebookItem)) {
            REPORT_ERROR("Internal error, can't edit notebook: the edited model "
                         "item has null pointer to the notebook item");
            return;
        }

        // If needed, update the notebook name
        QModelIndex nameIndex = m_pNotebookModel->index(index.row(),
                                                        NotebookModel::Columns::Name,
                                                        index.parent());
        if (pNotebookItem->name().toUpper() != notebookName.toUpper())
        {
            bool res = m_pNotebookModel->setData(nameIndex, notebookName);
            if (Q_UNLIKELY(!res))
            {
                // Probably the new name collides with some existing notebook's name
                QModelIndex existingItemIndex = m_pNotebookModel->indexForNotebookName(notebookName);
                if (existingItemIndex.isValid() &&
                    ((existingItemIndex.row() != nameIndex.row()) ||
                     (existingItemIndex.parent() != nameIndex.parent())))
                {
                    // The new name collides with some existing notebook and not with the currently edited one
                    REPORT_ERROR("The notebook name must be unique in case insensitive manner");
                }
                else
                {
                    // Don't really know what happened...
                    REPORT_ERROR("Can't set this name for the notebook");
                }

                return;
            }
        }

        if (pNotebookItem->stack() != stack)
        {
            QModelIndex movedItemIndex = m_pNotebookModel->moveToStack(nameIndex, stack);
            if (Q_UNLIKELY(!movedItemIndex.isValid())) {
                REPORT_ERROR("Can't set this stack for the notebook");
                return;
            }
        }
    }

    QDialog::accept();
}

#undef REPORT_ERROR

void AddOrEditNotebookDialog::onNotebookNameEdited(const QString & notebookName)
{
    QNDEBUG(QStringLiteral("AddOrEditNotebookDialog::onNotebookNameEdited: ") << notebookName);

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        QNDEBUG(QStringLiteral("Notebook model is missing"));
        return;
    }

    QModelIndex itemIndex = m_pNotebookModel->indexForNotebookName(notebookName);
    if (itemIndex.isValid()) {
        m_pUi->statusBar->setText(tr("The notebook name must be unique in case insensitive manner"));
        m_pUi->statusBar->setHidden(false);
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    m_pUi->statusBar->clear();
    m_pUi->statusBar->setHidden(true);
    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(false);
}

void AddOrEditNotebookDialog::onNotebookStackChanged(const QString & stack)
{
    QNDEBUG(QStringLiteral("AddOrEditNotebookDialog::onNotebookStackChanged: ") << stack);

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        QNDEBUG(QStringLiteral("No notebook model is set, nothing to do"));
        return;
    }

    ApplicationSettings appSettings(m_pNotebookModel->account(), QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("AddOrEditNotebookDialog"));
    appSettings.setValue(LAST_SELECTED_STACK, stack);
    appSettings.endGroup();
}

void AddOrEditNotebookDialog::createConnections()
{
    QObject::connect(m_pUi->notebookNameLineEdit, QNSIGNAL(QLineEdit,textEdited,const QString&),
                     this, QNSLOT(AddOrEditNotebookDialog,onNotebookNameEdited,const QString&));

    QObject::connect(m_pUi->notebookStackComboBox, SIGNAL(currentIndexChanged(QString)),
                     this, SLOT(onNotebookStackChanged(QString)));
    QObject::connect(m_pUi->notebookStackComboBox, QNSIGNAL(QComboBox,editTextChanged,const QString&),
                     this, QNSLOT(AddOrEditNotebookDialog,onNotebookStackChanged,const QString &));
}

} // namespace quentier
