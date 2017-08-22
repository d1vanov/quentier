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

static QString keySequenceToEditString(const QKeySequence & sequence)
{
    QString text = sequence.toString(QKeySequence::PortableText);
#ifdef Q_OS_MAC
    // adapt modifier names
    text.replace(QStringLiteral("Ctrl"), QStringLiteral("Cmd"), Qt::CaseInsensitive);
    text.replace(QStringLiteral("Alt"), QStringLiteral("Opt"), Qt::CaseInsensitive);
    text.replace(QStringLiteral("Meta"), QStringLiteral("Ctrl"), Qt::CaseInsensitive);
#endif
    return text;
}

static QKeySequence keySequenceFromEditString(const QString & editString)
{
    QString text = editString;
#ifdef Q_OS_MAC
    // adapt modifier names
    text.replace(QStringLiteral("Opt"), QStringLiteral("Alt"), Qt::CaseInsensitive);
    text.replace(QStringLiteral("Ctrl"), QStringLiteral("Meta"), Qt::CaseInsensitive);
    text.replace(QStringLiteral("Cmd"), QStringLiteral("Ctrl"), Qt::CaseInsensitive);
#endif
    return QKeySequence::fromString(text, QKeySequence::PortableText);
}

static bool keySequenceIsValid(const QKeySequence & sequence)
{
    if (sequence.isEmpty()) {
        return false;
    }

    for(int i = 0; i < static_cast<int>(sequence.count()); ++i)
    {
        if (sequence[static_cast<unsigned int>(i)] == Qt::Key_unknown) {
            return false;
        }
    }

    return true;
}

