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

#include "SavedSearchModelItemInfoWidget.h"
#include "ui_SavedSearchModelItemInfoWidget.h"

#include <lib/model/saved_search/SavedSearchModel.h>

#include <QKeyEvent>

namespace quentier {

SavedSearchModelItemInfoWidget::SavedSearchModelItemInfoWidget(
    const QModelIndex & index, QWidget * parent) :
    QWidget(parent, Qt::Window),
    m_pUi(new Ui::SavedSearchModelItemInfoWidget)
{
    m_pUi->setupUi(this);

    setWindowTitle(tr("Saved search info"));
    setCheckboxesReadOnly();

    QObject::connect(
        m_pUi->okButton, &QPushButton::clicked, this,
        &SavedSearchModelItemInfoWidget::close);

    if (Q_UNLIKELY(!index.isValid())) {
        setInvalidIndex();
        return;
    }

    const auto * pSavedSearchModel =
        qobject_cast<const SavedSearchModel *>(index.model());

    if (Q_UNLIKELY(!pSavedSearchModel)) {
        setNonSavedSearchModel();
        return;
    }

    const auto * pItem = pSavedSearchModel->itemForIndex(index);
    if (Q_UNLIKELY(!pItem)) {
        setNoModelItem();
        return;
    }

    const auto * pSavedSearchItem = pItem->cast<SavedSearchItem>();
    if (Q_UNLIKELY(!pSavedSearchItem)) {
        setNoModelItem();
        return;
    }

    setSavedSearchItem(*pSavedSearchItem);
}

SavedSearchModelItemInfoWidget::~SavedSearchModelItemInfoWidget()
{
    delete m_pUi;
}

void SavedSearchModelItemInfoWidget::setCheckboxesReadOnly()
{
#define SET_CHECKBOX_READ_ONLY(name)                                           \
    m_pUi->savedSearch##name##CheckBox->setAttribute(                          \
        Qt::WA_TransparentForMouseEvents, true);                               \
    m_pUi->savedSearch##name##CheckBox->setFocusPolicy(Qt::NoFocus)

    SET_CHECKBOX_READ_ONLY(Synchronizable);
    SET_CHECKBOX_READ_ONLY(Dirty);
    SET_CHECKBOX_READ_ONLY(Favorited);

#undef SET_CHECKBOX_READ_ONLY
}

void SavedSearchModelItemInfoWidget::setNonSavedSearchModel()
{
    hideAll();

    m_pUi->statusBarLabel->setText(
        tr("Non-saved search model is used on the view"));

    m_pUi->statusBarLabel->show();
}

void SavedSearchModelItemInfoWidget::setInvalidIndex()
{
    hideAll();

    m_pUi->statusBarLabel->setText(tr("No saved search is selected"));
    m_pUi->statusBarLabel->show();
}

void SavedSearchModelItemInfoWidget::setNoModelItem()
{
    hideAll();

    m_pUi->statusBarLabel->setText(
        tr("No saved search model item was found for index"));

    m_pUi->statusBarLabel->show();
}

void SavedSearchModelItemInfoWidget::setSavedSearchItem(
    const SavedSearchItem & item)
{
    m_pUi->statusBarLabel->hide();

    m_pUi->savedSearchNameLineEdit->setText(item.name());
    m_pUi->savedSearchQueryPlainTextEdit->setPlainText(item.query());
    m_pUi->savedSearchLocalIdLineEdit->setText(item.localId());
    m_pUi->savedSearchGuidLineEdit->setText(item.guid());

    m_pUi->savedSearchSynchronizableCheckBox->setChecked(
        item.isSynchronizable());

    m_pUi->savedSearchDirtyCheckBox->setChecked(item.isDirty());
    m_pUi->savedSearchFavoritedCheckBox->setChecked(item.isFavorited());

    setMinimumWidth(475);
}

void SavedSearchModelItemInfoWidget::hideAll()
{
    m_pUi->savedSearchLabel->setHidden(true);
    m_pUi->savedSearchNameLabel->setHidden(true);
    m_pUi->savedSearchNameLineEdit->setHidden(true);
    m_pUi->savedSearchQueryLabel->setHidden(true);
    m_pUi->savedSearchQueryPlainTextEdit->setHidden(true);
    m_pUi->savedSearchLocalIdLabel->setHidden(true);
    m_pUi->savedSearchLocalIdLineEdit->setHidden(true);
    m_pUi->savedSearchGuidLabel->setHidden(true);
    m_pUi->savedSearchGuidLineEdit->setHidden(true);
    m_pUi->savedSearchSynchronizableLabel->setHidden(true);
    m_pUi->savedSearchSynchronizableCheckBox->setHidden(true);
    m_pUi->savedSearchDirtyLabel->setHidden(true);
    m_pUi->savedSearchDirtyCheckBox->setHidden(true);
    m_pUi->savedSearchFavoritedLabel->setHidden(true);
    m_pUi->savedSearchFavoritedCheckBox->setHidden(true);
}

void SavedSearchModelItemInfoWidget::keyPressEvent(QKeyEvent * pEvent)
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
