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

#ifndef QUENTIER_ICON_THEME_MANAGER_H
#define QUENTIER_ICON_THEME_MANAGER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <QObject>
#include <QIcon>
#include <QHash>
#include <QSet>
#include <QStringList>

namespace quentier {

class IconThemeManager: public QObject
{
    Q_OBJECT
public:
    explicit IconThemeManager(QObject * parent = Q_NULLPTR);

    struct BuiltInIconTheme
    {
        enum type
        {
            Oxygen = 0,
            Tango,
            Breeze,
            BreezeDark
        };
    };

    QString iconThemeName() const;
    void setIconThemeName(const QString & name, const BuiltInIconTheme::type fallbackIconTheme);

    QIcon overrideThemeIcon(const QString & name) const;
    bool setOverrideThemeIcon(const QString & name, const QString & overrideIconFilePath,
                              ErrorString & errorDescription);

    QIcon iconFromTheme(const QString & name) const;

    QIcon overrideIconFromTheme(const QString & name) const;
    QIcon iconFromFallbackTheme(const QString & name) const;

    struct IconThemeSearchPathKind
    {
        enum type
        {
            Default = 0x0,
            Manual  = 0x1
        };
    };

    Q_DECLARE_FLAGS(IconThemeSearchPathKinds, IconThemeSearchPathKind::type)

    QStringList iconThemeSearchPaths(const IconThemeSearchPathKinds kinds =
                                     IconThemeSearchPathKinds(IconThemeSearchPathKind::Default |
                                                              IconThemeSearchPathKind::Manual)) const;
    void addIconThemeSearchPath(const QString & path);
    void addIconThemeSearchPaths(const QStringList & paths);
    void removeIconThemeSearchPath(const QString & path);
    void removeIconThemeSearchPaths(const QStringList & paths);

    const Account & currentAccount() const;
    void setCurrentAccount(const Account & account);

Q_SIGNALS:
    void iconThemeChanged(QString iconThemeName, QString fallbackIconTheme);
    void overrideIconChanged(QString name, QIcon icon);
    void iconThemeSearchPathsChanged();

    void notifyError(ErrorString errorDescription);

private:
    void persistIconTheme(const QString & name, const BuiltInIconTheme::type fallbackIconTheme);
    void restoreIconTheme();

    bool persistOverrideIcon(const QString & name, const QString & overrideIconFilePath,
                             ErrorString & errorDescription);
    void restoreOverrideIcons();

    void persistIconThemeSearchPaths();
    void restoreIconThemeSearchPaths();

    void updateIconThemeSearchPaths();
    QString builtInIconThemeName(const BuiltInIconTheme::type builtInIconTheme) const;

    void appendSubdirs(const QString & dirPath, QSet<QString> & paths) const;

private:
    Q_DISABLE_COPY(IconThemeManager)

private:
    Account                 m_currentAccount;
    BuiltInIconTheme::type  m_fallbackIconTheme;
    QHash<QString,QIcon>    m_overrideIcons;
    QSet<QString>           m_manualIconThemeSearchPaths;
    QStringList             m_defaultIconThemeSearchPaths;
};

} // namespace quentier

#endif // QUENTIER_ICON_THEME_MANAGER_H
