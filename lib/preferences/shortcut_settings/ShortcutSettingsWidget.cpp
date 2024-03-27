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

/**
 * The contents of this file were heavily inspired by the source code of Qt
 * Creator, a Qt and C/C++ IDE by The Qt Company Ltd., 2016. That source code is
 * licensed under GNU GPL v.3 license with two permissive exceptions although
 * they don't apply to this derived work.
 */

#include "ShortcutSettingsWidget.h"
#include "ShortcutButton.h"

using quentier::ShortcutButton;
#include "ui_ShortcutSettingsWidget.h"

#include <lib/utility/ActionsInfo.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/Compat.h>
#include <quentier/utility/ShortcutManager.h>

#include <QtAlgorithms>

#include <utility>

Q_DECLARE_METATYPE(quentier::ShortcutItem *)

namespace quentier {

namespace {

[[nodiscard]] QString keySequenceToEditString(const QKeySequence & sequence)
{
    QString text = sequence.toString(QKeySequence::PortableText);
#ifdef Q_OS_MAC
    // adapt modifier names
    text.replace(
        QStringLiteral("Ctrl"), QStringLiteral("Cmd"), Qt::CaseInsensitive);

    text.replace(
        QStringLiteral("Alt"), QStringLiteral("Opt"), Qt::CaseInsensitive);

    text.replace(
        QStringLiteral("Meta"), QStringLiteral("Ctrl"), Qt::CaseInsensitive);
#endif
    return text;
}

[[nodiscard]] QKeySequence keySequenceFromEditString(const QString & editString)
{
    QString text = editString;
#ifdef Q_OS_MAC
    // adapt modifier names
    text.replace(
        QStringLiteral("Opt"), QStringLiteral("Alt"), Qt::CaseInsensitive);

    text.replace(
        QStringLiteral("Ctrl"), QStringLiteral("Meta"), Qt::CaseInsensitive);

    text.replace(
        QStringLiteral("Cmd"), QStringLiteral("Ctrl"), Qt::CaseInsensitive);
#endif
    return QKeySequence::fromString(text, QKeySequence::PortableText);
}

[[nodiscard]] bool keySequenceIsValid(const QKeySequence & sequence)
{
    if (sequence.isEmpty()) {
        return false;
    }

    for (int i = 0; i < static_cast<int>(sequence.count()); ++i) {
        if (sequence[static_cast<unsigned int>(i)] == Qt::Key_unknown) {
            return false;
        }
    }

    return true;
}

} // namespace

ShortcutSettingsWidget::ShortcutSettingsWidget(QWidget * parent) :
    QWidget{parent}, m_ui{new Ui::ShortcutSettingsWidget}
{
    m_ui->setupUi(this);
    m_ui->filterLineEdit->setClearButtonEnabled(true);

    m_ui->actionsTreeWidget->setRootIsDecorated(false);
    m_ui->actionsTreeWidget->setUniformRowHeights(true);
    m_ui->actionsTreeWidget->setSortingEnabled(true);
    m_ui->actionsTreeWidget->setColumnCount(2);
    m_ui->actionsTreeWidget->setMinimumHeight(200);

    m_ui->actionsTreeWidget->header()->setSectionResizeMode(
        QHeaderView::ResizeToContents);

    QTreeWidgetItem * item = m_ui->actionsTreeWidget->headerItem();
    item->setText(0, tr("Action"));
    item->setText(1, tr("Shortcut"));

    m_ui->keySequenceLabel->setToolTip(
        QStringLiteral("<html><body>") +
#ifdef Q_OS_MAC
        tr("Use \"Cmd\", \"Opt\", \"Ctrl\", and \"Shift\" for modifier keys. "
           "Use \"Escape\", \"Backspace\", \"Delete\", \"Insert\", \"Home\", "
           "and so on, for special keys. Combine individual keys with \"+\", "
           "and combine multiple shortcuts to a shortcut sequence with \",\". "
           "For example, if the user must hold the Ctrl and Shift modifier "
           "keys while pressing Escape, and then release and press A, "
           "enter \"Ctrl+Shift+Escape,A\".") +
#else
        tr("Use \"Ctrl\", \"Alt\", \"Meta\", and \"Shift\" for modifier keys. "
           "Use \"Escape\", \"Backspace\", \"Delete\", \"Insert\", \"Home\", "
           "and so on, for special keys. Combine individual keys with \"+\", "
           "and combine multiple shortcuts to a shortcut sequence with \",\". "
           "For example, if the user must hold the Ctrl and Shift modifier "
           "keys while pressing Escape, and then release and press A, "
           "enter \"Ctrl+Shift+Escape,A\".") +
#endif
        QStringLiteral("</html></body>"));

    m_ui->warningLabel->hide();

    m_ui->resetAllPushButton->setDefault(false);
    m_ui->resetAllPushButton->setAutoDefault(false);

    m_ui->resetPushButton->setDefault(false);
    m_ui->resetPushButton->setAutoDefault(false);

    m_ui->recordPushButton->setDefault(false);
    m_ui->recordPushButton->setAutoDefault(false);

    QObject::connect(
        m_ui->resetAllPushButton, &QPushButton::clicked, this,
        &ShortcutSettingsWidget::resetAll);

    QObject::connect(
        m_ui->resetPushButton, &QPushButton::clicked, this,
        &ShortcutSettingsWidget::resetToDefault);

    QObject::connect(
        m_ui->recordPushButton, &ShortcutButton::keySequenceChanged, this,
        &ShortcutSettingsWidget::setCurrentItemKeySequence);

    QObject::connect(
        m_ui->warningLabel, &QLabel::linkActivated, this,
        &ShortcutSettingsWidget::showConflicts);

    QObject::connect(
        m_ui->filterLineEdit, &QLineEdit::textChanged, this,
        &ShortcutSettingsWidget::onActionFilterChanged);

    QObject::connect(
        m_ui->actionsTreeWidget, &QTreeWidget::currentItemChanged, this,
        &ShortcutSettingsWidget::onCurrentActionChanged);

    QObject::connect(
        m_ui->keySequenceLineEdit, &QLineEdit::editingFinished, this,
        &ShortcutSettingsWidget::onCurrentItemKeySequenceEdited);
}

ShortcutSettingsWidget::~ShortcutSettingsWidget()
{
    delete m_ui;
    qDeleteAll(m_shortcutItems);
}

void ShortcutSettingsWidget::initialize(
    Account account, const ActionsInfo & actionsInfo,
    ShortcutManager * shortcutManager)
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget",
        "ShortcutSettingsWidget::initialize: account = " << account);

    clear();

    m_shortcutManager = shortcutManager;
    m_currentAccount = std::move(account);

    if (Q_UNLIKELY(!shortcutManager)) {
        ErrorString error(
            QT_TR_NOOP("Detected null pointer to ShortcutManager "
                       "passed to ShortcutSettingsWidget"));
        QNWARNING("preferences::ShortcutSettingsWidget", error);
        return;
    }

    for (auto it = actionsInfo.begin(), end = actionsInfo.end(); it != end;
         ++it)
    {
        const auto info = it.actionInfo();
        if (Q_UNLIKELY(info.isEmpty())) {
            continue;
        }

        auto * shortcutItem = new ShortcutItem;
        m_shortcutItems << shortcutItem;

        if (info.m_shortcutKey >= 0) {
            shortcutItem->m_actionKey = info.m_shortcutKey;
        }
        else {
            shortcutItem->m_nonStandardActionKey =
                info.m_nonStandardShortcutKey;
        }

        shortcutItem->m_actionName = info.m_localizedName;
        shortcutItem->m_context = info.m_context;
        shortcutItem->m_category = info.m_category;
        shortcutItem->m_keySequence = info.m_shortcut;

        QNDEBUG("preferences::ShortcutSettingsWidget", "ActionInfo: " << info);

        QNDEBUG(
            "preferences::ShortcutSettingsWidget",
            "Setup ShortcutItem: " << *shortcutItem);
    }

    QMap<QString, QTreeWidgetItem *> sections;

    for (auto it = m_shortcutItems.begin(), end = m_shortcutItems.end();
         it != end; ++it)
    {
        auto * shortcutItem = *it;

        shortcutItem->m_keySequence =
            ((shortcutItem->m_actionKey >= 0)
                 ? m_shortcutManager->shortcut(
                       shortcutItem->m_actionKey, m_currentAccount,
                       shortcutItem->m_context)
                 : m_shortcutManager->shortcut(
                       shortcutItem->m_nonStandardActionKey, m_currentAccount,
                       shortcutItem->m_context));

        shortcutItem->m_treeWidgetItem = new QTreeWidgetItem;

        QString sectionName = shortcutItem->m_category;
        if (sectionName.isEmpty()) {
            sectionName = tr("Actions");
            shortcutItem->m_category = sectionName;
        }

        if (!sections.contains(sectionName)) {
            auto * menuItem = new QTreeWidgetItem(
                m_ui->actionsTreeWidget, QStringList(sectionName));

            QFont font = menuItem->font(0);
            font.setBold(true);
            menuItem->setFont(0, font);

            sections.insert(sectionName, menuItem);
            m_ui->actionsTreeWidget->expandItem(menuItem);
        }
        sections[sectionName]->addChild(shortcutItem->m_treeWidgetItem);

        shortcutItem->m_treeWidgetItem->setText(0, shortcutItem->m_actionName);

        shortcutItem->m_treeWidgetItem->setText(
            1, shortcutItem->m_keySequence.toString(QKeySequence::NativeText));

        if (!shortcutItem->m_keySequence.isEmpty()) {
            const auto defaultShortcut =
                ((shortcutItem->m_actionKey >= 0)
                     ? m_shortcutManager->defaultShortcut(
                           shortcutItem->m_actionKey, m_currentAccount,
                           shortcutItem->m_context)
                     : m_shortcutManager->defaultShortcut(
                           shortcutItem->m_nonStandardActionKey,
                           m_currentAccount, shortcutItem->m_context));

            if (defaultShortcut.isEmpty() ||
                defaultShortcut != shortcutItem->m_keySequence)
            {
                shortcutItem->m_isModified = true;
            }
        }

        setModified(
            *shortcutItem->m_treeWidgetItem, shortcutItem->m_isModified);

        shortcutItem->m_treeWidgetItem->setData(
            0, Qt::UserRole, QVariant::fromValue(shortcutItem));

        markCollisions(*shortcutItem);
    }

    m_ui->actionsTreeWidget->sortItems(0, Qt::AscendingOrder);

    onActionFilterChanged(m_ui->filterLineEdit->text());
}

