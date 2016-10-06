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

#include <quentier/types/ResourceRecognitionIndices.h>
#include "data/ResourceRecognitionIndicesData.h"

namespace quentier {

ResourceRecognitionIndices::ResourceRecognitionIndices() :
    Printable(),
    d(new ResourceRecognitionIndicesData)
{}

ResourceRecognitionIndices::ResourceRecognitionIndices(const ResourceRecognitionIndices & other) :
    Printable(),
    d(other.d)
{}

ResourceRecognitionIndices::ResourceRecognitionIndices(const QByteArray & rawRecognitionIndicesData) :
    d(new ResourceRecognitionIndicesData)
{
    d->setData(rawRecognitionIndicesData);
}

ResourceRecognitionIndices & ResourceRecognitionIndices::operator=(const ResourceRecognitionIndices & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

ResourceRecognitionIndices::~ResourceRecognitionIndices()
{}

bool ResourceRecognitionIndices::isNull() const
{
    return d->m_isNull;
}

bool ResourceRecognitionIndices::isValid() const
{
    return d->isValid();
}

QString ResourceRecognitionIndices::objectId() const
{
    return d->m_objectId;
}

QString ResourceRecognitionIndices::objectType() const
{
    return d->m_objectType;
}

QString ResourceRecognitionIndices::recoType() const
{
    return d->m_recoType;
}

QString ResourceRecognitionIndices::engineVersion() const
{
    return d->m_engineVersion;
}

QString ResourceRecognitionIndices::docType() const
{
    return d->m_docType;
}

QString ResourceRecognitionIndices::lang() const
{
    return d->m_lang;
}

int ResourceRecognitionIndices::objectHeight() const
{
    return d->m_objectHeight;
}

int ResourceRecognitionIndices::objectWidth() const
{
    return d->m_objectWidth;
}

QVector<ResourceRecognitionIndexItem> ResourceRecognitionIndices::items() const
{
    return d->m_items;
}

bool ResourceRecognitionIndices::setData(const QByteArray & rawRecognitionIndicesData)
{
    return d->setData(rawRecognitionIndicesData);
}

QTextStream & ResourceRecognitionIndices::print(QTextStream & strm) const
{
    strm << QStringLiteral("ResourceRecognitionIndices: {\n");

    if (isNull()) {
        strm << QStringLiteral("  <null>\n};\n");
        return strm;
    }

    if (!d->m_objectId.isEmpty()) {
        strm << QStringLiteral("  object id = ") << d->m_objectId << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  object id is not set;\n");
    }

    if (!d->m_objectType.isEmpty()) {
        strm << QStringLiteral("  object type = ") << d->m_objectType << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  object type is not set;\n");
    }

    if (!d->m_recoType.isEmpty()) {
        strm << QStringLiteral("  reco type = ") << d->m_recoType << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  reco type is not set;\n");
    }

    if (!d->m_engineVersion.isEmpty()) {
        strm << QStringLiteral("  engine version = ") << d->m_engineVersion << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  engine version is not set;\n");
    }

    if (!d->m_docType.isEmpty()) {
        strm << QStringLiteral("  doc type = ") << d->m_docType << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  doc type is not set;\n");
    }

    if (!d->m_lang.isEmpty()) {
        strm << QStringLiteral("  lang = ") << d->m_lang << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  lang is not set;\n");
    }

    if (d->m_objectHeight >= 0) {
        strm << QStringLiteral("  object height = ") << d->m_objectHeight << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  object height is not set;\n");
    }

    if (d->m_objectWidth >= 0) {
        strm << QStringLiteral("  object width = ") << d->m_objectWidth << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  object width is not set;\n");
    }

    if (!d->m_items.isEmpty())
    {
        strm << QStringLiteral("  recognition items: \n");
        for(auto it = d->m_items.constBegin(), end = d->m_items.constEnd(); it != end; ++it) {
            strm << *it << QStringLiteral("\n");
        }
        strm << QStringLiteral("\n");
    }
    else
    {
        strm << QStringLiteral("  no recognition items are set;\n");
    }

    strm << QStringLiteral("};\n");

    return strm;
}

} // namespace quentier
