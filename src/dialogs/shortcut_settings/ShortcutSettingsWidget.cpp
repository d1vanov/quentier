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
#include <QtAlgorithms>

Q_DECLARE_METATYPE(quentier::ShortcutItem*)

namespace quentier {

ShortcutSettingsWidget::ShortcutSettingsWidget(QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::ShortcutSettingsWidget),
    m_pShortcutManager(),
    m_currentAccount(),
    m_shortcutItems(),
    m_actionNameToShortcutInfo()
{
    m_pUi->setupUi(this);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    m_pUi->filterLineEdit->setClearButtonEnabled(true);
#endif

    m_pUi->warningLabel->setHidden(true);

    m_pUi->actionsTreeWidget->setRootIsDecorated(false);
    m_pUi->actionsTreeWidget->setUniformRowHeights(true);
    m_pUi->actionsTreeWidget->setSortingEnabled(true);
    m_pUi->actionsTreeWidget->setColumnCount(2);

    QTreeWidgetItem * pItem = m_pUi->actionsTreeWidget->headerItem();
    pItem->setText(0, tr("Action"));
    pItem->setText(1, tr("Shortcut"));

    m_pUi->keySequenceLabel->setToolTip(QStringLiteral("<html><body>") +
#ifdef Q_OS_MAC
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

    mapActionNamesToShortcutKeys();
}

ShortcutSettingsWidget::~ShortcutSettingsWidget()
{
    delete m_pUi;
    qDeleteAll(m_shortcutItems);
}

void ShortcutSettingsWidget::initialize(const Account & account, ShortcutManager * pShortcutManager)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::initialize: account = ") << account);

    clear();

    m_currentAccount = account;

    if (Q_UNLIKELY(!pShortcutManager)) {
        ErrorString error(QT_TR_NOOP("Detected null pointer to ShortcutManager passed to ShortcutSettingsWidget"));
        QNWARNING(error);
        return;
    }

    m_pShortcutManager = pShortcutManager;

    QMap<QString, QTreeWidgetItem*> sections;

    for(auto it = m_actionNameToShortcutInfo.constBegin(),
        end = m_actionNameToShortcutInfo.constEnd(); it != end; ++it)
    {
        const QString & actionName = it.key();
        const ShortcutInfo & shortcutInfo = it.value();

        ShortcutItem * pShortcutItem = new ShortcutItem;
        m_shortcutItems << pShortcutItem;
        pShortcutItem->m_actionKey = shortcutInfo.m_key;
        pShortcutItem->m_nonStandardActionKey = shortcutInfo.m_nonStandardKey;
        pShortcutItem->m_keySequence = ((pShortcutItem->m_actionKey >= 0)
                                        ? m_pShortcutManager->shortcut(pShortcutItem->m_actionKey, m_currentAccount,
                                                                       shortcutInfo.m_context)
                                        : m_pShortcutManager->shortcut(pShortcutItem->m_nonStandardActionKey,
                                                                       m_currentAccount, shortcutInfo.m_context));
        pShortcutItem->m_pTreeWidgetItem = new QTreeWidgetItem;

        QString sectionName = shortcutInfo.m_context;
        if (sectionName.isEmpty()) {
            sectionName = tr("Actions");
        }

        if (!sections.contains(sectionName))
        {
            QTreeWidgetItem * pMenuItem = new QTreeWidgetItem(m_pUi->actionsTreeWidget, QStringList(sectionName));
            QFont font = pMenuItem->font(0);
            font.setBold(true);
            pMenuItem->setFont(0, font);

            Q_UNUSED(sections.insert(sectionName, pMenuItem))
            m_pUi->actionsTreeWidget->expandItem(pMenuItem);
        }
        sections[sectionName]->addChild(pShortcutItem->m_pTreeWidgetItem);

        pShortcutItem->m_pTreeWidgetItem->setText(0, actionName);
        pShortcutItem->m_pTreeWidgetItem->setText(1, pShortcutItem->m_keySequence.toString(QKeySequence::NativeText));

        bool shortcutModified = false;
        if (!pShortcutItem->m_keySequence.isEmpty())
        {
            QKeySequence defaultShortcut = ((pShortcutItem->m_actionKey >= 0)
                                            ? m_pShortcutManager->defaultShortcut(pShortcutItem->m_actionKey, m_currentAccount,
                                                                                  shortcutInfo.m_context)
                                            : m_pShortcutManager->defaultShortcut(pShortcutItem->m_actionKey, m_currentAccount,
                                                                                  shortcutInfo.m_context));
            if ( defaultShortcut.isEmpty() || (defaultShortcut != pShortcutItem->m_keySequence)) {
                shortcutModified = true;
            }
        }

        setModified(pShortcutItem->m_pTreeWidgetItem, shortcutModified);

        pShortcutItem->m_pTreeWidgetItem->setData(0, Qt::UserRole, qVariantFromValue(pShortcutItem));
        markCollisions(pShortcutItem);
    }
    onActionFilterChanged(m_pUi->filterLineEdit->text());
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

void ShortcutSettingsWidget::mapActionNamesToShortcutKeys()
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::mapActionNamesToShortcutKeys"));

#define PROCESS_ACTION_SHORTCUT(action, key, ...) \
    { \
        ShortcutInfo info; \
        info.m_key = key; \
        info.m_context = QString::fromUtf8("" __VA_ARGS__); \
        m_actionNameToShortcutInfo[ QString::fromUtf8(#action) ] = info; \
        QNTRACE(QStringLiteral("Action name " #action " corresponds to shortcut info: ") << info); \
    }

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, context) \
    { \
        ShortcutInfo info; \
        info.m_nonStandardKey = QString::fromUtf8("" #action); \
        info.m_context = QString::fromUtf8("" context); \
        m_actionNameToShortcutInfo[ QString::fromUtf8(#action) ] = info; \
        QNTRACE(QStringLiteral("Action name " #action " corresponds to non-standard shortcut info: ") << info); \
    }

#include "../../ActionShortcuts.inl"

#undef PROCESS_ACTION_SHORTCUT
#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
}

bool ShortcutSettingsWidget::validateShortcutEdit() const
{
    // TODO: implement
    return false;
}

bool ShortcutSettingsWidget::markCollisions(ShortcutItem * pItem)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::markCollisions"));

    if (Q_UNLIKELY(!pItem)) {
        QNWARNING(QStringLiteral("Detected null pointer to ShortcutItem"));
        return false;
    }

    bool hasCollision = false;
    if (!pItem->m_keySequence.isEmpty())
    {
        for(auto it = m_shortcutItems.constBegin(), end = m_shortcutItems.constEnd(); it != end; ++it)
        {
            const ShortcutItem * pCurrentItem = *it;
            if (Q_UNLIKELY(!pCurrentItem)) {
                QNWARNING(QStringLiteral("Skipping null pointer to ShortcutItem"));
                continue;
            }

            if (Q_UNLIKELY(!pCurrentItem->m_pTreeWidgetItem)) {
                QNWARNING(QStringLiteral("Skipping ShortcutItem with null pointer to QTreeWidgetItem"));
                continue;
            }

            if (!pCurrentItem->m_keySequence.isEmpty() && (pCurrentItem->m_keySequence != pItem->m_keySequence)) {
                QNDEBUG(QStringLiteral("Found item with conflicting shortcut: ") << pCurrentItem->m_pTreeWidgetItem->text(0));
                pCurrentItem->m_pTreeWidgetItem->setForeground(1, Qt::red);
                hasCollision = true;
            }
        }
    }

    if (pItem->m_pTreeWidgetItem) {
        pItem->m_pTreeWidgetItem->setForeground(1, (hasCollision ? Qt::red : palette().foreground().color()));
    }

    return hasCollision;
}

void ShortcutSettingsWidget::setModified(QTreeWidgetItem * pItem, bool modified)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::setModified"));

    if (Q_UNLIKELY(!pItem)) {
        QNWARNING(QStringLiteral("Detected null pointer to QTreeWidgetItem"));
        return;
    }

    QFont font = pItem->font(0);
    font.setItalic(modified);
    pItem->setFont(0, font);
    font.setBold(modified);
    pItem->setFont(1, font);
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

QTextStream & ShortcutSettingsWidget::ShortcutInfo::print(QTextStream & strm) const
{
    strm << QStringLiteral("Shortcut info: key = ")
         << m_key << QStringLiteral(", non-standard key = ")
         << (m_nonStandardKey.isEmpty() ? QStringLiteral("<empty>") : m_nonStandardKey)
         << QStringLiteral(", context = ")
         << (m_context.isEmpty() ? QStringLiteral("<empty>") : m_context);
    return strm;
}

} // namespace quentier