ShortcutSettingsWidget::ShortcutSettingsWidget(QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::ShortcutSettingsWidget),
    m_pShortcutManager(),
    m_currentAccount(),
    m_shortcutItems()
{
    m_pUi->setupUi(this);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    m_pUi->filterLineEdit->setClearButtonEnabled(true);
#endif

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

    m_pUi->warningLabel->hide();

    QObject::connect(m_pUi->resetAllPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(ShortcutSettingsWidget,resetAll));
    QObject::connect(m_pUi->resetPushButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(ShortcutSettingsWidget,resetToDefault));
    QObject::connect(m_pUi->recordPushButton, QNSIGNAL(ShortcutButton,keySequenceChanged,QKeySequence),
                     this, QNSLOT(ShortcutSettingsWidget,setCurrentItemKeySequence,QKeySequence));
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

#define PROCESS_ACTION_SHORTCUT(action, key, ...) \
    { \
        ShortcutItem * pShortcutItem = new ShortcutItem; \
        m_shortcutItems << pShortcutItem; \
        pShortcutItem->m_actionKey = key; \
        pShortcutItem->m_actionName = QString::fromUtf8(#action); \
        pShortcutItem->m_context = QString::fromUtf8("" __VA_ARGS__); \
        QNTRACE(QString::fromUtf8("Action name " #action " corresponds to shortcut item: ") << *pShortcutItem); \
    }

#define PROCESS_NON_STANDARD_ACTION_SHORTCUT(action, context) \
    { \
        ShortcutItem * pShortcutItem = new ShortcutItem; \
        m_shortcutItems << pShortcutItem; \
        pShortcutItem->m_nonStandardActionKey = QString::fromUtf8(#action); \
        pShortcutItem->m_actionName = pShortcutItem->m_nonStandardActionKey; \
        pShortcutItem->m_context = QString::fromUtf8(context); \
        QNTRACE(QString::fromUtf8("Action name " #action " corresponds to shortcut item: ") << *pShortcutItem); \
    }

#include "../../ActionShortcuts.inl"

#undef PROCESS_NON_STANDARD_ACTION_SHORTCUT
#undef PROCESS_ACTION_SHORTCUT

    QMap<QString, QTreeWidgetItem*> sections;

    for(auto it = m_shortcutItems.begin(), end = m_shortcutItems.end(); it != end; ++it)
    {
        ShortcutItem * pShortcutItem = *it;

        pShortcutItem->m_keySequence = ((pShortcutItem->m_actionKey >= 0)
                                        ? m_pShortcutManager->shortcut(pShortcutItem->m_actionKey, m_currentAccount,
                                                                       pShortcutItem->m_context)
                                        : m_pShortcutManager->shortcut(pShortcutItem->m_nonStandardActionKey,
                                                                       m_currentAccount, pShortcutItem->m_context));
        pShortcutItem->m_pTreeWidgetItem = new QTreeWidgetItem;

        QString sectionName = pShortcutItem->m_context;
        if (sectionName.isEmpty()) {
            sectionName = tr("Actions");
            pShortcutItem->m_context = sectionName;
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

        pShortcutItem->m_pTreeWidgetItem->setText(0, pShortcutItem->m_actionName);
        pShortcutItem->m_pTreeWidgetItem->setText(1, pShortcutItem->m_keySequence.toString(QKeySequence::NativeText));

        if (!pShortcutItem->m_keySequence.isEmpty())
        {
            QKeySequence defaultShortcut = ((pShortcutItem->m_actionKey >= 0)
                                            ? m_pShortcutManager->defaultShortcut(pShortcutItem->m_actionKey, m_currentAccount,
                                                                                  pShortcutItem->m_context)
                                            : m_pShortcutManager->defaultShortcut(pShortcutItem->m_actionKey, m_currentAccount,
                                                                                  pShortcutItem->m_context));
            if ( defaultShortcut.isEmpty() || (defaultShortcut != pShortcutItem->m_keySequence)) {
                pShortcutItem->m_isModified = true;
            }
        }

        setModified(*(pShortcutItem->m_pTreeWidgetItem), pShortcutItem->m_isModified);

        pShortcutItem->m_pTreeWidgetItem->setData(0, Qt::UserRole, qVariantFromValue(pShortcutItem));
        markCollisions(*pShortcutItem);
    }
    onActionFilterChanged(m_pUi->filterLineEdit->text());
}

void ShortcutSettingsWidget::apply()
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::apply"));

    for(auto it = m_shortcutItems.begin(), end = m_shortcutItems.end(); it != end; ++it) {
        applyItem(*it);
    }
}

void ShortcutSettingsWidget::onCurrentActionChanged(QTreeWidgetItem * pCurrentItem)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::onCurrentActionChanged"));

    ShortcutItem * pShortcutItem = shortcutItemFromTreeItem(pCurrentItem);
    if (!pShortcutItem) {
        m_pUi->keySequenceLineEdit->clear();
        m_pUi->warningLabel->clear();
        m_pUi->warningLabel->hide();
        m_pUi->shortcutGroupBox->setEnabled(false);
        return;
    }

    setCurrentItemKeySequence(pShortcutItem->m_keySequence);
    markCollisions(*pShortcutItem);
    m_pUi->shortcutGroupBox->setEnabled(true);
}

void ShortcutSettingsWidget::resetToDefault()
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::resetToDefault"));

    QTreeWidgetItem * pCurrentItem = m_pUi->actionsTreeWidget->currentItem();
    ShortcutItem * pShortcutItem = shortcutItemFromTreeItem(pCurrentItem);
    if (Q_UNLIKELY(!pShortcutItem)) {
        QNDEBUG(QStringLiteral("No current shortcut item"));
        return;
    }

    pShortcutItem->m_isModified = false;

    if (Q_UNLIKELY(m_pShortcutManager.isNull())) {
        QNWARNING(QStringLiteral("Can't reset the shortcut to the default one: shortcut manager is expired"));
        return;
    }

    QKeySequence defaultShortcut;

    if (pShortcutItem->m_actionKey >= 0)
    {
        m_pShortcutManager->setUserShortcut(pShortcutItem->m_actionKey, QKeySequence(),
                                            m_currentAccount, pShortcutItem->m_context);
        defaultShortcut = m_pShortcutManager->defaultShortcut(pShortcutItem->m_actionKey, m_currentAccount,
                                                              pShortcutItem->m_context);
    }
    else
    {
        m_pShortcutManager->setNonStandardUserShortcut(pShortcutItem->m_nonStandardActionKey, QKeySequence(),
                                                       m_currentAccount, pShortcutItem->m_context);
        defaultShortcut = m_pShortcutManager->defaultShortcut(pShortcutItem->m_nonStandardActionKey, m_currentAccount,
                                                              pShortcutItem->m_context);
    }

    setCurrentItemKeySequence(defaultShortcut);

    for(auto it = m_shortcutItems.begin(), end = m_shortcutItems.end(); it != end; ++it)
    {
        ShortcutItem * pCurrentShortcutItem = *it;
        if (Q_UNLIKELY(!pCurrentShortcutItem)) {
            QNWARNING(QStringLiteral("Skipping null pointer to ShortcutItem"));
            continue;
        }

        markCollisions(*pCurrentShortcutItem);
    }
}

void ShortcutSettingsWidget::resetAll()
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::resetAll"));

    for(auto it = m_shortcutItems.begin(), end = m_shortcutItems.end(); it != end; ++it)
    {
        ShortcutItem * pCurrentShortcutItem = *it;
        if (Q_UNLIKELY(!pCurrentShortcutItem)) {
            QNWARNING(QStringLiteral("Skipping null pointer to ShortcutItem"));
            continue;
        }

        if (!pCurrentShortcutItem->m_isModified) {
            continue;
        }

        if (pCurrentShortcutItem->m_actionKey >= 0)
        {
            m_pShortcutManager->setUserShortcut(pCurrentShortcutItem->m_actionKey, QKeySequence(),
                                                m_currentAccount, pCurrentShortcutItem->m_context);
            pCurrentShortcutItem->m_keySequence = m_pShortcutManager->defaultShortcut(pCurrentShortcutItem->m_actionKey,
                                                                                      m_currentAccount,
                                                                                      pCurrentShortcutItem->m_context);
        }
        else
        {
            m_pShortcutManager->setNonStandardUserShortcut(pCurrentShortcutItem->m_nonStandardActionKey, QKeySequence(),
                                                           m_currentAccount, pCurrentShortcutItem->m_context);
            pCurrentShortcutItem->m_keySequence = m_pShortcutManager->defaultShortcut(pCurrentShortcutItem->m_nonStandardActionKey,
                                                                                      m_currentAccount,
                                                                                      pCurrentShortcutItem->m_context);
        }

        if (pCurrentShortcutItem->m_pTreeWidgetItem)
        {
            pCurrentShortcutItem->m_pTreeWidgetItem->setText(1, pCurrentShortcutItem->m_keySequence.toString(QKeySequence::NativeText));
            setModified(*(pCurrentShortcutItem->m_pTreeWidgetItem), false);

            if (pCurrentShortcutItem->m_pTreeWidgetItem == m_pUi->actionsTreeWidget->currentItem()) {
                onCurrentActionChanged(pCurrentShortcutItem->m_pTreeWidgetItem);
            }
        }
    }

    for(auto it = m_shortcutItems.begin(), end = m_shortcutItems.end(); it != end; ++it)
    {
        ShortcutItem * pCurrentShortcutItem = *it;
        if (Q_UNLIKELY(!pCurrentShortcutItem)) {
            QNWARNING(QStringLiteral("Skipping null pointer to ShortcutItem"));
            continue;
        }

        markCollisions(*pCurrentShortcutItem);
    }
}

void ShortcutSettingsWidget::onActionFilterChanged(const QString & filter)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::onActionFilterChanged: filter = ") << filter);

    for(int i = 0; i < m_pUi->actionsTreeWidget->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem * pCurrentItem = m_pUi->actionsTreeWidget->topLevelItem(i);
        if (pCurrentItem) {
            Q_UNUSED(filterItem(filter, *pCurrentItem))
        }
    }
}

bool ShortcutSettingsWidget::validateShortcutEdit() const
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::validateShortcutEdit"));

    m_pUi->warningLabel->clear();
    m_pUi->warningLabel->hide();

    QTreeWidgetItem * pCurrentItem = m_pUi->actionsTreeWidget->currentItem();
    ShortcutItem * pShortcutItem = shortcutItemFromTreeItem(pCurrentItem);
    if (!pShortcutItem) {
        return true;
    }

    bool valid = false;

    const QString text = m_pUi->keySequenceLineEdit->text().trimmed();
    const QKeySequence currentKeySequence = keySequenceFromEditString(text);

    if (keySequenceIsValid(currentKeySequence) || text.isEmpty())
    {
        pShortcutItem->m_keySequence = currentKeySequence;

        QKeySequence defaultShortcut = ((pShortcutItem->m_actionKey >= 0)
                                        ? m_pShortcutManager->defaultShortcut(pShortcutItem->m_actionKey, m_currentAccount,
                                                                              pShortcutItem->m_context)
                                        : m_pShortcutManager->defaultShortcut(pShortcutItem->m_actionKey, m_currentAccount,
                                                                              pShortcutItem->m_context));
        ShortcutSettingsWidget * nonConstThis = const_cast<ShortcutSettingsWidget*>(this);
        if (defaultShortcut != pShortcutItem->m_keySequence) {
            nonConstThis->setModified(*pCurrentItem, true);
        }
        else {
            nonConstThis->setModified(*pCurrentItem, false);
        }

        pCurrentItem->setText(1, pShortcutItem->m_keySequence.toString(QKeySequence::NativeText));
        valid = !nonConstThis->markCollisions(*pShortcutItem);
        if (!valid) {
            m_pUi->warningLabel->setText(tr("Key sequence has potential conflicts. <a href=\"#conflicts\">Show.</a>"));
            m_pUi->warningLabel->show();
        }
    }
    else
    {
        m_pUi->warningLabel->setText(tr("Invalid key sequence."));
        m_pUi->warningLabel->show();
    }

    return valid;
}

bool ShortcutSettingsWidget::markCollisions(ShortcutItem & item)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::markCollisions"));

    bool hasCollision = false;
    if (!item.m_keySequence.isEmpty())
    {
        for(auto it = m_shortcutItems.constBegin(), end = m_shortcutItems.constEnd(); it != end; ++it)
        {
            const ShortcutItem * pCurrentItem = *it;
            if (Q_UNLIKELY(!pCurrentItem)) {
                QNWARNING(QStringLiteral("Skipping null pointer to ShortcutItem"));
                continue;
            }

            if (Q_UNLIKELY(pCurrentItem == &item)) {
                continue;
            }

            if (Q_UNLIKELY(!pCurrentItem->m_pTreeWidgetItem)) {
                QNWARNING(QStringLiteral("Skipping ShortcutItem with null pointer to QTreeWidgetItem"));
                continue;
            }

            if (!pCurrentItem->m_keySequence.isEmpty() && (pCurrentItem->m_keySequence == item.m_keySequence)) {
                QNDEBUG(QStringLiteral("Found item with conflicting shortcut: ") << pCurrentItem->m_pTreeWidgetItem->text(0));
                pCurrentItem->m_pTreeWidgetItem->setForeground(1, Qt::red);
                hasCollision = true;
            }
        }
    }

    if (item.m_pTreeWidgetItem) {
        item.m_pTreeWidgetItem->setForeground(1, (hasCollision ? Qt::red : palette().foreground().color()));
    }

    return hasCollision;
}

