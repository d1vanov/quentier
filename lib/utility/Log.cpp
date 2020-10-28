/*
 * Copyright 2020 Dmitry Ivanov
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

#include "Log.h"

#include <lib/preferences/SettingsNames.h>
#include <lib/preferences/keys/Logging.h>

#include <quentier/utility/ApplicationSettings.h>

namespace quentier {

QString restoreLogFilterByComponent()
{
    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::loggingGroup);

    QString filter =
        appSettings.value(preferences::keys::loggingFilterByComponentRegex)
            .toString();

    appSettings.endGroup();

    return filter;
}

void setLogFilterByComponent(const QString & filter)
{
    ApplicationSettings appSettings;
    appSettings.beginGroup(preferences::keys::loggingGroup);

    appSettings.setValue(
        preferences::keys::loggingFilterByComponentRegex, filter);

    appSettings.endGroup();
}

} // namespace quentier