void ShortcutSettingsWidget::onCurrentActionChanged(
    QTreeWidgetItem * currentItem,
    [[maybe_unused]] QTreeWidgetItem * previousItem)
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget",
        "ShortcutSettingsWidget::onCurrentActionChanged");

    auto * shortcutItem = shortcutItemFromTreeItem(currentItem);
    if (!shortcutItem) {
        m_ui->keySequenceLineEdit->clear();
        m_ui->warningLabel->clear();
        m_ui->warningLabel->hide();
        m_ui->shortcutGroupBox->setEnabled(false);
        return;
    }

    m_ui->keySequenceLineEdit->setText(
        keySequenceToEditString(shortcutItem->m_keySequence));

    if (markCollisions(*shortcutItem)) {
        warnOfConflicts();
    }
    else {
        m_ui->warningLabel->hide();
    }

    m_ui->shortcutGroupBox->setEnabled(true);
}

void ShortcutSettingsWidget::resetToDefault()
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget",
        "ShortcutSettingsWidget::resetToDefault");

    auto * currentItem = m_ui->actionsTreeWidget->currentItem();
    auto * shortcutItem = shortcutItemFromTreeItem(currentItem);
    if (Q_UNLIKELY(!shortcutItem)) {
        QNDEBUG(
            "preferences::ShortcutSettingsWidget", "No current shortcut item");
        return;
    }

    if (!shortcutItem->m_isModified) {
        QNDEBUG(
            "preferences::ShortcutSettingsWidget",
            "The shortcut for action "
                << shortcutItem->m_actionName
                << " is the default one already, nothing to reset");
        return;
    }

    QNTRACE(
        "preferences::ShortcutSettingsWidget",
        "Resetting the user shortcut for action "
            << shortcutItem->m_actionName);

    shortcutItem->m_isModified = false;
    setModified(*currentItem, false);

    if (Q_UNLIKELY(m_shortcutManager.isNull())) {
        QNWARNING(
            "preferences::ShortcutSettingsWidget",
            "Can't reset the shortcut to the default one: shortcut manager is "
            "expired");
        return;
    }

    QKeySequence defaultShortcut;

    if (shortcutItem->m_actionKey >= 0) {
        m_shortcutManager->setUserShortcut(
            shortcutItem->m_actionKey, QKeySequence{}, m_currentAccount,
            shortcutItem->m_context);

        defaultShortcut = m_shortcutManager->defaultShortcut(
            shortcutItem->m_actionKey, m_currentAccount,
            shortcutItem->m_context);
    }
    else {
        m_shortcutManager->setNonStandardUserShortcut(
            shortcutItem->m_nonStandardActionKey, QKeySequence{},
            m_currentAccount, shortcutItem->m_context);

        defaultShortcut = m_shortcutManager->defaultShortcut(
            shortcutItem->m_nonStandardActionKey, m_currentAccount,
            shortcutItem->m_context);
    }

    shortcutItem->m_keySequence = defaultShortcut;

    m_ui->keySequenceLineEdit->setText(
        keySequenceToEditString(defaultShortcut));

    currentItem->setText(1, defaultShortcut.toString(QKeySequence::NativeText));

    for (auto * currentShortcutItem: std::as_const(m_shortcutItems)) {
        if (Q_UNLIKELY(!currentShortcutItem)) {
            QNWARNING(
                "preferences::ShortcutSettingsWidget",
                "Skipping null pointer to ShortcutItem");
            continue;
        }

        if (markCollisions(*currentShortcutItem) &&
            (currentShortcutItem == shortcutItem))
        {
            warnOfConflicts();
        }
        else {
            m_ui->warningLabel->hide();
        }
    }
}

