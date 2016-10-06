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

#include "data/ResourceWrapperData.h"
#include <quentier/types/ResourceWrapper.h>

namespace quentier {

ResourceWrapper::ResourceWrapper() :
    IResource(),
    d(new ResourceWrapperData)
{}

ResourceWrapper::ResourceWrapper(const IResource & other) :
    IResource(other),
    d(new ResourceWrapperData(other.GetEnResource()))
{}

ResourceWrapper::ResourceWrapper(const ResourceWrapper & other) :
    IResource(other),
    d(other.d)
{}

ResourceWrapper::ResourceWrapper(ResourceWrapper && other) :
    IResource(std::move(other)),
    d(other.d)
{}

ResourceWrapper::ResourceWrapper(const qevercloud::Resource & other) :
    IResource(),
    d(new ResourceWrapperData(other))
{}

ResourceWrapper::ResourceWrapper(qevercloud::Resource && other) :
    IResource(),
    d(new ResourceWrapperData(other))
{}

ResourceWrapper & ResourceWrapper::operator=(const ResourceWrapper & other)
{
    IResource::operator=(other);

    if (this != &other) {
        d = other.d;
    }

    return *this;
}

ResourceWrapper & ResourceWrapper::operator=(ResourceWrapper && other)
{
    IResource::operator=(other);

    if (this != &other) {
        d = other.d;
    }

    return *this;
}

ResourceWrapper::~ResourceWrapper()
{}

const qevercloud::Resource & ResourceWrapper::GetEnResource() const
{
    return d->m_qecResource;
}

qevercloud::Resource & ResourceWrapper::GetEnResource()
{
    return d->m_qecResource;
}

QTextStream & ResourceWrapper::print(QTextStream & strm) const
{
    strm << QStringLiteral("ResourceWrapper { \n");
    IResource::print(strm);
    strm << QStringLiteral("} \n");

    return strm;
}

} // namespace quentier
