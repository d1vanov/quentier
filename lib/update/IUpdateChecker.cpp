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

QString IUpdateChecker::providerName() const
{
    return m_providerName;
}

void IUpdateChecker::setProviderName(QString name)
{
    m_providerName = std::move(name);
}

} // namespace quentier