void ShortcutSettingsWidget::resetAll()
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget",
        "ShortcutSettingsWidget::resetAll");

    for (auto * currentShortcutItem: std::as_const(m_shortcutItems)) {
        if (Q_UNLIKELY(!currentShortcutItem)) {
            QNWARNING(
                "preferences::ShortcutSettingsWidget",
                "Skipping null pointer to ShortcutItem");
            continue;
        }

        if (!currentShortcutItem->m_isModified) {
            continue;
        }

        if (currentShortcutItem->m_actionKey >= 0) {
            m_shortcutManager->setUserShortcut(
                currentShortcutItem->m_actionKey, QKeySequence{},
                m_currentAccount, currentShortcutItem->m_context);

            currentShortcutItem->m_keySequence =
                m_shortcutManager->defaultShortcut(
                    currentShortcutItem->m_actionKey, m_currentAccount,
                    currentShortcutItem->m_context);
        }
        else {
            m_shortcutManager->setNonStandardUserShortcut(
                currentShortcutItem->m_nonStandardActionKey, QKeySequence{},
                m_currentAccount, currentShortcutItem->m_context);

            currentShortcutItem->m_keySequence =
                m_shortcutManager->defaultShortcut(
                    currentShortcutItem->m_nonStandardActionKey,
                    m_currentAccount, currentShortcutItem->m_context);
        }

        if (currentShortcutItem->m_treeWidgetItem) {
            currentShortcutItem->m_treeWidgetItem->setText(
                1,
                currentShortcutItem->m_keySequence.toString(
                    QKeySequence::NativeText));

            setModified(*(currentShortcutItem->m_treeWidgetItem), false);

            if (currentShortcutItem->m_treeWidgetItem ==
                m_ui->actionsTreeWidget->currentItem())
            {
                onCurrentActionChanged(
                    currentShortcutItem->m_treeWidgetItem, nullptr);
            }
        }
    }

    for (auto * currentShortcutItem: std::as_const(m_shortcutItems)) {
        if (Q_UNLIKELY(!currentShortcutItem)) {
            QNWARNING(
                "preferences::ShortcutSettingsWidget",
                "Skipping null pointer to ShortcutItem");
            continue;
        }

        if (markCollisions(*currentShortcutItem) &&
            (currentShortcutItem->m_treeWidgetItem ==
             m_ui->actionsTreeWidget->currentItem()))
        {
            warnOfConflicts();
        }
        else {
            m_ui->warningLabel->hide();
        }
    }
}

