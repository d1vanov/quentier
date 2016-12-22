#include "TagModelItemInfoWidget.h"
#include "ui_TagModelItemInfoWidget.h"
#include "../models/TagModel.h"
#include <QKeyEvent>

namespace quentier {

TagModelItemInfoWidget::TagModelItemInfoWidget(const QModelIndex & index, QWidget * parent) :
    QWidget(parent, Qt::Window),
    m_pUi(new Ui::TagModelItemInfoWidget)
{
    m_pUi->setupUi(this);
    setCheckboxesReadOnly();

    QObject::connect(m_pUi->okButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(TagModelItemInfoWidget,close));

    if (Q_UNLIKELY(!index.isValid())) {
        setInvalidIndex();
        return;
    }

    const TagModel * pTagModel = qobject_cast<const TagModel*>(index.model());
    if (Q_UNLIKELY(!pTagModel)) {
        setNonTagModel();
        return;
    }

    const TagModelItem * pModelItem = pTagModel->itemForIndex(index);
    if (Q_UNLIKELY(!pModelItem)) {
        setNoModelItem();
        return;
    }

    setTagItem(*pModelItem);
}

TagModelItemInfoWidget::~TagModelItemInfoWidget()
{
    delete m_pUi;
}

void TagModelItemInfoWidget::setCheckboxesReadOnly()
{
#define SET_CHECKBOX_READ_ONLY(name) \
    m_pUi->name##CheckBox->setAttribute(Qt::WA_TransparentForMouseEvents, true); \
    m_pUi->name##CheckBox->setFocusPolicy(Qt::NoFocus)

    SET_CHECKBOX_READ_ONLY(synchronizable);
    SET_CHECKBOX_READ_ONLY(dirty);

#undef SET_CHECKBOX_READ_ONLY
}

void TagModelItemInfoWidget::setNonTagModel()
{
    hideAll();

    m_pUi->statusBarLabel->setText(tr("Non-tag model is used on the view"));
    m_pUi->statusBarLabel->show();
}

void TagModelItemInfoWidget::setInvalidIndex()
{
    hideAll();

    m_pUi->statusBarLabel->setText(tr("No tag is selected"));
    m_pUi->statusBarLabel->show();
}

void TagModelItemInfoWidget::setNoModelItem()
{
    hideAll();

    m_pUi->statusBarLabel->setText(tr("No tag model item was found for index"));
    m_pUi->statusBarLabel->show();
}

void TagModelItemInfoWidget::setTagItem(const TagModelItem & item)
{
    m_pUi->statusBarLabel->hide();

    m_pUi->tagNameLineEdit->setText(item.name());

    const TagModelItem * pParentItem = item.parent();
    const QString & parentLocalUid = item.parentLocalUid();

    if (pParentItem && !parentLocalUid.isEmpty()) {
        m_pUi->parentTagLineEdit->setText(pParentItem->name());
    }

    m_pUi->childrenLineEdit->setText(QString::number(item.numChildren()));
    m_pUi->numNotesLineEdit->setText(QString::number(item.numNotesPerTag()));

    m_pUi->synchronizableCheckBox->setChecked(item.isSynchronizable());
    m_pUi->dirtyCheckBox->setChecked(item.isDirty());

    m_pUi->linkedNotebookGuidLineEdit->setText(item.linkedNotebookGuid());
    m_pUi->guidLineEdit->setText(item.guid());
    m_pUi->localUidLineEdit->setText(item.localUid());

    setMinimumWidth(475);
    setWindowTitle(tr("Tag info"));
}

void TagModelItemInfoWidget::hideAll()
{
    m_pUi->tagNameLabel->setHidden(true);
    m_pUi->tagNameLineEdit->setHidden(true);
    m_pUi->parentTagLabel->setHidden(true);
    m_pUi->parentTagLineEdit->setHidden(true);
    m_pUi->childrenLabel->setHidden(true);
    m_pUi->childrenLineEdit->setHidden(true);
    m_pUi->numNotesLabel->setHidden(true);
    m_pUi->numNotesLineEdit->setHidden(true);
    m_pUi->synchronizableLabel->setHidden(true);
    m_pUi->synchronizableCheckBox->setHidden(true);
    m_pUi->dirtyLabel->setHidden(true);
    m_pUi->dirtyCheckBox->setHidden(true);
    m_pUi->linkedNotebookGuidLabel->setHidden(true);
    m_pUi->linkedNotebookGuidLineEdit->setHidden(true);
    m_pUi->guidLabel->setHidden(true);
    m_pUi->guidLineEdit->setHidden(true);
    m_pUi->localUidLabel->setHidden(true);
    m_pUi->localUidLineEdit->setHidden(true);
}

void TagModelItemInfoWidget::keyPressEvent(QKeyEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return;
    }

    if (pEvent->key() == Qt::Key_Escape) {
        close();
        return;
    }

    QWidget::keyPressEvent(pEvent);
}

} // namespace quentier
