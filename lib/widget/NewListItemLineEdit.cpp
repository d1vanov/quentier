/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#include <lib/model/TagModel.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/VersionInfo.h>

#include <QKeyEvent>
#include <QCompleter>
#include <QModelIndex>
#include <QStringListModel>
#include <QAbstractItemView>

#include <algorithm>

namespace quentier {

NewListItemLineEdit::NewListItemLineEdit(
        ItemModel * pItemModel,
        const QStringList & reservedItemNames,
        const QString & linkedNotebookGuid,
        QWidget * parent) :
    QLineEdit(parent),
    m_pUi(new Ui::NewListItemLineEdit),
    m_pItemModel(pItemModel),
    m_reservedItemNames(reservedItemNames),
    m_linkedNotebookGuid(linkedNotebookGuid),
    m_pItemNamesModel(new QStringListModel(this)),
    m_pCompleter(new QCompleter(this))
{
    m_pUi->setupUi(this);
    setPlaceholderText(tr("Click here to add") + QStringLiteral("..."));
    setupCompleter();

    QObject::connect(m_pItemModel.data(),
                     QNSIGNAL(ItemModel,rowsInserted,
                              const QModelIndex&,int,int),
                     this,
                     QNSLOT(NewListItemLineEdit,onModelRowsInserted,
                            const QModelIndex&,int,int));
    QObject::connect(m_pItemModel.data(),
                     QNSIGNAL(ItemModel,rowsRemoved,
                              const QModelIndex&,int,int),
                     this,
                     QNSLOT(NewListItemLineEdit,onModelRowsRemoved,
                            const QModelIndex&,int,int));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QObject::connect(m_pItemModel.data(),
                     SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     this,
                     SLOT(onModelDataChanged(QModelIndex,QModelIndex)));
#else
    QObject::connect(m_pItemModel.data(), &ItemModel::dataChanged,
                     this, &NewListItemLineEdit::onModelDataChanged);
#endif

    // NOTE: working around what seems to be a Qt bug: when one selects some item
    // from the drop-down menu shown by QCompleter via pressing Return/Enter,
    // the line edit can't be cleared unless one presses Enter once again;
    // see this thread for more details:
    // http://stackoverflow.com/questions/11865129/fail-to-clear-qlineedit-after-selecting-item-from-qcompleter

    QObject::connect(m_pCompleter, SIGNAL(activated(const QString&)),
                     this, SLOT(clear()), Qt::QueuedConnection);

    QNTRACE("Created NewListItemLineEdit: " << this);
}

NewListItemLineEdit::~NewListItemLineEdit()
{
    QNTRACE("Destroying NewListItemLineEdit: " << this);
    delete m_pUi;
}

QStringList NewListItemLineEdit::reservedItemNames() const
{
    return m_reservedItemNames;
}

void NewListItemLineEdit::updateReservedItemNames(
    const QStringList & reservedItemNames)
{
    QNDEBUG("NewListItemLineEdit::updateReservedItemNames: "
            << reservedItemNames.join(QStringLiteral(", ")));

    m_reservedItemNames = reservedItemNames;
    setupCompleter();
}

QString NewListItemLineEdit::linkedNotebookGuid() const
{
    return m_linkedNotebookGuid;
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
    const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    )
#else
    , const QVector<int> & roles)
#endif
{
    QNDEBUG("NewListItemLineEdit::onModelDataChanged");

    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_UNUSED(roles)
#endif

    setupCompleter();
}

void NewListItemLineEdit::setupCompleter()
{
    QNDEBUG("NewListItemLineEdit::setupCompleter: reserved item names: "
            << m_reservedItemNames.join(QStringLiteral(", ")));

    m_pCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_pCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);

    QStringList itemNames;
    if (!m_pItemModel.isNull())
    {
        itemNames = m_pItemModel->itemNames(m_linkedNotebookGuid);
        QNTRACE("Model item names: " << itemNames.join(QStringLiteral(", ")));

        for(auto it = m_reservedItemNames.constBegin(),
            end = m_reservedItemNames.constEnd(); it != end; ++it)
        {
            auto nit = std::lower_bound(
                itemNames.constBegin(), itemNames.constEnd(), *it);
            if ((nit != itemNames.constEnd()) && (*nit == *it)) {
                int offset = static_cast<int>(
                    std::distance(itemNames.constBegin(), nit));
                Q_UNUSED(itemNames.erase(
                    QStringList::iterator(itemNames.begin() + offset)))
            }
        }
    }

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

} // namespace quentier