void ShortcutSettingsWidget::onActionFilterChanged(const QString & filter)
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget",
        "ShortcutSettingsWidget::onActionFilterChanged: filter = " << filter);

    for (int i = 0; i < m_ui->actionsTreeWidget->topLevelItemCount(); ++i) {
        auto * currentItem = m_ui->actionsTreeWidget->topLevelItem(i);
        if (currentItem) {
            filterItem(filter, *currentItem);
        }
    }

    auto * currentItem = m_ui->actionsTreeWidget->currentItem();
    if (currentItem) {
        m_ui->actionsTreeWidget->scrollToItem(currentItem);
    }
}

void ShortcutSettingsWidget::onCurrentItemKeySequenceEdited()
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget",
        "ShortcutSettingsWidget::onCurrentItemKeySequenceEdited");

    m_ui->warningLabel->clear();
    m_ui->warningLabel->hide();

    auto * currentItem = m_ui->actionsTreeWidget->currentItem();
    auto * shortcutItem = shortcutItemFromTreeItem(currentItem);
    if (!shortcutItem) {
        return;
    }

    const QString text = m_ui->keySequenceLineEdit->text().trimmed();
    const auto currentKeySequence = keySequenceFromEditString(text);

    if (keySequenceIsValid(currentKeySequence) || text.isEmpty()) {
        shortcutItem->m_keySequence = currentKeySequence;

        QKeySequence defaultShortcut =
            ((shortcutItem->m_actionKey >= 0)
                 ? m_shortcutManager->defaultShortcut(
                       shortcutItem->m_actionKey, m_currentAccount,
                       shortcutItem->m_context)
                 : m_shortcutManager->defaultShortcut(
                       shortcutItem->m_nonStandardActionKey, m_currentAccount,
                       shortcutItem->m_context));

        const bool modified = (defaultShortcut != shortcutItem->m_keySequence);

        shortcutItem->m_isModified = modified;
        setModified(*currentItem, modified);

        if (shortcutItem->m_actionKey >= 0) {
            m_shortcutManager->setUserShortcut(
                shortcutItem->m_actionKey,
                (modified ? shortcutItem->m_keySequence : QKeySequence{}),
                m_currentAccount, shortcutItem->m_context);
        }
        else {
            m_shortcutManager->setNonStandardUserShortcut(
                shortcutItem->m_nonStandardActionKey,
                (modified ? shortcutItem->m_keySequence : QKeySequence{}),
                m_currentAccount, shortcutItem->m_context);
        }

        currentItem->setText(
            1, shortcutItem->m_keySequence.toString(QKeySequence::NativeText));

        if (markCollisions(*shortcutItem)) {
            warnOfConflicts();
        }
        else {
            m_ui->warningLabel->hide();
        }
    }
    else {
        m_ui->warningLabel->setText(
            QStringLiteral("<font color=\"red\">") +
            tr("Invalid key sequence.") + QStringLiteral("</font>"));

        m_ui->warningLabel->show();
    }
}

