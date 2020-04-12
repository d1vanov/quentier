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

#include "IUpdateChecker.h"

#include <lib/utility/VersionInfo.h>

#if QUENTIER_PACKAGED_AS_APP_IMAGE
#include "AppImageUpdateChecker.h"
#endif

#include "GitHubUpdateChecker.h"

namespace quentier {

IUpdateChecker::IUpdateChecker(QObject * parent) :
    QObject(parent)
{}

QString IUpdateChecker::updateChannel() const
{
    return m_updateChannel;
}

void IUpdateChecker::setUpdateChannel(QString channel)
{
    m_updateChannel = std::move(channel);
}

bool IUpdateChecker::useContinuousUpdateChannel() const
{
    return m_useContinuousUpdateChannel;
}

void IUpdateChecker::setUseContinuousUpdateChannel(const bool use)
{
    m_useContinuousUpdateChannel = use;
}

////////////////////////////////////////////////////////////////////////////////

IUpdateChecker * newUpdateChecker(
    const UpdateProvider updateProvider, QObject * parent)
{
    switch(updateProvider)
    {
    case UpdateProvider::APPIMAGE:
#if QUENTIER_PACKAGED_AS_APP_IMAGE
        return new AppImageUpdateChecker(parent);
#endif
    case UpdateProvider::GITHUB:
    default:
        return new GitHubUpdateChecker(parent);
    }
}

} // namespace quentier
