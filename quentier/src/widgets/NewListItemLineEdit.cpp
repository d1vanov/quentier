/*
 * Copyright 2016 Dmitry Ivanov
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

#include "NewListItemLineEdit.h"
#include "ui_NewListItemLineEdit.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QKeyEvent>
#include <QCompleter>
#include <QStringListModel>
#include <algorithm>

namespace quentier {

NewListItemLineEdit::NewListItemLineEdit(ItemModel * pItemModel, const QStringList & reservedItemNames, QWidget * parent) :
    QLineEdit(parent),
    m_pUi(new Ui::NewListItemLineEdit),
    m_pItemModel(pItemModel),
    m_reservedItemNames(reservedItemNames),
    m_pItemNamesModel(new QStringListModel(this)),
    m_pCompleter(new QCompleter(this))
{
    m_pUi->setupUi(this);
    setPlaceholderText(QStringLiteral("+"));
    setupCompleter();
}

NewListItemLineEdit::~NewListItemLineEdit()
{
    delete m_pUi;
}

void NewListItemLineEdit::updateReservedItemNames(const QStringList & reservedItemNames)
{
    QNDEBUG(QStringLiteral("NewListItemLineEdit::updateReservedItemNames: ")
            << reservedItemNames.join(QStringLiteral(", ")));

    m_reservedItemNames = reservedItemNames;
    setupCompleter();
}

void NewListItemLineEdit::keyPressEvent(QKeyEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return;
    }

    int key = pEvent->key();
    QNTRACE(QStringLiteral("NewListItemLineEdit::keyPressEvent: key = ") << key);

    if (key == Qt::Key_Tab) {
        emit editingFinished();
        return;
    }

    QLineEdit::keyPressEvent(pEvent);
}

void NewListItemLineEdit::setupCompleter()
{
    QNDEBUG(QStringLiteral("NewListItemLineEdit::setupCompleter"));

    m_pCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_pCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);

    QStringList itemNames = m_pItemModel->itemNames();
    for(auto it = m_reservedItemNames.constBegin(), end = m_reservedItemNames.constEnd(); it != end; ++it)
    {
        auto nit = std::lower_bound(itemNames.constBegin(), itemNames.constEnd(), *it);
        if ((nit != itemNames.constEnd()) && (*nit == *it)) {
            int offset = static_cast<int>(std::distance(itemNames.constBegin(), nit));
            Q_UNUSED(itemNames.erase(QStringList::iterator(itemNames.begin() + offset)))
        }
    }

    m_pItemNamesModel->setStringList(itemNames);
    m_pCompleter->setModel(m_pItemNamesModel);
    setCompleter(m_pCompleter);
}

} // namespace quentier