bool ShortcutSettingsWidget::markCollisions(ShortcutItem & item)
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget",
        "ShortcutSettingsWidget::markCollisions");

    bool hasCollision = false;
    if (!item.m_keySequence.isEmpty()) {
        for (auto * currentItem: std::as_const(m_shortcutItems)) {
            if (Q_UNLIKELY(!currentItem)) {
                QNWARNING(
                    "preferences::ShortcutSettingsWidget",
                    "Skipping null pointer to ShortcutItem");
                continue;
            }

            if (Q_UNLIKELY(currentItem == &item)) {
                continue;
            }

            if (Q_UNLIKELY(!currentItem->m_treeWidgetItem)) {
                QNTRACE(
                    "preferences::ShortcutSettingsWidget",
                    "Skipping ShortcutItem with null pointer to "
                    "QTreeWidgetItem");
                continue;
            }

            if (!currentItem->m_keySequence.isEmpty() &&
                currentItem->m_keySequence == item.m_keySequence)
            {
                QNDEBUG(
                    "preferences::ShortcutSettingsWidget",
                    "Found item with conflicting shortcut: "
                        << currentItem->m_treeWidgetItem->text(0));

                currentItem->m_treeWidgetItem->setForeground(1, Qt::red);
                hasCollision = true;
            }
        }
    }

    if (item.m_treeWidgetItem) {
        item.m_treeWidgetItem->setForeground(
            1, (hasCollision ? Qt::red : palette().windowText().color()));
    }

    return hasCollision;
}

