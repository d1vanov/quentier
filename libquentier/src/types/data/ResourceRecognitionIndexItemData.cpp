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

#include "ResourceRecognitionIndexItemData.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

ResourceRecognitionIndexItemData::ResourceRecognitionIndexItemData() :
    m_x(-1),
    m_y(-1),
    m_h(-1),
    m_w(-1),
    m_offset(-1),
    m_duration(-1),
    m_strokeList(),
    m_textItems(),
    m_objectItems(),
    m_shapeItems(),
    m_barcodeItems()
{}

bool ResourceRecognitionIndexItemData::isValid() const
{
    if (m_textItems.isEmpty() && m_objectItems.isEmpty() && m_shapeItems.isEmpty() && m_barcodeItems.isEmpty()) {
        QNTRACE(QStringLiteral("Resource recognition index item is empty"));
        return false;
    }

    for(auto it = m_textItems.constBegin(), end = m_textItems.constEnd(); it != end; ++it)
    {
        const TextItem & textItem = *it;

        if (textItem.m_weight < 0) {
            QNTRACE(QStringLiteral("Resource recognition index item contains text item with weight less than 0: ")
                    << textItem.m_text << QStringLiteral(", weight = ") << textItem.m_weight);
            return false;
        }
    }

    for(auto it = m_objectItems.constBegin(), end = m_objectItems.constEnd(); it != end; ++it)
    {
        const ObjectItem & objectItem = *it;

        if (objectItem.m_weight < 0) {
            QNTRACE(QStringLiteral("Resource recognition index item contains object item with weight less than 0: ")
                    << objectItem.m_objectType << QStringLiteral(", weight = ") << objectItem.m_weight);
            return false;
        }

        if ((objectItem.m_objectType != QStringLiteral("face")) && (objectItem.m_objectType != QStringLiteral("sky")) &&
            (objectItem.m_objectType != QStringLiteral("ground")) && (objectItem.m_objectType != QStringLiteral("water")) &&
            (objectItem.m_objectType != QStringLiteral("lake")) && (objectItem.m_objectType != QStringLiteral("sea")) &&
            (objectItem.m_objectType != QStringLiteral("snow")) && (objectItem.m_objectType != QStringLiteral("mountains")) &&
            (objectItem.m_objectType != QStringLiteral("verdure")) && (objectItem.m_objectType != QStringLiteral("grass")) &&
            (objectItem.m_objectType != QStringLiteral("trees")) && (objectItem.m_objectType != QStringLiteral("building")) &&
            (objectItem.m_objectType != QStringLiteral("road")) && (objectItem.m_objectType != QStringLiteral("car")))
        {
            QNTRACE(QStringLiteral("Resource recognition index object item has invalid object type: ") << objectItem.m_objectType);
            return false;
        }
    }

    for(auto it = m_shapeItems.constBegin(), end = m_shapeItems.constEnd(); it != end; ++it)
    {
        const ShapeItem & shapeItem = *it;

        if (shapeItem.m_weight < 0) {
            QNTRACE(QStringLiteral("Resource recognition index item contains shape item with weight less than 0: ")
                    << shapeItem.m_shapeType << QStringLiteral(", weight = ") << shapeItem.m_weight);
            return false;
        }

        if ((shapeItem.m_shapeType != QStringLiteral("circle")) && (shapeItem.m_shapeType != QStringLiteral("oval")) &&
            (shapeItem.m_shapeType != QStringLiteral("rectangle")) && (shapeItem.m_shapeType != QStringLiteral("triangle")) &&
            (shapeItem.m_shapeType != QStringLiteral("line")) && (shapeItem.m_shapeType != QStringLiteral("arrow")) &&
            (shapeItem.m_shapeType != QStringLiteral("polyline")))
        {
            QNTRACE(QStringLiteral("Resource recognition index shape item has invalid shape type: ") << shapeItem.m_shapeType);
            return false;
        }
    }

    for(auto it = m_barcodeItems.constBegin(), end = m_barcodeItems.constEnd(); it != end; ++it)
    {
        const BarcodeItem & barcodeItem = *it;

        if (barcodeItem.m_weight < 0) {
            QNTRACE(QStringLiteral("Resource recognition index item contains barcode item with weight less than 0: ")
                    << barcodeItem.m_barcode << QStringLiteral(", weight = ") << barcodeItem.m_weight);
            return false;
        }
    }

    return true;
}

} // namespace quentier
