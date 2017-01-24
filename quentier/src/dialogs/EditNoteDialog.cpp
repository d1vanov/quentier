#include "EditNoteDialog.h"
#include "ui_EditNoteDialog.h"
#include "../models/NotebookModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QStringListModel>
#include <QModelIndex>
#include <algorithm>
#include <iterator>

namespace quentier {

EditNoteDialog::EditNoteDialog(const Note & note,
                               NotebookModel * pNotebookModel,
                               QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::EditNoteDialog),
    m_note(note),
    m_pNotebookModel(pNotebookModel),
    m_pNotebookNamesModel(new QStringListModel(this))
{
    m_pUi->setupUi(this);

    fillNotebookNames();
    m_pUi->notebookComboBox->setModel(m_pNotebookNamesModel);

    createConnections();
}

EditNoteDialog::~EditNoteDialog()
{
    delete m_pUi;
}

void EditNoteDialog::accept()
{
    QNDEBUG(QStringLiteral("EditNoteDialog::accept"));

    // TODO: implement
    QDialog::accept();
}

void EditNoteDialog::dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                                 )
#else
                                 , const QVector<int> & roles)
#endif
{
    QNDEBUG(QStringLiteral("EditNoteDialog::dataChanged"));

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_UNUSED(roles)
#endif

    if (!topLeft.isValid() || !bottomRight.isValid()) {
        QNDEBUG(QStringLiteral("At least one of changed indexes is invalid"));
        fillNotebookNames();
        return;
    }

    if ((topLeft.column() > NotebookModel::Columns::Name) ||
        (bottomRight.column() < NotebookModel::Columns::Name))
    {
        QNTRACE(QStringLiteral("The updated indexed don't contain the notebook name"));
        return;
    }

    fillNotebookNames();
}

void EditNoteDialog::rowsInserted(const QModelIndex & parent, int start, int end)
{
    QNDEBUG(QStringLiteral("EditNoteDialog::rowsInserted"));

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    fillNotebookNames();
}

void EditNoteDialog::rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end)
{
    QNDEBUG(QStringLiteral("EditNoteDialog::rowsAboutToBeRemoved"));

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        QNDEBUG(QStringLiteral("Notebook model is null, nothing to do"));
        return;
    }

    QStringList currentNotebookNames = m_pNotebookNamesModel->stringList();

    for(int i = start; i <= end; ++i)
    {
        QModelIndex index = m_pNotebookModel->index(i, NotebookModel::Columns::Name, parent);
        QString removedNotebookName = m_pNotebookModel->data(index).toString();
        if (Q_UNLIKELY(removedNotebookName.isEmpty())) {
            continue;
        }

        auto it = std::lower_bound(currentNotebookNames.constBegin(),
                                   currentNotebookNames.constEnd(),
                                   removedNotebookName);
        if ((it != currentNotebookNames.constEnd()) && (*it == removedNotebookName)) {
            int offset = static_cast<int>(std::distance(currentNotebookNames.constBegin(), it));
            QStringList::iterator nit = currentNotebookNames.begin() + offset;
            Q_UNUSED(currentNotebookNames.erase(nit));
        }
    }

    m_pNotebookNamesModel->setStringList(currentNotebookNames);
}

void EditNoteDialog::createConnections()
{
    QNDEBUG(QStringLiteral("EditNoteDialog::createConnections"));

    if (!m_pNotebookModel.isNull()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QObject::connect(m_pNotebookModel.data(), &NotebookModel::dataChanged,
                         this, &EditNoteDialog::dataChanged);
#else
        QObject::connect(m_pNotebookModel.data(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                         this, SLOT(dataChanged(QModelIndex,QModelIndex)));
#endif
        QObject::connect(m_pNotebookModel.data(), QNSIGNAL(NotebookModel,rowsInserted,QModelIndex,int,int),
                         this, QNSLOT(EditNoteDialog,rowsInserted,QModelIndex,int,int));
        QObject::connect(m_pNotebookModel.data(), QNSIGNAL(NotebookModel,rowsAboutToBeRemoved,QModelIndex,int,int),
                         this, QNSLOT(EditNoteDialog,rowsAboutToBeRemoved,QModelIndex,int,int));
    }
}

void EditNoteDialog::fillNotebookNames()
{
    QNDEBUG(QStringLiteral("EditNoteDialog::fillNotebookNames"));

    QStringList notebookNames;
    if (!m_pNotebookModel.isNull()) {
        NotebookModel::NotebookFilters filter(NotebookModel::NotebookFilter::CanCreateNotes);
        notebookNames = m_pNotebookModel->notebookNames(filter);
    }

    m_pNotebookNamesModel->setStringList(notebookNames);
}

} // namespace quentier
