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

#include "SavedSearchModelItemInfoWidget.h"
#include "ui_SavedSearchModelItemInfoWidget.h"

#include <lib/model/saved_search/SavedSearchModel.h>

#include <QKeyEvent>

namespace quentier {

SavedSearchModelItemInfoWidget::SavedSearchModelItemInfoWidget(
    const QModelIndex & index, QWidget * parent) :
    QWidget{parent, Qt::Window},
    m_ui{new Ui::SavedSearchModelItemInfoWidget}
{
    m_ui->setupUi(this);

    setWindowTitle(tr("Saved search info"));
    setCheckboxesReadOnly();

    QObject::connect(
        m_ui->okButton, &QPushButton::clicked, this,
        &SavedSearchModelItemInfoWidget::close);

    if (Q_UNLIKELY(!index.isValid())) {
        setInvalidIndex();
        return;
    }

    const auto * savedSearchModel =
        qobject_cast<const SavedSearchModel *>(index.model());

    if (Q_UNLIKELY(!savedSearchModel)) {
        setNonSavedSearchModel();
        return;
    }

    const auto * item = savedSearchModel->itemForIndex(index);
    if (Q_UNLIKELY(!item)) {
        setNoModelItem();
        return;
    }

    const auto * savedSearchItem = item->cast<SavedSearchItem>();
    if (Q_UNLIKELY(!savedSearchItem)) {
        setNoModelItem();
        return;
    }

    setSavedSearchItem(*savedSearchItem);
}

SavedSearchModelItemInfoWidget::~SavedSearchModelItemInfoWidget()
{
    delete m_ui;
}

void SavedSearchModelItemInfoWidget::setCheckboxesReadOnly()
{
    const auto setCheckboxReadOnly = [](QCheckBox & checkbox) {
        checkbox.setAttribute(
            Qt::WA_TransparentForMouseEvents, true);
        checkbox.setFocusPolicy(Qt::NoFocus);
    };

    setCheckboxReadOnly(*m_ui->savedSearchSynchronizableCheckBox);
    setCheckboxReadOnly(*m_ui->savedSearchDirtyCheckBox);
    setCheckboxReadOnly(*m_ui->savedSearchFavoritedCheckBox);
}

void SavedSearchModelItemInfoWidget::setNonSavedSearchModel()
{
    hideAll();

    m_ui->statusBarLabel->setText(
        tr("Non-saved search model is used on the view"));

    m_ui->statusBarLabel->show();
}

void SavedSearchModelItemInfoWidget::setInvalidIndex()
{
    hideAll();

    m_ui->statusBarLabel->setText(tr("No saved search is selected"));
    m_ui->statusBarLabel->show();
}

void SavedSearchModelItemInfoWidget::setNoModelItem()
{
    hideAll();

    m_ui->statusBarLabel->setText(
        tr("No saved search model item was found for index"));

    m_ui->statusBarLabel->show();
}

void SavedSearchModelItemInfoWidget::setSavedSearchItem(
    const SavedSearchItem & item)
{
    m_ui->statusBarLabel->hide();

    m_ui->savedSearchNameLineEdit->setText(item.name());
    m_ui->savedSearchQueryPlainTextEdit->setPlainText(item.query());
    m_ui->savedSearchLocalIdLineEdit->setText(item.localId());
    m_ui->savedSearchGuidLineEdit->setText(item.guid());

    m_ui->savedSearchSynchronizableCheckBox->setChecked(
        item.isSynchronizable());

    m_ui->savedSearchDirtyCheckBox->setChecked(item.isDirty());
    m_ui->savedSearchFavoritedCheckBox->setChecked(item.isFavorited());

    setMinimumWidth(475);
}

void SavedSearchModelItemInfoWidget::hideAll()
{
    m_ui->savedSearchLabel->setHidden(true);
    m_ui->savedSearchNameLabel->setHidden(true);
    m_ui->savedSearchNameLineEdit->setHidden(true);
    m_ui->savedSearchQueryLabel->setHidden(true);
    m_ui->savedSearchQueryPlainTextEdit->setHidden(true);
    m_ui->savedSearchLocalIdLabel->setHidden(true);
    m_ui->savedSearchLocalIdLineEdit->setHidden(true);
    m_ui->savedSearchGuidLabel->setHidden(true);
    m_ui->savedSearchGuidLineEdit->setHidden(true);
    m_ui->savedSearchSynchronizableLabel->setHidden(true);
    m_ui->savedSearchSynchronizableCheckBox->setHidden(true);
    m_ui->savedSearchDirtyLabel->setHidden(true);
    m_ui->savedSearchDirtyCheckBox->setHidden(true);
    m_ui->savedSearchFavoritedLabel->setHidden(true);
    m_ui->savedSearchFavoritedCheckBox->setHidden(true);
}

void SavedSearchModelItemInfoWidget::keyPressEvent(QKeyEvent * event)
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
