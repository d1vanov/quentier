#include <qute_note/types/ResourceRecognitionIndices.h>
#include "data/ResourceRecognitionIndicesData.h"

namespace qute_note {

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
    strm << "ResourceRecognitionIndices: {\n";

    if (isNull()) {
        strm << "  <null>\n};\n";
        return strm;
    }

    if (!d->m_objectId.isEmpty()) {
        strm << "  object id = " << d->m_objectId << ";\n";
    }
    else {
        strm << "  object id is not set;\n";
    }

    if (!d->m_objectType.isEmpty()) {
        strm << "  object type = " << d->m_objectType << ";\n";
    }
    else {
        strm << "  object type is not set;\n";
    }

    if (!d->m_recoType.isEmpty()) {
        strm << "  reco type = " << d->m_recoType << ";\n";
    }
    else {
        strm << "  reco type is not set;\n";
    }

    if (!d->m_engineVersion.isEmpty()) {
        strm << "  engine version = " << d->m_engineVersion << ";\n";
    }
    else {
        strm << "  engine version is not set;\n";
    }

    if (!d->m_docType.isEmpty()) {
        strm << "  doc type = " << d->m_docType << ";\n";
    }
    else {
        strm << "  doc type is not set;\n";
    }

    if (!d->m_lang.isEmpty()) {
        strm << "  lang = " << d->m_lang << ";\n";
    }
    else {
        strm << "  lang is not set;\n";
    }

    if (d->m_objectHeight >= 0) {
        strm << "  object height = " << d->m_objectHeight << ";\n";
    }
    else {
        strm << "  object height is not set;\n";
    }

    if (d->m_objectWidth >= 0) {
        strm << "  object width = " << d->m_objectWidth << ";\n";
    }
    else {
        strm << "  object width is not set;\n";
    }

    if (!d->m_items.isEmpty()) {
        strm << "  recognition items: \n";
        for(auto it = d->m_items.begin(), end = d->m_items.end(); it != end; ++it) {
            strm << *it << "\n";
        }
        strm << "\n";
    }
    else {
        strm << "  no recognition items are set;\n";
    }

    strm << "};\n";

    return strm;
}

} // namespace qute_note
