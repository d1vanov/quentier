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
#include <QModelIndex>
#include <QStringListModel>
#include <algorithm>

namespace quentier {

NewListItemLineEdit::NewListItemLineEdit(ItemModel * pItemModel,
                                         const QStringList & reservedItemNames,
                                         QWidget * parent) :
    QLineEdit(parent),
    m_pUi(new Ui::NewListItemLineEdit),
    m_pItemModel(pItemModel),
    m_reservedItemNames(reservedItemNames),
    m_pItemNamesModel(new QStringListModel(this)),
    m_pCompleter(new QCompleter(this))
{
    m_pUi->setupUi(this);
    setPlaceholderText(tr("Click here to add") + QStringLiteral("..."));
    setupCompleter();

    QObject::connect(m_pItemModel.data(), QNSIGNAL(ItemModel,rowsInserted,const QModelIndex&,int,int),
                     this, QNSLOT(NewListItemLineEdit,onModelRowsInserted,const QModelIndex&,int,int));
    QObject::connect(m_pItemModel.data(), QNSIGNAL(ItemModel,rowsRemoved,const QModelIndex&,int,int),
                     this, QNSLOT(NewListItemLineEdit,onModelRowsRemoved,const QModelIndex&,int,int));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QObject::connect(m_pItemModel.data(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     this, SLOT(onModelDataChanged(QModelIndex,QModelIndex)));
#else
    QObject::connect(m_pItemModel.data(), &ItemModel::dataChanged, this, &NewListItemLineEdit::onModelDataChanged);
#endif
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

void NewListItemLineEdit::onModelRowsInserted(const QModelIndex & parent, int start, int end)
{
    QNDEBUG(QStringLiteral("NewListItemLineEdit::onModelRowsInserted"));

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    setupCompleter();
}

void NewListItemLineEdit::onModelRowsRemoved(const QModelIndex & parent, int start, int end)
{
    QNDEBUG(QStringLiteral("NewListItemLineEdit::onModelRowsRemoved"));

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    setupCompleter();
}

void NewListItemLineEdit::onModelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                                             )
#else
                                             , const QVector<int> & roles)
#endif
{
    QNDEBUG(QStringLiteral("NewListItemLineEdit::onModelDataChanged"));

    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_UNUSED(roles)
#endif

    setupCompleter();
}

void NewListItemLineEdit::setupCompleter()
{
    QNDEBUG(QStringLiteral("NewListItemLineEdit::setupCompleter"));

    m_pCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_pCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);

    QStringList itemNames;
    if (!m_pItemModel.isNull())
    {
        itemNames = m_pItemModel->itemNames();
        for(auto it = m_reservedItemNames.constBegin(), end = m_reservedItemNames.constEnd(); it != end; ++it)
        {
            auto nit = std::lower_bound(itemNames.constBegin(), itemNames.constEnd(), *it);
            if ((nit != itemNames.constEnd()) && (*nit == *it)) {
                int offset = static_cast<int>(std::distance(itemNames.constBegin(), nit));
                Q_UNUSED(itemNames.erase(QStringList::iterator(itemNames.begin() + offset)))
            }
        }
    }

    if (!itemNames.isEmpty()) {
        m_pItemNamesModel->setStringList(itemNames);
        m_pCompleter->setModel(m_pItemNamesModel);
    }

    setCompleter(m_pCompleter);
}

} // namespace quentier
