/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include <lib/model/tag/TagModel.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/Compat.h>
#include <quentier/utility/VersionInfo.h>

#include <QAbstractItemView>
#include <QCompleter>
#include <QKeyEvent>
#include <QModelIndex>
#include <QStringListModel>

#include <algorithm>
#include <utility>

namespace quentier {

NewListItemLineEdit::NewListItemLineEdit(
    AbstractItemModel * itemModel, QList<ItemInfo> reservedItems,
    QWidget * parent) :
    QLineEdit{parent},
    m_ui{new Ui::NewListItemLineEdit}, m_itemModel{itemModel},
    m_reservedItems{std::move(reservedItems)},
    m_itemNamesModel{new QStringListModel(this)},
    m_completer{new QCompleter(this)}
{
    m_ui->setupUi(this);
    setPlaceholderText(tr("Click here to add") + QStringLiteral("..."));
    setupCompleter();

    QObject::connect(
        m_itemModel.data(), &AbstractItemModel::rowsInserted, this,
        &NewListItemLineEdit::onModelRowsInserted);

    QObject::connect(
        m_itemModel.data(), &AbstractItemModel::rowsRemoved, this,
        &NewListItemLineEdit::onModelRowsRemoved);

    QObject::connect(
        m_itemModel.data(), &AbstractItemModel::dataChanged, this,
        &NewListItemLineEdit::onModelDataChanged);

    // NOTE: working around what seems to be a Qt bug: when one selects some
    // item from the drop-down menu shown by QCompleter via pressing
    // Return/Enter, the line edit can't be cleared unless one presses Enter
    // once again; see this thread for more details:
    // http://stackoverflow.com/questions/11865129/fail-to-clear-qlineedit-after-selecting-item-from-qcompleter
    QObject::connect(
        m_completer, qOverload<const QString &>(&QCompleter::activated), this,
        &NewListItemLineEdit::clear, Qt::QueuedConnection);

    QNTRACE(
        "widget::NewListItemLineEdit",
        "Created NewListItemLineEdit: " << this);
}

NewListItemLineEdit::~NewListItemLineEdit()
{
    QNTRACE(
        "widget::NewListItemLineEdit",
        "Destroying NewListItemLineEdit: " << this);

    delete m_ui;
}

const QString & NewListItemLineEdit::targetLinkedNotebookGuid() const
{
    return m_targetLinkedNotebookGuid;
}

void NewListItemLineEdit::setTargetLinkedNotebookGuid(
    QString linkedNotebookGuid)
{
    m_targetLinkedNotebookGuid = std::move(linkedNotebookGuid);
}

QList<NewListItemLineEdit::ItemInfo> NewListItemLineEdit::reservedItems()
    const
{
    return m_reservedItems;
}

void NewListItemLineEdit::setReservedItems(QList<ItemInfo> items)
{
    m_reservedItems = std::move(items);
}

void NewListItemLineEdit::addReservedItem(ItemInfo item)
{
    for (const auto & reservedItem: std::as_const(m_reservedItems)) {
        if ((reservedItem.m_name == item.m_name) &&
            (reservedItem.m_linkedNotebookGuid == item.m_linkedNotebookGuid) &&
            (reservedItem.m_linkedNotebookUsername ==
             item.m_linkedNotebookUsername))
        {
            // This item is already reserved, nothing to do
            return;
        }
    }

    m_reservedItems.push_back(std::move(item));
}

void NewListItemLineEdit::removeReservedItem(ItemInfo item)
{
    for (auto it = m_reservedItems.begin(), end = m_reservedItems.end();
         it != end; ++it)
    {
        const auto & reservedItem = *it;

        if ((reservedItem.m_name == item.m_name) &&
            (reservedItem.m_linkedNotebookGuid == item.m_linkedNotebookGuid) &&
            (reservedItem.m_linkedNotebookUsername ==
             item.m_linkedNotebookUsername))
        {
            m_reservedItems.erase(it);
            return;
        }
    }
}

QSize NewListItemLineEdit::sizeHint() const
{
    return QLineEdit::minimumSizeHint();
}

QSize NewListItemLineEdit::minimumSizeHint() const
{
    return QLineEdit::minimumSizeHint();
}

void NewListItemLineEdit::keyPressEvent(QKeyEvent * event)
{
    if (Q_UNLIKELY(!event)) {
        return;
    }

    const int key = event->key();
    QNTRACE(
        "widget::NewListItemLineEdit",
        "NewListItemLineEdit::keyPressEvent: key = " << key);

    if (key == Qt::Key_Tab) {
        QKeyEvent keyEvent{QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier};
        QApplication::sendEvent(this, &keyEvent);
        return;
    }

    QLineEdit::keyPressEvent(event);
}

void NewListItemLineEdit::focusInEvent(QFocusEvent * event)
{
    QNTRACE(
        "widget::NewListItemLineEdit",
        "NewListItemLineEdit::focusInEvent: "
            << this << ", event type = " << event->type()
            << ", reason = " << event->reason());

    QLineEdit::focusInEvent(event);

    if (event->reason() == Qt::ActiveWindowFocusReason) {
        QNTRACE(
            "widget::NewListItemLineEdit",
            "Received focus from the window system");
        Q_EMIT receivedFocusFromWindowSystem();
    }
}

void NewListItemLineEdit::onModelRowsInserted(
    const QModelIndex & parent, const int start, const int end)
{
    QNDEBUG(
        "widget::NewListItemLineEdit",
        "NewListItemLineEdit::onModelRowsInserted");

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    setupCompleter();
}

void NewListItemLineEdit::onModelRowsRemoved(
    const QModelIndex & parent, const int start, const int end)
{
    QNDEBUG(
        "widget::NewListItemLineEdit",
        "NewListItemLineEdit::onModelRowsRemoved");

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    setupCompleter();
}

void NewListItemLineEdit::onModelDataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight,
    const QVector<int> & roles)
{
    QNDEBUG(
        "widget::NewListItemLineEdit",
        "NewListItemLineEdit::onModelDataChanged");

    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)

    setupCompleter();
}