void ShortcutSettingsWidget::setModified(QTreeWidgetItem & item, bool modified)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::setModified"));

    QFont font = item.font(0);
    font.setItalic(modified);
    item.setFont(0, font);
    font.setBold(modified);
    item.setFont(1, font);
}

bool ShortcutSettingsWidget::filterColumn(const QString & filter, QTreeWidgetItem & item, int column)
{
    QString text;
    ShortcutItem * pShortcutItem = shortcutItemFromTreeItem(&item);

    if (column == (item.columnCount()-1)) // shortcut
    {
        if (pShortcutItem) {
            text = keySequenceToEditString(pShortcutItem->m_keySequence);
        }
        else {
            return true;
        }
    }
    else if (pShortcutItem)
    {
        text = pShortcutItem->m_actionName;
    }
    else
    {
        text = item.text(column);
    }

    return !text.contains(filter, Qt::CaseInsensitive);
}

bool ShortcutSettingsWidget::filterItem(const QString & filter, QTreeWidgetItem & item)
{
    bool visible = filter.isEmpty();
    int numColumns = item.columnCount();
    for(int i = 0; !visible && i < numColumns; ++i) {
        visible |= !filterColumn(filter, item, i);
    }

    int numChildren = item.childCount();
    if (numChildren > 0)
    {
        // force visibility if this item matches
        QString leafFilterString = visible ? QString() : filter;
        for (int i = 0; i < numChildren; ++i)
        {
            QTreeWidgetItem * pChildItem = item.child(i);
            if (pChildItem) {
                visible |= !filterItem(leafFilterString, *pChildItem); // parent is visible if any child is visible
            }
        }
    }

    item.setHidden(!visible);
    return !visible;
}

