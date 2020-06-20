/*
 * Copyright 2016-2020 Dmitry Ivanov
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
#include <quentier/utility/VersionInfo.h>

#include <QAbstractItemView>
#include <QCompleter>
#include <QKeyEvent>
#include <QModelIndex>
#include <QStringListModel>

#include <algorithm>

namespace quentier {

NewListItemLineEdit::NewListItemLineEdit(
        ItemModel * pItemModel,
        QVector<ItemInfo> reservedItems,
        QWidget * parent) :
    QLineEdit(parent),
    m_pUi(new Ui::NewListItemLineEdit),
    m_pItemModel(pItemModel),
    m_reservedItems(std::move(reservedItems)),
    m_pItemNamesModel(new QStringListModel(this)),
    m_pCompleter(new QCompleter(this))
{
    m_pUi->setupUi(this);
    setPlaceholderText(tr("Click here to add") + QStringLiteral("..."));
    setupCompleter();

    QObject::connect(
        m_pItemModel.data(),
        &ItemModel::rowsInserted,
        this,
        &NewListItemLineEdit::onModelRowsInserted);

    QObject::connect(
        m_pItemModel.data(),
        &ItemModel::rowsRemoved,
        this,
        &NewListItemLineEdit::onModelRowsRemoved);

    QObject::connect(
        m_pItemModel.data(),
        &ItemModel::dataChanged,
        this,
        &NewListItemLineEdit::onModelDataChanged);

    // NOTE: working around what seems to be a Qt bug: when one selects some
    // item from the drop-down menu shown by QCompleter via pressing
    // Return/Enter, the line edit can't be cleared unless one presses Enter
    // once again; see this thread for more details:
    // http://stackoverflow.com/questions/11865129/fail-to-clear-qlineedit-after-selecting-item-from-qcompleter

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pCompleter,
        qOverload<const QString&>(&QCompleter::activated),
        this,
        &NewListItemLineEdit::clear,
        Qt::QueuedConnection);
#else
    QObject::connect(
        m_pCompleter,
        SIGNAL(activated(const QString&)),
        this,
        SLOT(clear()),
        Qt::QueuedConnection);
#endif

    QNTRACE("Created NewListItemLineEdit: " << this);
}

NewListItemLineEdit::~NewListItemLineEdit()
{
    QNTRACE("Destroying NewListItemLineEdit: " << this);
    delete m_pUi;
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

QVector<NewListItemLineEdit::ItemInfo> NewListItemLineEdit::reservedItems() const
{
    return m_reservedItems;
}

void NewListItemLineEdit::setReservedItems(QVector<ItemInfo> items)
{
    m_reservedItems = std::move(items);
}

void NewListItemLineEdit::addReservedItem(ItemInfo item)
{
    for(const auto & reservedItem: qAsConst(m_reservedItems))
    {
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
    for(auto it = m_reservedItems.begin(), end = m_reservedItems.end();
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

void NewListItemLineEdit::keyPressEvent(QKeyEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return;
    }

    int key = pEvent->key();
    QNTRACE("NewListItemLineEdit::keyPressEvent: key = " << key);

    if (key == Qt::Key_Tab) {
        QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
        QApplication::sendEvent(this, &keyEvent);
        return;
    }

    QLineEdit::keyPressEvent(pEvent);
}

void NewListItemLineEdit::focusInEvent(QFocusEvent * pEvent)
{
    QNTRACE("NewListItemLineEdit::focusInEvent: " << this << ", event type = "
            << pEvent->type() << ", reason = " << pEvent->reason());

    QLineEdit::focusInEvent(pEvent);

    if (pEvent->reason() == Qt::ActiveWindowFocusReason) {
        QNTRACE("Received focus from the window system");
        Q_EMIT receivedFocusFromWindowSystem();
    }
}

void NewListItemLineEdit::onModelRowsInserted(
    const QModelIndex & parent, int start, int end)
{
    QNDEBUG("NewListItemLineEdit::onModelRowsInserted");

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    setupCompleter();
}

void NewListItemLineEdit::onModelRowsRemoved(
    const QModelIndex & parent, int start, int end)
{
    QNDEBUG("NewListItemLineEdit::onModelRowsRemoved");

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    setupCompleter();
}

void NewListItemLineEdit::onModelDataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight,
    const QVector<int> & roles)
{
    QNDEBUG("NewListItemLineEdit::onModelDataChanged");

    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)

    setupCompleter();
}

void NewListItemLineEdit::setupCompleter()
{
    QNDEBUG("NewListItemLineEdit::setupCompleter");

    m_pCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_pCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);

    auto itemNames = itemNamesForCompleter();
    m_pItemNamesModel->setStringList(itemNames);

    QNTRACE("The item names to set to the completer: "
        << itemNames.join(QStringLiteral(", ")));

    m_pCompleter->setModel(m_pItemNamesModel);
    setCompleter(m_pCompleter);

#ifdef LIB_QUENTIER_USE_QT_WEB_ENGINE
    QNDEBUG("Working around Qt bug https://bugreports.qt.io/browse/QTBUG-56652");
    m_pCompleter->setCompletionMode(QCompleter::InlineCompletion);
#endif
}

QStringList NewListItemLineEdit::itemNamesForCompleter() const
{
    if (m_pItemModel.isNull()) {
        return {};
    }

    QStringList itemNames;

    if (!m_targetLinkedNotebookGuid.isNull())
    {
        itemNames = m_pItemModel->itemNames(m_targetLinkedNotebookGuid);

        if (!m_targetLinkedNotebookGuid.isEmpty())
        {
            QString linkedNotebookUsername = m_pItemModel->linkedNotebookUsername(
                m_targetLinkedNotebookGuid);

            for(auto & itemName: itemNames) {
                itemName += QStringLiteral(" \\ @");
                itemName += linkedNotebookUsername;
            }
        }
    }
    else
    {
        // First list item names corresponding to user's own account
        itemNames = m_pItemModel->itemNames(QLatin1String(""));

        // Now add items corresponding to linked notebooks
        auto linkedNotebooksInfo = m_pItemModel->linkedNotebooksInfo();
        for(const auto & info: qAsConst(linkedNotebooksInfo))
        {
            auto linkedNotebookItemNames = m_pItemModel->itemNames(info.m_guid);
            for(auto & itemName: linkedNotebookItemNames) {
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