void NewListItemLineEdit::setupCompleter()
{
    QNDEBUG(
        "widget::NewListItemLineEdit",
        "NewListItemLineEdit::setupCompleter");

    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);

    const auto itemNames = itemNamesForCompleter();
    m_itemNamesModel->setStringList(itemNames);

    QNTRACE(
        "widget::NewListItemLineEdit",
        "The item names to set to the completer: "
            << itemNames.join(QStringLiteral(", ")));

    m_completer->setModel(m_itemNamesModel);
    setCompleter(m_completer);

#ifdef LIB_QUENTIER_USE_QT_WEB_ENGINE
    QNDEBUG(
        "widget::NewListItemLineEdit",
        "Working around Qt bug "
            << "https://bugreports.qt.io/browse/QTBUG-56652");

    m_completer->setCompletionMode(QCompleter::InlineCompletion);
#endif
}

QStringList NewListItemLineEdit::itemNamesForCompleter() const
{
    if (m_itemModel.isNull()) {
        return {};
    }

    QStringList itemNames;

    if (!m_targetLinkedNotebookGuid.isNull()) {
        itemNames = m_itemModel->itemNames(m_targetLinkedNotebookGuid);

        if (!m_targetLinkedNotebookGuid.isEmpty()) {
            const QString linkedNotebookUsername =
                m_itemModel->linkedNotebookUsername(
                    m_targetLinkedNotebookGuid);

            for (auto & itemName: itemNames) {
                itemName += QStringLiteral(" \\ @");
                itemName += linkedNotebookUsername;
            }
        }
    }
    else {
        // First list item names corresponding to user's own account
        itemNames = m_itemModel->itemNames(QLatin1String(""));

        // Now add items corresponding to linked notebooks
        const auto linkedNotebooksInfo = m_itemModel->linkedNotebooksInfo();
        for (const auto & info: std::as_const(linkedNotebooksInfo)) {
            auto linkedNotebookItemNames = m_itemModel->itemNames(info.m_guid);
            for (auto & itemName: linkedNotebookItemNames) {
                itemName += QStringLiteral(" \\ @");
                itemName += info.m_username;
            }

            itemNames << linkedNotebookItemNames;
        }
    }

    std::sort(itemNames.begin(), itemNames.end());
    return itemNames;
}

} // namespace quentier
