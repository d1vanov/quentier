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

#include "NotebookModelItemInfoWidget.h"
#include "ui_NotebookModelItemInfoWidget.h"

#include <lib/model/notebook/NotebookModel.h>

#include <QKeyEvent>

#include <algorithm>

namespace quentier {

NotebookModelItemInfoWidget::NotebookModelItemInfoWidget(
    const QModelIndex & index, QWidget * parent) :
    QWidget{parent, Qt::Window},
    m_ui{new Ui::NotebookModelItemInfoWidget}
{
    m_ui->setupUi(this);

    setWindowTitle(tr("Notebook info")); // Assume it's notebook for now
    setCheckboxesReadOnly();

    QObject::connect(
        m_ui->okButton, &QPushButton::clicked, this,
        &NotebookModelItemInfoWidget::close);

    if (Q_UNLIKELY(!index.isValid())) {
        setInvalidIndex();
        return;
    }

    const auto * notebookModel =
        qobject_cast<const NotebookModel *>(index.model());

    if (Q_UNLIKELY(!notebookModel)) {
        setNonNotebookModel();
        return;
    }

    const auto * modelItem = notebookModel->itemForIndex(index);
    if (Q_UNLIKELY(!modelItem)) {
        setNoModelItem();
        return;
    }

    const auto * notebookItem = modelItem->cast<NotebookItem>();
    if (notebookItem) {
        setNotebookItem(*notebookItem);
        return;
    }

    const auto * stackItem = modelItem->cast<StackItem>();
    if (stackItem) {
        setStackItem(*stackItem, modelItem->childrenCount());
        return;
    }

    setMessedUpModelItemType();
}

NotebookModelItemInfoWidget::~NotebookModelItemInfoWidget()
{
    delete m_ui;
}

void NotebookModelItemInfoWidget::setCheckboxesReadOnly()
{
    const auto setCheckboxReadOnly = [](QCheckBox & checkbox) {
        checkbox.setAttribute(Qt::WA_TransparentForMouseEvents, true);
        checkbox.setFocusPolicy(Qt::NoFocus);
    };

    setCheckboxReadOnly(*m_ui->notebookSynchronizableCheckBox);
    setCheckboxReadOnly(*m_ui->notebookDirtyCheckBox);
    setCheckboxReadOnly(*m_ui->notebookUpdatableCheckBox);
    setCheckboxReadOnly(*m_ui->notebookNameUpdatableCheckBox);
    setCheckboxReadOnly(*m_ui->notebookPublishedCheckBox);
    setCheckboxReadOnly(*m_ui->notebookFavoritedCheckBox);
    setCheckboxReadOnly(*m_ui->notebookFromLinkedNotebookCheckBox);
}

void NotebookModelItemInfoWidget::setNonNotebookModel()
{
    hideAll();

    m_ui->statusBarLabel->setText(
        tr("Non-notebook model is used on the view"));

    m_ui->statusBarLabel->show();
}

void NotebookModelItemInfoWidget::setInvalidIndex()
{
    hideAll();

    m_ui->statusBarLabel->setText(tr("No notebook is selected"));
    m_ui->statusBarLabel->show();
}

void NotebookModelItemInfoWidget::setNoModelItem()
{
    hideAll();

    m_ui->statusBarLabel->setText(
        tr("No notebook model item was found for index"));

    m_ui->statusBarLabel->show();
}

void NotebookModelItemInfoWidget::setMessedUpModelItemType()
{
    hideAll();

    m_ui->statusBarLabel->setText(
        tr("Internal error: improper type of notebook model item"));

    m_ui->statusBarLabel->show();
}

void NotebookModelItemInfoWidget::hideAll()
{
    hideNotebookStuff();
    hideStackStuff();
}

void NotebookModelItemInfoWidget::hideNotebookStuff()
{
    setNotebookStuffHidden(true);
}

void NotebookModelItemInfoWidget::showNotebookStuff()
{
    setNotebookStuffHidden(false);
}

void NotebookModelItemInfoWidget::setNotebookStuffHidden(const bool flag)
{
    m_ui->notebookLabel->setHidden(flag);
    m_ui->notebookNameLabel->setHidden(flag);
    m_ui->notebookNameLineEdit->setHidden(flag);
    m_ui->notebookStackLabel->setHidden(flag);
    m_ui->notebookStackLineEdit->setHidden(flag);
    m_ui->notebookNumNotesLabel->setHidden(flag);
    m_ui->notebookNumNotesLineEdit->setHidden(flag);
    m_ui->notebookSynchronizableLabel->setHidden(flag);
    m_ui->notebookSynchronizableCheckBox->setHidden(flag);
    m_ui->notebookDirtyLabel->setHidden(flag);
    m_ui->notebookDirtyCheckBox->setHidden(flag);
    m_ui->notebookUpdatableLabel->setHidden(flag);
    m_ui->notebookUpdatableCheckBox->setHidden(flag);
    m_ui->notebookNameUpdatableLabel->setHidden(flag);
    m_ui->notebookNameUpdatableCheckBox->setHidden(flag);
    m_ui->notebookDefaultLabel->setHidden(flag);
    m_ui->notebookDefaultCheckBox->setHidden(flag);
    m_ui->notebookLastUsedLabel->setHidden(flag);
    m_ui->notebookLastUsedCheckBox->setHidden(flag);
    m_ui->notebookPublishedLabel->setHidden(flag);
    m_ui->notebookPublishedCheckBox->setHidden(flag);
    m_ui->notebookFavoritedLabel->setHidden(flag);
    m_ui->notebookFavoritedCheckBox->setHidden(flag);
    m_ui->notebookFromLinkedNotebookLabel->setHidden(flag);
    m_ui->notebookFromLinkedNotebookCheckBox->setHidden(flag);
    m_ui->notebookGuidLabel->setHidden(flag);
    m_ui->notebookGuidLineEdit->setHidden(flag);
    m_ui->notebookLocalIdLabel->setHidden(flag);
    m_ui->notebookLocalIdLineEdit->setHidden(flag);
}

void NotebookModelItemInfoWidget::hideStackStuff()
{
    setStackStuffHidden(true);
}

void NotebookModelItemInfoWidget::showStackStuff()
{
    setStackStuffHidden(false);
}

void NotebookModelItemInfoWidget::setStackStuffHidden(const bool flag)
{
    m_ui->stackLabel->setHidden(flag);
    m_ui->stackNameLabel->setHidden(flag);
    m_ui->stackNameLineEdit->setHidden(flag);
    m_ui->stackNumNotebooksLabel->setHidden(flag);
    m_ui->stackNumNotebooksLineEdit->setHidden(flag);
}

void NotebookModelItemInfoWidget::setNotebookItem(const NotebookItem & item)
{
    showNotebookStuff();
    hideStackStuff();
    m_ui->statusBarLabel->hide();

    m_ui->notebookNameLineEdit->setText(item.name());
    m_ui->notebookStackLineEdit->setText(item.stack());

    m_ui->notebookNumNotesLineEdit->setText(
        QString::number(std::max<quint32>(item.noteCount(), 0)));

    m_ui->notebookSynchronizableCheckBox->setChecked(item.isSynchronizable());
    m_ui->notebookDirtyCheckBox->setChecked(item.isDirty());
    m_ui->notebookUpdatableCheckBox->setChecked(item.isUpdatable());

    m_ui->notebookNameUpdatableCheckBox->setChecked(
        item.isUpdatable() && item.nameIsUpdatable());

    m_ui->notebookDefaultCheckBox->setChecked(item.isDefault());
    m_ui->notebookPublishedCheckBox->setChecked(item.isPublished());
    m_ui->notebookFavoritedCheckBox->setChecked(item.isFavorited());

    m_ui->notebookFromLinkedNotebookCheckBox->setChecked(
        !item.linkedNotebookGuid().isEmpty());

    m_ui->notebookGuidLineEdit->setText(item.guid());
    m_ui->notebookLocalIdLineEdit->setText(item.localId());

    setMinimumWidth(475);
    setWindowTitle(tr("Notebook info"));
}

void NotebookModelItemInfoWidget::setStackItem(
    const StackItem & item, const int numChildren)
{
    hideNotebookStuff();
    showStackStuff();
    m_ui->statusBarLabel->hide();

    m_ui->stackNameLineEdit->setText(item.name());

    m_ui->stackNumNotebooksLineEdit->setText(
        QString::number(std::max(numChildren, 0)));

    setWindowTitle(tr("Notebook stack info"));
}

void NotebookModelItemInfoWidget::keyPressEvent(QKeyEvent * event)
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
