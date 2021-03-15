/*
 * Copyright 2017-2021 Dmitry Ivanov
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
    QWidget(parent, Qt::Window),
    m_pUi(new Ui::NotebookModelItemInfoWidget)
{
    m_pUi->setupUi(this);

    setWindowTitle(tr("Notebook info")); // Assume it's notebook for now
    setCheckboxesReadOnly();

    QObject::connect(
        m_pUi->okButton, &QPushButton::clicked, this,
        &NotebookModelItemInfoWidget::close);

    if (Q_UNLIKELY(!index.isValid())) {
        setInvalidIndex();
        return;
    }

    const auto * pNotebookModel =
        qobject_cast<const NotebookModel *>(index.model());

    if (Q_UNLIKELY(!pNotebookModel)) {
        setNonNotebookModel();
        return;
    }

    const auto * pModelItem = pNotebookModel->itemForIndex(index);
    if (Q_UNLIKELY(!pModelItem)) {
        setNoModelItem();
        return;
    }

    const auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (pNotebookItem) {
        setNotebookItem(*pNotebookItem);
        return;
    }

    const auto * pStackItem = pModelItem->cast<StackItem>();
    if (pStackItem) {
        setStackItem(*pStackItem, pModelItem->childrenCount());
        return;
    }

    setMessedUpModelItemType();
}

NotebookModelItemInfoWidget::~NotebookModelItemInfoWidget()
{
    delete m_pUi;
}

void NotebookModelItemInfoWidget::setCheckboxesReadOnly()
{
#define SET_CHECKBOX_READ_ONLY(name)                                           \
    m_pUi->notebook##name##CheckBox->setAttribute(                             \
        Qt::WA_TransparentForMouseEvents, true);                               \
    m_pUi->notebook##name##CheckBox->setFocusPolicy(Qt::NoFocus)

    SET_CHECKBOX_READ_ONLY(Synchronizable);
    SET_CHECKBOX_READ_ONLY(Dirty);
    SET_CHECKBOX_READ_ONLY(Updatable);
    SET_CHECKBOX_READ_ONLY(NameUpdatable);
    SET_CHECKBOX_READ_ONLY(Default);
    SET_CHECKBOX_READ_ONLY(LastUsed);
    SET_CHECKBOX_READ_ONLY(Published);
    SET_CHECKBOX_READ_ONLY(Favorited);
    SET_CHECKBOX_READ_ONLY(FromLinkedNotebook);

#undef SET_CHECKBOX_READ_ONLY
}

void NotebookModelItemInfoWidget::setNonNotebookModel()
{
    hideAll();

    m_pUi->statusBarLabel->setText(
        tr("Non-notebook model is used on the view"));

    m_pUi->statusBarLabel->show();
}

void NotebookModelItemInfoWidget::setInvalidIndex()
{
    hideAll();

    m_pUi->statusBarLabel->setText(tr("No notebook is selected"));
    m_pUi->statusBarLabel->show();
}

void NotebookModelItemInfoWidget::setNoModelItem()
{
    hideAll();

    m_pUi->statusBarLabel->setText(
        tr("No notebook model item was found for index"));

    m_pUi->statusBarLabel->show();
}

void NotebookModelItemInfoWidget::setMessedUpModelItemType()
{
    hideAll();

    m_pUi->statusBarLabel->setText(
        tr("Internal error: improper type of notebook model item"));

    m_pUi->statusBarLabel->show();
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
    m_pUi->notebookLabel->setHidden(flag);
    m_pUi->notebookNameLabel->setHidden(flag);
    m_pUi->notebookNameLineEdit->setHidden(flag);
    m_pUi->notebookStackLabel->setHidden(flag);
    m_pUi->notebookStackLineEdit->setHidden(flag);
    m_pUi->notebookNumNotesLabel->setHidden(flag);
    m_pUi->notebookNumNotesLineEdit->setHidden(flag);
    m_pUi->notebookSynchronizableLabel->setHidden(flag);
    m_pUi->notebookSynchronizableCheckBox->setHidden(flag);
    m_pUi->notebookDirtyLabel->setHidden(flag);
    m_pUi->notebookDirtyCheckBox->setHidden(flag);
    m_pUi->notebookUpdatableLabel->setHidden(flag);
    m_pUi->notebookUpdatableCheckBox->setHidden(flag);
    m_pUi->notebookNameUpdatableLabel->setHidden(flag);
    m_pUi->notebookNameUpdatableCheckBox->setHidden(flag);
    m_pUi->notebookDefaultLabel->setHidden(flag);
    m_pUi->notebookDefaultCheckBox->setHidden(flag);
    m_pUi->notebookLastUsedLabel->setHidden(flag);
    m_pUi->notebookLastUsedCheckBox->setHidden(flag);
    m_pUi->notebookPublishedLabel->setHidden(flag);
    m_pUi->notebookPublishedCheckBox->setHidden(flag);
    m_pUi->notebookFavoritedLabel->setHidden(flag);
    m_pUi->notebookFavoritedCheckBox->setHidden(flag);
    m_pUi->notebookFromLinkedNotebookLabel->setHidden(flag);
    m_pUi->notebookFromLinkedNotebookCheckBox->setHidden(flag);
    m_pUi->notebookGuidLabel->setHidden(flag);
    m_pUi->notebookGuidLineEdit->setHidden(flag);
    m_pUi->notebookLocalUidLabel->setHidden(flag);
    m_pUi->notebookLocalUidLineEdit->setHidden(flag);
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
    m_pUi->stackLabel->setHidden(flag);
    m_pUi->stackNameLabel->setHidden(flag);
    m_pUi->stackNameLineEdit->setHidden(flag);
    m_pUi->stackNumNotebooksLabel->setHidden(flag);
    m_pUi->stackNumNotebooksLineEdit->setHidden(flag);
}

void NotebookModelItemInfoWidget::setNotebookItem(const NotebookItem & item)
{
    showNotebookStuff();
    hideStackStuff();
    m_pUi->statusBarLabel->hide();

    m_pUi->notebookNameLineEdit->setText(item.name());
    m_pUi->notebookStackLineEdit->setText(item.stack());

    m_pUi->notebookNumNotesLineEdit->setText(
        QString::number(std::max(item.noteCount(), 0)));

    m_pUi->notebookSynchronizableCheckBox->setChecked(item.isSynchronizable());
    m_pUi->notebookDirtyCheckBox->setChecked(item.isDirty());
    m_pUi->notebookUpdatableCheckBox->setChecked(item.isUpdatable());

    m_pUi->notebookNameUpdatableCheckBox->setChecked(
        item.isUpdatable() && item.nameIsUpdatable());

    m_pUi->notebookDefaultCheckBox->setChecked(item.isDefault());
    m_pUi->notebookLastUsedCheckBox->setChecked(item.isLastUsed());
    m_pUi->notebookPublishedCheckBox->setChecked(item.isPublished());
    m_pUi->notebookFavoritedCheckBox->setChecked(item.isFavorited());

    m_pUi->notebookFromLinkedNotebookCheckBox->setChecked(
        !item.linkedNotebookGuid().isEmpty());

    m_pUi->notebookGuidLineEdit->setText(item.guid());
    m_pUi->notebookLocalUidLineEdit->setText(item.localUid());

    setMinimumWidth(475);
    setWindowTitle(tr("Notebook info"));
}

void NotebookModelItemInfoWidget::setStackItem(
    const StackItem & item, const int numChildren)
{
    hideNotebookStuff();
    showStackStuff();
    m_pUi->statusBarLabel->hide();

    m_pUi->stackNameLineEdit->setText(item.name());

    m_pUi->stackNumNotebooksLineEdit->setText(
        QString::number(std::max(numChildren, 0)));

    setWindowTitle(tr("Notebook stack info"));
}

void NotebookModelItemInfoWidget::keyPressEvent(QKeyEvent * pEvent)
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