void ShortcutSettingsWidget::setModified(
    QTreeWidgetItem & item, const bool modified)
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget",
        "ShortcutSettingsWidget::setModified");

    QFont font = item.font(0);
    font.setItalic(modified);
    item.setFont(0, font);
    font.setBold(modified);
    item.setFont(1, font);
}

bool ShortcutSettingsWidget::filterColumn(
    const QString & filter, QTreeWidgetItem & item, const int column)
{
    QString text;
    auto * shortcutItem = shortcutItemFromTreeItem(&item);

    if (column == (item.columnCount() - 1)) // shortcut
    {
        if (shortcutItem) {
            text = keySequenceToEditString(shortcutItem->m_keySequence);
        }
        else {
            return true;
        }
    }
    else if (shortcutItem) {
        text = shortcutItem->m_actionName;
    }
    else {
        text = item.text(column);
    }

    return !text.contains(filter, Qt::CaseInsensitive);
}

bool ShortcutSettingsWidget::filterItem(
    const QString & filter, QTreeWidgetItem & item)
{
    bool visible = filter.isEmpty();
    int numColumns = item.columnCount();
    for (int i = 0; !visible && i < numColumns; ++i) {
        visible |= !filterColumn(filter, item, i);
    }

    const int numChildren = item.childCount();
    if (numChildren > 0) {
        // force visibility if this item matches
        const QString leafFilterString = visible ? QString() : filter;
        for (int i = 0; i < numChildren; ++i) {
            auto * childItem = item.child(i);
            if (childItem) {
                // parent is visible if any child is visible
                visible |= !filterItem(leafFilterString, *childItem);
            }
        }
    }

    item.setHidden(!visible);
    return !visible;
}

