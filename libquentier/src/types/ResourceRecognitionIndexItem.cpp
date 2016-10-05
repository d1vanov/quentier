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

#include <quentier/types/ResourceRecognitionIndexItem.h>
#include "data/ResourceRecognitionIndexItemData.h"

namespace quentier {

ResourceRecognitionIndexItem::ResourceRecognitionIndexItem() :
    Printable(),
    d(new ResourceRecognitionIndexItemData)
{}

ResourceRecognitionIndexItem::ResourceRecognitionIndexItem(const ResourceRecognitionIndexItem & other) :
    Printable(),
    d(other.d)
{}

ResourceRecognitionIndexItem & ResourceRecognitionIndexItem::operator=(const ResourceRecognitionIndexItem & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

ResourceRecognitionIndexItem::~ResourceRecognitionIndexItem()
{}

bool ResourceRecognitionIndexItem::isValid() const
{
    return d->isValid();
}

int ResourceRecognitionIndexItem::x() const
{
    return d->m_x;
}

void ResourceRecognitionIndexItem::setX(const int x)
{
    d->m_x = x;
}

int ResourceRecognitionIndexItem::y() const
{
    return d->m_y;
}

void ResourceRecognitionIndexItem::setY(const int y)
{
    d->m_y = y;
}

int ResourceRecognitionIndexItem::h() const
{
    return d->m_h;
}

void ResourceRecognitionIndexItem::setH(const int h)
{
    d->m_h = h;
}

int ResourceRecognitionIndexItem::w() const
{
    return d->m_w;
}

void ResourceRecognitionIndexItem::setW(const int w)
{
    d->m_w = w;
}

int ResourceRecognitionIndexItem::offset() const
{
    return d->m_offset;
}

void ResourceRecognitionIndexItem::setOffset(const int offset)
{
    d->m_offset = offset;
}

int ResourceRecognitionIndexItem::duration() const
{
    return d->m_duration;
}

void ResourceRecognitionIndexItem::setDuration(const int duration)
{
    d->m_duration = duration;
}

QVector<int> ResourceRecognitionIndexItem::strokeList() const
{
    return d->m_strokeList;
}

int ResourceRecognitionIndexItem::numStrokes() const
{
    return d->m_strokeList.size();
}

bool ResourceRecognitionIndexItem::strokeAt(const int strokeIndex, int & stroke) const
{
    if ((strokeIndex < 0) || (d->m_strokeList.size() <= strokeIndex)) {
        return false;
    }

    stroke = d->m_strokeList[strokeIndex];
    return true;
}

bool ResourceRecognitionIndexItem::setStrokeAt(const int strokeIndex, const int stroke)
{
    if ((strokeIndex < 0) || (d.constData()->m_strokeList.size() <= strokeIndex)) {
        return false;
    }

    d->m_strokeList[strokeIndex] = stroke;
    return true;
}

void ResourceRecognitionIndexItem::setStrokeList(const QVector<int> & strokeList)
{
    d->m_strokeList = strokeList;
}

void ResourceRecognitionIndexItem::reserveStrokeListSpace(const int numItems)
{
    d->m_strokeList.reserve(numItems);
}

void ResourceRecognitionIndexItem::addStroke(const int stroke)
{
    d->m_strokeList.push_back(stroke);
}

bool ResourceRecognitionIndexItem::removeStroke(const int stroke)
{
    const QVector<int> & strokeList = d.constData()->m_strokeList;
    int index = strokeList.indexOf(stroke);
    if (index < 0) {
        return false;
    }

    d->m_strokeList.remove(index);
    return true;
}

bool ResourceRecognitionIndexItem::removeStrokeAt(const int strokeIndex)
{
    const QVector<int> & strokeList = d.constData()->m_strokeList;
    if (strokeList.size() <= strokeIndex) {
        return false;
    }

    d->m_strokeList.remove(strokeIndex);
    return true;
}

QVector<ResourceRecognitionIndexItem::TextItem> ResourceRecognitionIndexItem::textItems() const
{
    return d->m_textItems;
}

int ResourceRecognitionIndexItem::numTextItems() const
{
    return d->m_textItems.size();
}

bool ResourceRecognitionIndexItem::textItemAt(const int textItemIndex, TextItem & item) const
{
    if ((textItemIndex < 0) || (d->m_textItems.size() <= textItemIndex)) {
        return false;
    }

    item = d->m_textItems[textItemIndex];
    return true;
}

bool ResourceRecognitionIndexItem::setTextItemAt(const int textItemIndex, const TextItem & textItem)
{
    if ((textItemIndex < 0) || (d.constData()->m_textItems.size() <= textItemIndex)) {
        return false;
    }

    d->m_textItems[textItemIndex] = textItem;
    return true;
}

void ResourceRecognitionIndexItem::setTextItems(const QVector<TextItem> & textItems)
{
    d->m_textItems = textItems;
}

void ResourceRecognitionIndexItem::reserveTextItemsSpace(const int numItems)
{
    d->m_textItems.reserve(numItems);
}

void ResourceRecognitionIndexItem::addTextItem(const TextItem & item)
{
    d->m_textItems.push_back(item);
}

bool ResourceRecognitionIndexItem::removeTextItem(const TextItem & item)
{
    const QVector<TextItem> & textItems = d.constData()->m_textItems;
    int index = textItems.indexOf(item);
    if (index < 0) {
        return false;
    }

    d->m_textItems.remove(index);
    return true;
}

bool ResourceRecognitionIndexItem::removeTextItemAt(const int textItemIndex)
{
    const QVector<TextItem> & textItems = d.constData()->m_textItems;
    if (textItems.size() <= textItemIndex) {
        return false;
    }

    d->m_textItems.remove(textItemIndex);
    return true;
}

QVector<ResourceRecognitionIndexItem::ObjectItem> ResourceRecognitionIndexItem::objectItems() const
{
    return d->m_objectItems;
}

int ResourceRecognitionIndexItem::numObjectItems() const
{
    return d->m_objectItems.size();
}

bool ResourceRecognitionIndexItem::objectItemAt(const int objectItemIndex, ObjectItem & objectItem) const
{
    if ((objectItemIndex < 0) || (d->m_objectItems.size() <= objectItemIndex)) {
        return false;
    }

    objectItem = d->m_objectItems[objectItemIndex];
    return true;
}

bool ResourceRecognitionIndexItem::setObjectItemAt(const int objectItemIndex, const ObjectItem & objectItem)
{
    if ((objectItemIndex < 0) || (d.constData()->m_objectItems.size() <= objectItemIndex)) {
        return false;
    }

    d->m_objectItems[objectItemIndex] = objectItem;
    return true;
}

void ResourceRecognitionIndexItem::setObjectItems(const QVector<ObjectItem> & objectItems)
{
    d->m_objectItems = objectItems;
}

void ResourceRecognitionIndexItem::reserveObjectItemsSpace(const int numItems)
{
    d->m_objectItems.reserve(numItems);
}

void ResourceRecognitionIndexItem::addObjectItem(const ObjectItem & item)
{
    d->m_objectItems.push_back(item);
}

bool ResourceRecognitionIndexItem::removeObjectItem(const ObjectItem & item)
{
    const QVector<ObjectItem> & objectItems = d.constData()->m_objectItems;
    int index = objectItems.indexOf(item);
    if (index < 0) {
        return false;
    }

    d->m_objectItems.remove(index);
    return true;
}

bool ResourceRecognitionIndexItem::removeObjectItemAt(const int objectItemIndex)
{
    const QVector<ObjectItem> & objectItems = d.constData()->m_objectItems;
    if (objectItems.size() <= objectItemIndex) {
        return false;
    }

    d->m_objectItems.remove(objectItemIndex);
    return true;
}

QVector<ResourceRecognitionIndexItem::ShapeItem> ResourceRecognitionIndexItem::shapeItems() const
{
    return d->m_shapeItems;
}

int ResourceRecognitionIndexItem::numShapeItems() const
{
    return d->m_shapeItems.size();
}

bool ResourceRecognitionIndexItem::shapeItemAt(const int shapeItemIndex, ShapeItem & shapeItem) const
{
    if ((shapeItemIndex < 0) || (d->m_shapeItems.size() <= shapeItemIndex)) {
        return false;
    }

    shapeItem = d->m_shapeItems[shapeItemIndex];
    return true;
}

bool ResourceRecognitionIndexItem::setShapeItemAt(const int shapeItemIndex, const ShapeItem & shapeItem)
{
    if ((shapeItemIndex < 0) || (d.constData()->m_shapeItems.size() <= shapeItemIndex)) {
        return false;
    }

    d->m_shapeItems[shapeItemIndex] = shapeItem;
    return true;
}

void ResourceRecognitionIndexItem::setShapeItems(const QVector<ShapeItem> & shapeItems)
{
    d->m_shapeItems = shapeItems;
}

void ResourceRecognitionIndexItem::reserveShapeItemsSpace(const int numItems)
{
    d->m_shapeItems.reserve(numItems);
}

void ResourceRecognitionIndexItem::addShapeItem(const ShapeItem & item)
{
    d->m_shapeItems.push_back(item);
}

bool ResourceRecognitionIndexItem::removeShapeItem(const ShapeItem & item)
{
    const QVector<ShapeItem> & shapeItems = d.constData()->m_shapeItems;
    int index = shapeItems.indexOf(item);
    if (index < 0) {
        return false;
    }

    d->m_shapeItems.remove(index);
    return true;
}

bool ResourceRecognitionIndexItem::removeShapeItemAt(const int shapeItemIndex)
{
    const QVector<ShapeItem> & shapeItems = d.constData()->m_shapeItems;
    if (shapeItems.size() <= shapeItemIndex) {
        return false;
    }

    d->m_shapeItems.remove(shapeItemIndex);
    return true;
}

QVector<ResourceRecognitionIndexItem::BarcodeItem> ResourceRecognitionIndexItem::barcodeItems() const
{
    return d->m_barcodeItems;
}

int ResourceRecognitionIndexItem::numBarcodeItems() const
{
    return d->m_barcodeItems.size();
}

bool ResourceRecognitionIndexItem::barcodeItemAt(const int barcodeItemIndex, BarcodeItem & barcodeItem) const
{
    if ((barcodeItemIndex < 0) || (d->m_barcodeItems.size() <= barcodeItemIndex)) {
        return false;
    }

    barcodeItem = d->m_barcodeItems[barcodeItemIndex];
    return true;
}

bool ResourceRecognitionIndexItem::setBarcodeItemAt(const int barcodeItemIndex, const BarcodeItem & barcodeItem)
{
    if ((barcodeItemIndex < 0) || (d.constData()->m_barcodeItems.size() <= barcodeItemIndex)) {
        return false;
    }

    d->m_barcodeItems[barcodeItemIndex] = barcodeItem;
    return true;
}

void ResourceRecognitionIndexItem::setBarcodeItems(const QVector<BarcodeItem> & barcodeItems)
{
    d->m_barcodeItems = barcodeItems;
}

void ResourceRecognitionIndexItem::reserveBarcodeItemsSpace(const int numItems)
{
    d->m_barcodeItems.reserve(numItems);
}

void ResourceRecognitionIndexItem::addBarcodeItem(const BarcodeItem & item)
{
    d->m_barcodeItems.push_back(item);
}

bool ResourceRecognitionIndexItem::removeBarcodeItem(const BarcodeItem & item)
{
    const QVector<BarcodeItem> & barcodeItems = d.constData()->m_barcodeItems;
    int index = barcodeItems.indexOf(item);
    if (index < 0) {
        return false;
    }

    d->m_barcodeItems.remove(index);
    return true;
}

bool ResourceRecognitionIndexItem::removeBarcodeItemAt(const int barcodeItemIndex)
{
    const QVector<BarcodeItem> & barcodeItems = d.constData()->m_barcodeItems;
    if (barcodeItems.size() <= barcodeItemIndex) {
        return false;
    }

    d->m_barcodeItems.remove(barcodeItemIndex);
    return true;
}

QTextStream & ResourceRecognitionIndexItem::print(QTextStream & strm) const
{
    strm << QStringLiteral("ResourceRecognitionIndexItem: {\n");

    if (d->m_x >= 0) {
        strm << QStringLiteral("  x = ") << d->m_x << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  x is not set;\n");
    }

    if (d->m_y >= 0) {
        strm << QStringLiteral("  y = ") << d->m_y << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  y is not set;\n");
    }

    if (d->m_h >= 0) {
        strm << QStringLiteral("  h = ") << d->m_h << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  h is not set;\n");
    }

    if (d->m_w >= 0) {
        strm << QStringLiteral("  w = ") << d->m_w << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  w is not set;\n");
    }

    if (d->m_offset >= 0) {
        strm << QStringLiteral("  offset = ") << d->m_offset << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  offset is not set;\n");
    }

    if (d->m_duration >= 0) {
        strm << QStringLiteral("  duration = ") << d->m_duration << QStringLiteral(";\n");
    }
    else {
        strm << QStringLiteral("  duration is not set;\n");
    }

    if (!d->m_strokeList.isEmpty())
    {
        strm << QStringLiteral("  stroke list: ");
        for(auto it = d->m_strokeList.constBegin(), end = d->m_strokeList.constEnd(); it != end; ++it) {
            strm << *it << QStringLiteral(" ");
        }
        strm << QStringLiteral(";\n");
    }
    else
    {
        strm << QStringLiteral("  stroke list is not set;\n");
    }

    if (!d->m_textItems.isEmpty())
    {
        strm << QStringLiteral("  text items: \n");
        for(auto it = d->m_textItems.constBegin(), end = d->m_textItems.constEnd(); it != end; ++it) {
            strm << QStringLiteral("    text: ") << it->m_text << QStringLiteral("; weight = ") << it->m_weight
                 << QStringLiteral(";\n");
        }
    }
    else
    {
        strm << QStringLiteral("  text items are not set;\n");
    }

    if (!d->m_objectItems.isEmpty())
    {
        strm << QStringLiteral("  object items: \n");
        for(auto it = d->m_objectItems.constBegin(), end = d->m_objectItems.constEnd(); it != end; ++it) {
            strm << QStringLiteral("    object type: ") << it->m_objectType << QStringLiteral("; weight: ")
                 << it->m_weight << QStringLiteral(";\n");
        }
    }
    else
    {
        strm << QStringLiteral("  object items are not set;\n");
    }

    if (!d->m_shapeItems.isEmpty())
    {
        strm << QStringLiteral("  shape items: \n");
        for(auto it = d->m_shapeItems.constBegin(), end = d->m_shapeItems.constEnd(); it != end; ++it) {
            strm << QStringLiteral("    shape type: ") << it->m_shapeType << QStringLiteral("; weight: ") << it->m_weight
                 << QStringLiteral(";\n");
        }
    }
    else
    {
        strm << QStringLiteral("  shape items are not set;\n");
    }

    if (!d->m_barcodeItems.isEmpty())
    {
        strm << QStringLiteral("  barcode items: \n");
        for(auto it = d->m_barcodeItems.constBegin(), end = d->m_barcodeItems.constEnd(); it != end; ++it) {
            strm << QStringLiteral("    barcode: ") << it->m_barcode << QStringLiteral("; weight: ") << it->m_weight
                 << QStringLiteral(";\n");
        }
    }
    else
    {
        strm << QStringLiteral("  barcode items are not set;\n");
    }

    strm << QStringLiteral("};\n");

    return strm;
}

} // namespace quentier