void ShortcutSettingsWidget::applyItem(ShortcutItem * pItem)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::applyItem: ") << (pItem ? pItem->toString() : QStringLiteral("<null>")));

    if (Q_UNLIKELY(!pItem)) {
        QNWARNING(QStringLiteral("Detected null pointer to ShortcutItem"));
        return;
    }

    if (!pItem->m_isModified) {
        QNDEBUG(QStringLiteral("Not modified, skipping"));
        return;
    }

    if (Q_UNLIKELY(m_pShortcutManager.isNull())) {
        QNWARNING(QStringLiteral("Can't apply the user shortcut: the pointer to shortcut manager is null"));
        return;
    }

    if (pItem->m_actionKey >= 0) {
        m_pShortcutManager->setUserShortcut(pItem->m_actionKey, pItem->m_keySequence, m_currentAccount, pItem->m_context);
    }
    else {
        m_pShortcutManager->setNonStandardUserShortcut(pItem->m_nonStandardActionKey, pItem->m_keySequence, m_currentAccount, pItem->m_context);
    }
}

void ShortcutSettingsWidget::setCurrentItemKeySequence(const QKeySequence & key)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::setCurrentItemKeySequence: ") << key);
    m_pUi->keySequenceLineEdit->setText(keySequenceToEditString(key));
}

