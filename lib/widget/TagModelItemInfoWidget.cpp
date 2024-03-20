/*
 * Copyright 2017-2024 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TagModelItemInfoWidget.h"
#include "ui_TagModelItemInfoWidget.h"

#include <lib/model/tag/TagModel.h>

#include <QKeyEvent>

namespace quentier {

TagModelItemInfoWidget::TagModelItemInfoWidget(
    const QModelIndex & index, QWidget * parent) :
    QWidget{parent, Qt::Window},
    m_ui{new Ui::TagModelItemInfoWidget}
{
    m_ui->setupUi(this);

    setWindowTitle(tr("Tag info"));
    setCheckboxesReadOnly();

    QObject::connect(
        m_ui->okButton, &QPushButton::clicked, this,
        &TagModelItemInfoWidget::close);

    if (Q_UNLIKELY(!index.isValid())) {
        setInvalidIndex();
        return;
    }

    const auto * tagModel = qobject_cast<const TagModel *>(index.model());
    if (Q_UNLIKELY(!tagModel)) {
        setNonTagModel();
        return;
    }

    const auto * modelItem = tagModel->itemForIndex(index);
    if (Q_UNLIKELY(!modelItem)) {
        setNoModelItem();
        return;
    }

    const auto * tagItem = modelItem->cast<TagItem>();
    if (Q_UNLIKELY(!tagItem)) {
        setNonTagItem();
        return;
    }

    setTagItem(*modelItem, *tagItem);
}

TagModelItemInfoWidget::~TagModelItemInfoWidget()
{
    delete m_ui;
}

void TagModelItemInfoWidget::setCheckboxesReadOnly()
{
    const auto setCheckboxReadOnly = [](QCheckBox & checkbox) {
        checkbox.setAttribute(
            Qt::WA_TransparentForMouseEvents, true);
        checkbox.setFocusPolicy(Qt::NoFocus);
    };

    setCheckboxReadOnly(*m_ui->synchronizableCheckBox);
    setCheckboxReadOnly(*m_ui->dirtyCheckBox);
    setCheckboxReadOnly(*m_ui->favoritedCheckBox);
}

void TagModelItemInfoWidget::setNonTagModel()
{
    hideAll();

    m_ui->statusBarLabel->setText(tr("Non-tag model is used on the view"));
    m_ui->statusBarLabel->show();
}

void TagModelItemInfoWidget::setInvalidIndex()
{
    hideAll();

    m_ui->statusBarLabel->setText(tr("No tag is selected"));
    m_ui->statusBarLabel->show();
}

void TagModelItemInfoWidget::setNoModelItem()
{
    hideAll();

    m_ui->statusBarLabel->setText(tr("No tag model item was found for index"));
    m_ui->statusBarLabel->show();
}

void TagModelItemInfoWidget::setNonTagItem()
{
    hideAll();

    m_ui->statusBarLabel->setText(
        tr("The tag model item doesn't correspond to the actual tag"));

    m_ui->statusBarLabel->show();
}

void TagModelItemInfoWidget::setTagItem(
    const ITagModelItem & modelItem, const TagItem & item)
{
    m_ui->statusBarLabel->hide();

    m_ui->tagNameLineEdit->setText(item.name());

    const auto * parentItem = modelItem.parent();
    const QString & parentLocalId = item.parentLocalId();

    const auto * parentTagItem =
        (parentItem ? parentItem->cast<TagItem>() : nullptr);

    if (parentTagItem && !parentLocalId.isEmpty()) {
        m_ui->parentTagLineEdit->setText(parentTagItem->name());
    }

    m_ui->childrenLineEdit->setText(
        QString::number(modelItem.childrenCount()));

    m_ui->numNotesLineEdit->setText(QString::number(item.noteCount()));

    m_ui->synchronizableCheckBox->setChecked(item.isSynchronizable());
    m_ui->dirtyCheckBox->setChecked(item.isDirty());
    m_ui->favoritedCheckBox->setChecked(item.isFavorited());

    m_ui->linkedNotebookGuidLineEdit->setText(item.linkedNotebookGuid());
    m_ui->guidLineEdit->setText(item.guid());
    m_ui->localIdLineEdit->setText(item.localId());

    setMinimumWidth(475);
}

void TagModelItemInfoWidget::hideAll()
{
    m_ui->tagLabel->setHidden(true);
    m_ui->tagNameLabel->setHidden(true);
    m_ui->tagNameLineEdit->setHidden(true);
    m_ui->parentTagLabel->setHidden(true);
    m_ui->parentTagLineEdit->setHidden(true);
    m_ui->childrenLabel->setHidden(true);
    m_ui->childrenLineEdit->setHidden(true);
    m_ui->numNotesLabel->setHidden(true);
    m_ui->numNotesLineEdit->setHidden(true);
    m_ui->synchronizableLabel->setHidden(true);
    m_ui->synchronizableCheckBox->setHidden(true);
    m_ui->dirtyLabel->setHidden(true);
    m_ui->dirtyCheckBox->setHidden(true);
    m_ui->favoritedLabel->setHidden(true);
    m_ui->favoritedCheckBox->setHidden(true);
    m_ui->linkedNotebookGuidLabel->setHidden(true);
    m_ui->linkedNotebookGuidLineEdit->setHidden(true);
    m_ui->guidLabel->setHidden(true);
    m_ui->guidLineEdit->setHidden(true);
    m_ui->localIdLabel->setHidden(true);
    m_ui->localIdLineEdit->setHidden(true);
}

void TagModelItemInfoWidget::keyPressEvent(QKeyEvent * event)
{
    if (Q_UNLIKELY(!event)) {
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }

    QWidget::keyPressEvent(event);
}

} // namespace quentier
