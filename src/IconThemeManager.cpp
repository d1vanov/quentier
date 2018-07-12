/*
 * Copyright 2018 Dmitry Ivanov
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

#include "IconThemeManager.h"
#include "SettingsNames.h"
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/logging/QuentierLogger.h>
#include <QDir>

namespace quentier {

IconThemeManager::IconThemeManager(QObject * parent) :
    QObject(parent),
    m_currentAccount(),
    m_overrideIcons()
{}

QString IconThemeManager::iconThemeName() const
{
    return QIcon::themeName();
}

bool IconThemeManager::setIconThemeName(const QString & name)
{
    QNDEBUG(QStringLiteral("IconThemeManager::setIconThemeName: ") << name);

    QString previousIconName = QIcon::themeName();
    if (previousIconName == name) {
        QNDEBUG(QStringLiteral("Already using this icon theme"));
        return true;
    }

    QIcon::setThemeName(name);
    if (!QIcon::hasThemeIcon(QStringLiteral("document-new"))) {
        QNDEBUG(QStringLiteral("Refusing to switch to the icon theme, it doesn't contain document-new icon and is thus considered invalid"));
        QIcon::setThemeName(previousIconName);
        return false;
    }

    persistIconTheme(name);
    Q_EMIT iconThemeChanged(name);
    return true;
}

QIcon IconThemeManager::overrideThemeIcon(const QString & name) const
{
    auto it = m_overrideIcons.find(name);
    if (it == m_overrideIcons.end()) {
        return QIcon();
    }

    QNTRACE(QStringLiteral("Found override icon for name ") << name);
    return it.value();
}

void IconThemeManager::setOverrideThemeIcon(const QString & name, const QIcon & icon)
{
    QNDEBUG(QStringLiteral("IconThemeManager::setOverrideThemeIcon: name = ") << name);

    m_overrideIcons[name] = icon;
    persistOverrideIcon(name, icon);
    Q_EMIT overrideIconChanged(name, icon);
}

const Account & IconThemeManager::currentAccount() const
{
    return m_currentAccount;
}

void IconThemeManager::setCurrentAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("IconThemeManager::setCurrentAccount: ") << account);

    if (m_currentAccount == account) {
        QNDEBUG(QStringLiteral("Already using this account"));
        return;
    }

    m_currentAccount = account;
    if (m_currentAccount.isEmpty()) {
        QNDEBUG(QStringLiteral("New account is empty, not changing anything"));
        return;
    }

    restoreIconTheme();
    restoreOverrideIcons();
}

void IconThemeManager::persistIconTheme(const QString & name)
{
    QNDEBUG(QStringLiteral("IconThemeManager::persistIconTheme: ") << name);

    if (Q_UNLIKELY(m_currentAccount.isEmpty())) {
        QNDEBUG(QStringLiteral("Won't persist the icon theme: account is empty"));
        return;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(ICON_THEME_SETTINGS_KEY, name);
    appSettings.endGroup();
}

void IconThemeManager::restoreIconTheme()
{
    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QString iconThemeName = appSettings.value(ICON_THEME_SETTINGS_KEY).toString();
    appSettings.endGroup();

    Q_UNUSED(setIconThemeName(iconThemeName))
}

void IconThemeManager::persistOverrideIcon(const QString & name, const QIcon & icon)
{
    QString iconThemeName = QIcon::themeName();

    QNDEBUG(QStringLiteral("IconThemeManager::persistOverrideIcon: icon name = ") << name
            << QStringLiteral(", icon theme name = ") << iconThemeName);

    if (Q_UNLIKELY(m_currentAccount.isEmpty())) {
        QNDEBUG(QStringLiteral("Won't persist the override icon: current account is empty"));
        return;
    }

    QString overrideIconsStoragePath = accountPersistentStoragePath(m_currentAccount);
    overrideIconsStoragePath += QStringLiteral("OverrideIcons/");
    overrideIconsStoragePath += iconThemeName;

    QDir overrideIconsStorageDir(overrideIconsStoragePath);
    if (!overrideIconsStorageDir.exists())
    {
        bool res = overrideIconsStorageDir.mkpath(overrideIconsStoragePath);
        if (!res) {
            ErrorString errorDescription(QT_TR_NOOP("Failed to create the storage dir for the persistence of override icons"));
            errorDescription.details() = overrideIconsStoragePath;
            QNWARNING(errorDescription);
            Q_EMIT notifyError(errorDescription);
            return;
        }
    }

    // TODO: implement further
    Q_UNUSED(icon)
}

void IconThemeManager::restoreOverrideIcons()
{
    // TODO: implement
}

} // namespace quentier
