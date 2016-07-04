#include "NewTagLineEditor.h"
#include "ui_NewTagLineEditor.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QKeyEvent>
#include <QCompleter>
#include <QStringListModel>

namespace quentier {

NewTagLineEditor::NewTagLineEditor(TagModel * pTagModel, QWidget *parent) :
    QLineEdit(parent),
    m_pUi(new Ui::NewTagLineEditor),
    m_pTagModel(pTagModel),
    m_pCompleter(new QCompleter(this))
{
    m_pUi->setupUi(this);
    setPlaceholderText(tr("Add tag..."));
    createConnections();
}

NewTagLineEditor::~NewTagLineEditor()
{
    delete m_pUi;
}

void NewTagLineEditor::onTagModelDataChanged(const QModelIndex & from, const QModelIndex & to)
{
    Q_UNUSED(from)
    Q_UNUSED(to)
    setupCompleter();
}

void NewTagLineEditor::onTagModelRowsChanged(const QModelIndex & index, int start, int end)
{
    Q_UNUSED(index)
    Q_UNUSED(start)
    Q_UNUSED(end)
    setupCompleter();
}

void NewTagLineEditor::onTagModelChanged()
{
    setupCompleter();
}

void NewTagLineEditor::keyPressEvent(QKeyEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return;
    }

    int key = pEvent->key();
    QNTRACE("NewTagLineEditor::keyPressEvent: key = " << key);

    if ((key == Qt::Key_Return) || (key == Qt::Key_Tab)) {
        QString tagName = QLineEdit::text();
        emit newTagAdded(tagName);
    }

    QLineEdit::keyPressEvent(pEvent);
}

void NewTagLineEditor::createConnections()
{
    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        return;
    }

    QObject::connect(m_pTagModel.data(), QNSIGNAL(TagModel,dataChanged,const QModelIndex&,const QModelIndex&),
                     this, QNSLOT(NewTagLineEditor,onTagModelDataChanged,const QModelIndex&,const QModelIndex&));
    QObject::connect(m_pTagModel.data(), QNSIGNAL(TagModel,layoutChanged),
                     this, QNSLOT(NewTagLineEditor,onTagModelChanged));
    QObject::connect(m_pTagModel.data(), QNSIGNAL(TagModel,modelReset),
                     this, QNSLOT(NewTagLineEditor,onTagModelChanged));
    QObject::connect(m_pTagModel.data(), QNSIGNAL(TagModel,rowsInserted,const QModelIndex&,int,int),
                     this, QNSLOT(NewTagLineEditor,onTagModelRowsChanged,const QModelIndex&,int,int));
}

void NewTagLineEditor::setupCompleter()
{
    QNDEBUG("NewTagLineEditor::setupCompleter");

    QStringListModel * pModel = qobject_cast<QStringListModel*>(m_pCompleter->model());
    if (!pModel) {
        pModel = new QStringListModel(this);
    }

    QStringList tagNames;

    if (Q_LIKELY(!m_pTagModel.isNull())) {
        tagNames = m_pTagModel->tagNames();
        tagNames.sort();
    }

    pModel->setStringList(tagNames);

    m_pCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_pCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    m_pCompleter->setModel(pModel);
}

} // namespace quentier
