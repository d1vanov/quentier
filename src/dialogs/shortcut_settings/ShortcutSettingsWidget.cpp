/*
 * Copyright 2017 Dmitry Ivanov
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

/**
 * The contents of this file were heavily inspired by the source code of Qt Creator,
 * a Qt and C/C++ IDE by The Qt Company Ltd., 2016. That source code is licensed under GNU GPL v.3
 * license with two permissive exceptions although they don't apply to this derived work.
 */

#include "ShortcutSettingsWidget.h"
#include "ShortcutButton.h"

using quentier::ShortcutButton;
#include "ui_ShortcutSettingsWidget.h"

#include <quentier/types/ErrorString.h>
#include <quentier/utility/ShortcutManager.h>
#include <quentier/logging/QuentierLogger.h>
#include <QMenu>
#include <QAction>

namespace quentier {

ShortcutSettingsWidget::ShortcutSettingsWidget(QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::ShortcutSettingsWidget),
    m_pShortcutManager(),
    m_shortcutItems()
{
    m_pUi->setupUi(this);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    m_pUi->filterLineEdit->setClearButtonEnabled(true);
#endif

    m_pUi->warningLabel->setHidden(true);

    m_pUi->actionsTreeWidget->setRootIsDecorated(false);
    m_pUi->actionsTreeWidget->setUniformRowHeights(true);
    m_pUi->actionsTreeWidget->setSortingEnabled(true);
    m_pUi->actionsTreeWidget->setColumnCount(3);

    QTreeWidgetItem * pItem = m_pUi->actionsTreeWidget->headerItem();
    pItem->setText(0, tr("Action"));
    pItem->setText(1, tr("Context"));
    pItem->setText(2, tr("Shortcut"));

    m_pUi->keySequenceLabel->setToolTip(QStringLiteral("<html><body>") +
#if Q_OS_MAC
                tr("Use \"Cmd\", \"Opt\", \"Ctrl\", and \"Shift\" for modifier keys. "
                   "Use \"Escape\", \"Backspace\", \"Delete\", \"Insert\", \"Home\", and so on, for special keys. "
                   "Combine individual keys with \"+\", "
                   "and combine multiple shortcuts to a shortcut sequence with \",\". "
                   "For example, if the user must hold the Ctrl and Shift modifier keys "
                   "while pressing Escape, and then release and press A, "
                   "enter \"Ctrl+Shift+Escape,A\".") +
#else
                tr("Use \"Ctrl\", \"Alt\", \"Meta\", and \"Shift\" for modifier keys. "
                   "Use \"Escape\", \"Backspace\", \"Delete\", \"Insert\", \"Home\", and so on, for special keys. "
                   "Combine individual keys with \"+\", "
                   "and combine multiple shortcuts to a shortcut sequence with \",\". "
                   "For example, if the user must hold the Ctrl and Shift modifier keys "
                   "while pressing Escape, and then release and press A, "
                   "enter \"Ctrl+Shift+Escape,A\".") +
#endif
                QStringLiteral("</html></body>"));

    QObject::connect(m_pUi->resetAllPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(ShortcutSettingsWidget,resetAll));
    QObject::connect(m_pUi->resetPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(ShortcutSettingsWidget,resetToDefault));
    QObject::connect(m_pUi->recordPushButton, QNSIGNAL(ShortcutButton,keySequenceChanged,QKeySequence),
                     this, QNSLOT(ShortcutSettingsWidget,setKeySequence,QKeySequence));
    QObject::connect(m_pUi->warningLabel, QNSIGNAL(QLabel,linkActivated,QString),
                     this, QNSLOT(ShortcutSettingsWidget,showConflicts,QString));
    QObject::connect(m_pUi->filterLineEdit, QNSIGNAL(QLineEdit,textChanged,QString),
                     this, QNSLOT(ShortcutSettingsWidget,onActionFilterChanged,QString));
    QObject::connect(m_pUi->actionsTreeWidget, QNSIGNAL(QTreeWidget,currentItemChanged,QTreeWidgetItem*),
                     this, QNSLOT(ShortcutSettingsWidget,onCurrentActionChanged,QTreeWidgetItem*));
}

ShortcutSettingsWidget::~ShortcutSettingsWidget()
{
    delete m_pUi;
}

void ShortcutSettingsWidget::initialize(ShortcutManager * pShortcutManager,
                                        const QList<QMenu*> & menusWithActions)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::initialize"));

    clear();

    if (Q_UNLIKELY(!pShortcutManager)) {
        ErrorString error(QT_TR_NOOP("Detected null pointer to ShortcutManager passed to ShortcutSettingsWidget"));
        QNWARNING(error);
        return;
    }

    m_pShortcutManager = pShortcutManager;

    QMap<QString, QTreeWidgetItem*> sections;

    for(auto it = menusWithActions.constBegin(),
        end = menusWithActions.constEnd(); it != end; ++it)
    {
        const QMenu * pMenu = *it;
        if (Q_UNLIKELY(!pMenu)) {
            QNWARNING(QStringLiteral("Skipping null pointer to menu"));
            continue;
        }

        QString menuName = pMenu->title();
        if (Q_UNLIKELY(menuName.isEmpty())) {
            QNWARNING(QStringLiteral("Skipping menu with empty name"));
            continue;
        }

        if (!sections.contains(menuName))
        {
            QTreeWidgetItem * pMenuItem = new QTreeWidgetItem(m_pUi->actionsTreeWidget,
                                                              QStringList(menuName));
            QFont font = pMenuItem->font(0);
            font.setBold(true);
            pMenuItem->setFont(0, font);

            Q_UNUSED(sections.insert(menuName, pMenuItem))
            m_pUi->actionsTreeWidget->expandItem(pMenuItem);
        }

        QList<QAction*> actions = pMenu->actions();
        for(auto ait = actions.constBegin(), aend = actions.constEnd(); ait != aend; ++ait)
        {
            const QAction * pAction = *ait;
            if (Q_UNLIKELY(!pAction)) {
                QNWARNING(QStringLiteral("Skipping null pointer to QAction for menu ") << menuName);
                continue;
            }

            // TODO: process action
        }

        // TODO: process further
    }

    // TODO: implement further
}

void ShortcutSettingsWidget::apply()
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::apply"));
    // TODO: implement
}

void ShortcutSettingsWidget::onCurrentActionChanged(QTreeWidgetItem * pCurrentItem)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::onCurrentActionChanged"));

    // TODO: implement
    Q_UNUSED(pCurrentItem)
}

void ShortcutSettingsWidget::resetToDefault()
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::resetToDefault"));
    // TODO: implement
}

void ShortcutSettingsWidget::resetAll()
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::resetAll"));
    // TODO: implement
}

void ShortcutSettingsWidget::onActionFilterChanged(const QString & filter)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::onActionFilterChanged: filter = ") << filter);
    // TODO: implement
}

bool ShortcutSettingsWidget::validateShortcutEdit() const
{
    // TODO: implement
    return false;
}

bool ShortcutSettingsWidget::markCollisions(ShortcutItem * pItem)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::markCollisions"));

    // TODO: implement
    Q_UNUSED(pItem)
    return false;
}

void ShortcutSettingsWidget::setKeySequence(const QKeySequence & key)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::setKeySequence: ") << key);
    // TODO: implement
}

void ShortcutSettingsWidget::showConflicts(const QString &)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::showConflicts"));
    // TODO: implement
}

void ShortcutSettingsWidget::clear()
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::clear"));
    // TODO: implement
}

} // namespace quentier