void ShortcutSettingsWidget::showConflicts(const QString &)
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::showConflicts"));

    ShortcutItem * pShortcutItem = shortcutItemFromTreeItem(m_pUi->actionsTreeWidget->currentItem());
    if (pShortcutItem) {
        m_pUi->filterLineEdit->setText(keySequenceToEditString(pShortcutItem->m_keySequence));
    }
}

void ShortcutSettingsWidget::clear()
{
    QNDEBUG(QStringLiteral("ShortcutSettingsWidget::clear"));

    for(int i = m_pUi->actionsTreeWidget->topLevelItemCount()-1; i >= 0; --i) {
        delete m_pUi->actionsTreeWidget->takeTopLevelItem(i);
    }
    qDeleteAll(m_shortcutItems);
    m_shortcutItems.clear();
}

ShortcutItem * ShortcutSettingsWidget::shortcutItemFromTreeItem(QTreeWidgetItem * pItem) const
{
    if (!pItem) {
        return Q_NULLPTR;
    }

    QVariant shortcutItemData = pItem->data(0, Qt::UserRole);
    if (!shortcutItemData.isValid()) {
        return Q_NULLPTR;
    }

    return qvariant_cast<ShortcutItem*>(shortcutItemData);
}

QTextStream & ShortcutItem::print(QTextStream & strm) const
{
    strm << QStringLiteral("Shortcut item: action key = ")
         << m_actionKey << QStringLiteral(", non-standard action key = ")
         << (m_nonStandardActionKey.isEmpty() ? QStringLiteral("<empty>") : m_nonStandardActionKey)
         << QStringLiteral(", action name = " ) << m_actionName
         << QStringLiteral(", context = ") << m_context
         << QStringLiteral(", is modified = ") << (m_isModified ? QStringLiteral("true") : QStringLiteral("false"))
         << QStringLiteral(", key sequence = ") << m_keySequence.toString(QKeySequence::PortableText)
         << QStringLiteral(", tree widget item: ") << (m_pTreeWidgetItem ? QStringLiteral("non-null") : QStringLiteral("null"));
    return strm;
}

} // namespace quentier