void ShortcutSettingsWidget::setCurrentItemKeySequence(const QKeySequence & key)
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget",
        "ShortcutSettingsWidget::setCurrentItemKeySequence: " << key);

    if (Q_UNLIKELY(m_shortcutManager.isNull())) {
        QNWARNING(
            "preferences::ShortcutSettingsWidget",
            "Can't set the current item shortcut: shortcut manager has "
            "expired");
        return;
    }

    auto * currentItem = m_ui->actionsTreeWidget->currentItem();
    auto * currentShortcutItem = shortcutItemFromTreeItem(currentItem);
    if (Q_UNLIKELY(!currentShortcutItem)) {
        QNWARNING(
            "preferences::ShortcutSettingsWidget",
            "Can't find the shortcut item for the current tree widget item");
        return;
    }

    currentShortcutItem->m_keySequence = key;
    currentShortcutItem->m_isModified = true;

    currentItem->setText(
        1,
        currentShortcutItem->m_keySequence.toString(QKeySequence::NativeText));

    setModified(*currentItem, true);

    m_ui->keySequenceLineEdit->setText(keySequenceToEditString(key));

    if (currentShortcutItem->m_actionKey >= 0) {
        m_shortcutManager->setUserShortcut(
            currentShortcutItem->m_actionKey,
            currentShortcutItem->m_keySequence, m_currentAccount,
            currentShortcutItem->m_context);
    }
    else {
        m_shortcutManager->setNonStandardUserShortcut(
            currentShortcutItem->m_nonStandardActionKey,
            currentShortcutItem->m_keySequence, m_currentAccount,
            currentShortcutItem->m_context);
    }

    if (markCollisions(*currentShortcutItem)) {
        warnOfConflicts();
    }
    else {
        m_ui->warningLabel->hide();
    }
}

void ShortcutSettingsWidget::showConflicts(const QString &)
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget",
        "ShortcutSettingsWidget::showConflicts");

    auto * shortcutItem =
        shortcutItemFromTreeItem(m_ui->actionsTreeWidget->currentItem());

    if (shortcutItem) {
        m_ui->filterLineEdit->setText(
            keySequenceToEditString(shortcutItem->m_keySequence));
    }
}

void ShortcutSettingsWidget::clear()
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget", "ShortcutSettingsWidget::clear");

    for (int i = m_ui->actionsTreeWidget->topLevelItemCount() - 1; i >= 0; --i)
    {
        delete m_ui->actionsTreeWidget->takeTopLevelItem(i);
    }

    qDeleteAll(m_shortcutItems);
    m_shortcutItems.clear();
}

void ShortcutSettingsWidget::warnOfConflicts()
{
    QNDEBUG(
        "preferences::ShortcutSettingsWidget",
        "ShortcutSettingsWidget::warnOfConflicts");

    m_ui->warningLabel->setText(
        QStringLiteral("<font color=\"red\">") +
        tr("Key sequence has potential conflicts") +
        QStringLiteral(". <a href=\"#conflicts\">") + tr("Show") +
        QStringLiteral(".</a></font>"));

    m_ui->warningLabel->show();
}

ShortcutItem * ShortcutSettingsWidget::shortcutItemFromTreeItem(
    QTreeWidgetItem * item) const
{
    if (!item) {
        return nullptr;
    }

    const QVariant shortcutItemData = item->data(0, Qt::UserRole);
    if (!shortcutItemData.isValid()) {
        return nullptr;
    }

    return qvariant_cast<ShortcutItem *>(shortcutItemData);
}

QTextStream & ShortcutItem::print(QTextStream & strm) const
{
    strm << "Shortcut item: action key = " << m_actionKey
         << ", non-standard action key = "
         << (m_nonStandardActionKey.isEmpty() ? QStringLiteral("<empty>")
                                              : m_nonStandardActionKey)
         << ", action name = " << m_actionName << ", context = " << m_context
         << ", category = " << m_category
         << ", is modified = " << (m_isModified ? "true" : "false")
         << ", key sequence = "
         << m_keySequence.toString(QKeySequence::PortableText)
         << ", tree widget item: " << (m_treeWidgetItem ? "non-null" : "null");

    return strm;
}

} // namespace quentier
