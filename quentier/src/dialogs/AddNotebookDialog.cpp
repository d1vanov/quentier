#include "AddNotebookDialog.h"
#include "ui_AddNotebookDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <QStringListModel>

namespace quentier {

AddNotebookDialog::AddNotebookDialog(NotebookModel * pNotebookModel,
                                     QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::AddNotebookDialog),
    m_pNotebookModel(pNotebookModel)
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
    m_pUi->notebookNameLineEdit->setFocus();
}

AddNotebookDialog::~AddNotebookDialog()
{
    delete m_pUi;
}

void AddNotebookDialog::accept()
{
    QString notebookName = m_pUi->notebookNameLineEdit->text();
    QString stack = m_pUi->notebookStackComboBox->currentText();

    QNDEBUG(QStringLiteral("AddNotebookDialog::accept: notebook name = ")
            << notebookName << QStringLiteral(", stack: ") << stack);

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        m_pUi->statusBar->setText(tr("Can't accept new notebook: notebook model is gone"));
        m_pUi->statusBar->setHidden(false);
        QDialog::reject();
        return;
    }

    QNLocalizedString errorDescription;
    QModelIndex index = m_pNotebookModel->createNotebook(notebookName, stack, errorDescription);
    if (!index.isValid()) {
        m_pUi->statusBar->setText(errorDescription.localizedString());
        m_pUi->statusBar->setHidden(false);
        QDialog::reject();
        return;
    }

    QDialog::accept();
}

void AddNotebookDialog::onNotebookNameEdited(const QString & notebookName)
{
    QNDEBUG(QStringLiteral("AddNotebookDialog::onNotebookNameEdited: ") << notebookName);

    m_pUi->statusBar->clear();
    m_pUi->statusBar->setHidden(true);
}

void AddNotebookDialog::createConnections()
{
    QObject::connect(m_pUi->notebookNameLineEdit, QNSIGNAL(QLineEdit,textEdited,const QString&),
                     this, QNSLOT(AddNotebookDialog,onNotebookNameEdited,const QString&));
}

} // namespace quentier
