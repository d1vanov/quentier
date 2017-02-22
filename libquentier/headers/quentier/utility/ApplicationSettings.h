/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_UTILITY_APPLICATION_SETTINGS_H
#define LIB_QUENTIER_UTILITY_APPLICATION_SETTINGS_H

#include <quentier/types/Account.h>
#include <QSettings>

namespace quentier {

class QUENTIER_EXPORT ApplicationSettings: public QSettings,
                                           public Printable
{
    Q_OBJECT
public:
    /**
     * Constructor for application settings not being account-specific
     */
    ApplicationSettings();

    /**
     * Constructor for application settings specific to the account
     *
     * @param account - the account for which the settings are to be stored or read
     * @param settingsName - if not empty, the created application settings would manage the settings stored
     * in a file with a specific name within the account's settings storage; otherwise they would be stored in the default
     * settings file for the account
     */
    ApplicationSettings(const Account & account, const QString & settingsName = QString());

    ~ApplicationSettings();

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(ApplicationSettings)
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_APPLICATION_SETTINGS_H
