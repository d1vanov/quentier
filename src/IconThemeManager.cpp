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
#include <QFileInfo>

#define OXYGEN_FALLBACK_ICON_THEME_NAME QStringLiteral("oxygen")
#define TANGO_FALLBACK_ICON_THEME_NAME QStringLiteral("tango")
#define BREEZE_FALLBACK_ICON_THEME_NAME QStringLiteral("breeze")
#define BREEZE_DARK_FALLBACK_ICON_THEME_NAME QStringLiteral("breeze-dark")

namespace quentier {

IconThemeManager::IconThemeManager(QObject * parent) :
    QObject(parent),
    m_currentAccount(),
    m_fallbackIconTheme(BuiltInIconTheme::Oxygen),
    m_overrideIcons()
{}

QString IconThemeManager::iconThemeName() const
{
    return QIcon::themeName();
}

void IconThemeManager::setIconThemeName(const QString & name, const BuiltInIconTheme::type fallbackIconTheme)
{
    QNDEBUG(QStringLiteral("IconThemeManager::setIconThemeName: ") << name
            << QStringLiteral(", fallback icon theme = ") << fallbackIconTheme);

    QString previousIconThemeName = QIcon::themeName();
    if ((previousIconThemeName == name) && (fallbackIconTheme == m_fallbackIconTheme)) {
        QNDEBUG(QStringLiteral("Already using these icon theme and fallback"));
        return;
    }

    QIcon::setThemeName(name);

    QString fallbackIconThemeName = builtInIconThemeName(fallbackIconTheme);
    if (Q_UNLIKELY(fallbackIconThemeName.isEmpty()))
    {
        QNINFO(QStringLiteral("Fallback icon theme seems incorrect, fallback to oxygen"));
        fallbackIconThemeName = OXYGEN_FALLBACK_ICON_THEME_NAME;
        if ((previousIconThemeName == name) && (m_fallbackIconTheme == BuiltInIconTheme::Oxygen)) {
            QNDEBUG(QStringLiteral("Icon theme is already used with Oxygen fallback"));
            return;
        }

        m_fallbackIconTheme = BuiltInIconTheme::Oxygen;
    }
    else
    {
        m_fallbackIconTheme = fallbackIconTheme;
    }

    persistIconTheme(name, m_fallbackIconTheme);
    Q_EMIT iconThemeChanged(name, fallbackIconThemeName);
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

bool IconThemeManager::setOverrideThemeIcon(const QString & name, const QString & overrideIconFilePath,
                                            ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("IconThemeManager::setOverrideThemeIcon: name = ") << name
            << QStringLiteral(", override icon file path = ") << QDir::toNativeSeparators(overrideIconFilePath));

    QIcon overrideIcon(overrideIconFilePath);
    if (overrideIcon.isNull()) {
        errorDescription.setBase(QT_TR_NOOP("can't load icon from the specified file"));
        QNDEBUG(errorDescription);
        return false;
    }

    if (!persistOverrideIcon(name, overrideIconFilePath, errorDescription)) {
        return false;
    }

    m_overrideIcons[name] = overrideIcon;
    Q_EMIT overrideIconChanged(name, overrideIcon);
    return true;
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

QIcon IconThemeManager::iconFromTheme(const QString & name) const
{
    auto overrideIt = m_overrideIcons.find(name);
    if (overrideIt != m_overrideIcons.end()) {
        return overrideIt.value();
    }

    QIcon icon = QIcon::fromTheme(name);
    if (!icon.isNull()) {
        return icon;
    }

    QString fallbackIconThemeName = builtInIconThemeName(m_fallbackIconTheme);
    if (Q_UNLIKELY(fallbackIconThemeName.isEmpty())) {
        fallbackIconThemeName = OXYGEN_FALLBACK_ICON_THEME_NAME;
    }

    QString originalIconThemeName = QIcon::themeName();
    QIcon::setThemeName(fallbackIconThemeName);
    icon = QIcon::fromTheme(name);
    QIcon::setThemeName(originalIconThemeName);
    return icon;
}

void IconThemeManager::persistIconTheme(const QString & name, const BuiltInIconTheme::type fallbackIconTheme)
{
    QNDEBUG(QStringLiteral("IconThemeManager::persistIconTheme: ") << name
            << QStringLiteral(", fallback icon theme = ") << fallbackIconTheme);

    if (Q_UNLIKELY(m_currentAccount.isEmpty())) {
        QNDEBUG(QStringLiteral("Won't persist the icon theme: account is empty"));
        return;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    appSettings.setValue(ICON_THEME_SETTINGS_KEY, name);
    appSettings.setValue(FALLBACK_ICON_THEME_SETTINGS_KEY, fallbackIconTheme);
    appSettings.endGroup();
}

void IconThemeManager::restoreIconTheme()
{
    QNDEBUG(QStringLiteral("IconThemeManager::restoreIconTheme"));

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QString iconThemeName = appSettings.value(ICON_THEME_SETTINGS_KEY).toString();

    bool conversionResult = false;
    BuiltInIconTheme::type fallbackIconTheme = static_cast<BuiltInIconTheme::type>(appSettings.value(FALLBACK_ICON_THEME_SETTINGS_KEY).toInt(&conversionResult));
    if (Q_UNLIKELY(!conversionResult)) {
        QNDEBUG(QStringLiteral("Can't convert fallback icon theme to int, fallback to oxygen"));
        fallbackIconTheme = BuiltInIconTheme::Oxygen;
    }

    appSettings.endGroup();

    setIconThemeName(iconThemeName, fallbackIconTheme);
}

bool IconThemeManager::persistOverrideIcon(const QString & name, const QString & overrideIconFilePath, ErrorString & errorDescription)
{
    QString iconThemeName = QIcon::themeName();

    QNDEBUG(QStringLiteral("IconThemeManager::persistOverrideIcon: icon name = ") << name
            << QStringLiteral(", icon file path = ") << QDir::toNativeSeparators(overrideIconFilePath)
            << QStringLiteral(", icon theme name = ") << iconThemeName);

    if (Q_UNLIKELY(m_currentAccount.isEmpty())) {
        errorDescription.setBase(QT_TR_NOOP("current account is empty"));
        QNDEBUG(errorDescription);
        return false;
    }

    if (Q_UNLIKELY(name.isEmpty())) {
        errorDescription.setBase(QT_TR_NOOP("icon name from the theme is empty"));
        QNDEBUG(errorDescription);
        return false;
    }

    QFileInfo iconFileInfo(overrideIconFilePath);
    if (!iconFileInfo.exists() || !iconFileInfo.isFile() || !iconFileInfo.isReadable()) {
        errorDescription.setBase(QT_TR_NOOP("can't read the override icon from the specified file"));
        QNDEBUG(errorDescription);
        return false;
    }

    QString overrideIconsStoragePath = accountPersistentStoragePath(m_currentAccount);
    overrideIconsStoragePath += QStringLiteral("/OverrideIcons/");
    overrideIconsStoragePath += iconThemeName;

    QDir overrideIconsStorageDir(overrideIconsStoragePath);
    if (!overrideIconsStorageDir.exists())
    {
        bool res = overrideIconsStorageDir.mkpath(overrideIconsStoragePath);
        if (!res) {
            errorDescription.setBase(QT_TR_NOOP("can't create the storage dir to persist the override icons"));
            errorDescription.details() = QDir::toNativeSeparators(overrideIconsStoragePath);
            QNWARNING(errorDescription);
            return false;
        }
    }

    QString persistentIconFilePath = overrideIconsStoragePath + QStringLiteral("/") + name + QStringLiteral(".") + iconFileInfo.completeSuffix();
    QFileInfo persistentIconFileInfo(persistentIconFilePath);
    if (persistentIconFileInfo.exists() && !QFile::remove(persistentIconFilePath)) {
        errorDescription.setBase(QT_TR_NOOP("can't remove the previous override icon within the persistent storage area"));
        QNWARNING(errorDescription);
        return false;
    }

    if (!QFile::copy(overrideIconFilePath, persistentIconFilePath)) {
        errorDescription.setBase(QT_TR_NOOP("can't copy the override icon to the persistent storage area"));
        QNWARNING(errorDescription);
        return false;
    }

    return true;
}

void IconThemeManager::restoreOverrideIcons()
{
    QString iconThemeName = QIcon::themeName();
    QNDEBUG(QStringLiteral("IconThemeManager::restoreOverrideIcons: icon theme name = ") << iconThemeName);

    m_overrideIcons.clear();

    if (Q_UNLIKELY(m_currentAccount.isEmpty())) {
        QNDEBUG(QStringLiteral("Can't restore the override icons: current account is empty"));
        return;
    }

    QString overrideIconsStoragePath = accountPersistentStoragePath(m_currentAccount);
    overrideIconsStoragePath += QStringLiteral("/OverrideIcons/");
    overrideIconsStoragePath += iconThemeName;

    QDir overrideIconsStorageDir(overrideIconsStoragePath);
    if (!overrideIconsStorageDir.exists()) {
        QNDEBUG(QStringLiteral("Override icons storage folder doesn't exist for the current theme, nothing to restore"));
        return;
    }

    QFileInfoList iconFileInfos = overrideIconsStorageDir.entryInfoList(QDir::Filters(QDir::Files | QDir::NoDotAndDotDot));
    for(auto it = iconFileInfos.constBegin(), end = iconFileInfos.constEnd(); it != end; ++it)
    {
        const QFileInfo & iconFileInfo = *it;

        if (!iconFileInfo.exists() || !iconFileInfo.isFile() || !iconFileInfo.isReadable()) {
            QNDEBUG(QStringLiteral("Skipping ") << iconFileInfo.fileName() << QStringLiteral(" as it's either not a file or is not readable"));
            continue;
        }

        QIcon icon(iconFileInfo.absoluteFilePath());
        if (Q_UNLIKELY(icon.isNull())) {
            QNDEBUG(QStringLiteral("Skipping ") << iconFileInfo.fileName() << QStringLiteral(", can't load icon from it"));
            continue;
        }

        QString iconName = iconFileInfo.baseName();

        m_overrideIcons[iconName] = icon;
        Q_EMIT overrideIconChanged(iconName, icon);
    }
}

QString IconThemeManager::builtInIconThemeName(const BuiltInIconTheme::type builtInIconTheme) const
{
    switch(builtInIconTheme)
    {
    case BuiltInIconTheme::Oxygen:
        return OXYGEN_FALLBACK_ICON_THEME_NAME;
    case BuiltInIconTheme::Tango:
        return TANGO_FALLBACK_ICON_THEME_NAME;
    case BuiltInIconTheme::Breeze:
        return BREEZE_FALLBACK_ICON_THEME_NAME;
    case BuiltInIconTheme::BreezeDark:
        return BREEZE_DARK_FALLBACK_ICON_THEME_NAME;
    default:
        return QString();
    }
}

} // namespace quentier
