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

#ifndef LIB_QUENTIER_TYPES_DATA_RESOURCE_RECOGNITION_INDEX_ITEM_DATA_H
#define LIB_QUENTIER_TYPES_DATA_RESOURCE_RECOGNITION_INDEX_ITEM_DATA_H

#include <quentier/types/ResourceRecognitionIndexItem.h>
#include <QSharedData>
#include <QVector>
#include <QString>

namespace quentier {
class ResourceRecognitionIndexItemData: public QSharedData
{
public:
    ResourceRecognitionIndexItemData();

    bool isValid() const;

    int m_x;
    int m_y;
    int m_h;
    int m_w;

    int m_offset;
    int m_duration;

    QVector<int>    m_strokeList;

    typedef ResourceRecognitionIndexItem::TextItem TextItem;
    QVector<TextItem>   m_textItems;

    typedef ResourceRecognitionIndexItem::ObjectItem ObjectItem;
    QVector<ObjectItem> m_objectItems;

    typedef ResourceRecognitionIndexItem::ShapeItem ShapeItem;
    QVector<ShapeItem>  m_shapeItems;

    typedef ResourceRecognitionIndexItem::BarcodeItem BarcodeItem;
    QVector<BarcodeItem>    m_barcodeItems;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_RESOURCE_RECOGNITION_INDEX_ITEM_DATA_H
