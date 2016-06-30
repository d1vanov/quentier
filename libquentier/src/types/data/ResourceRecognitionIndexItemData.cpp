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
        QNTRACE("Resource recognition index item is empty");
        return false;
    }

    for(auto it = m_textItems.begin(), end = m_textItems.end(); it != end; ++it)
    {
        const TextItem & textItem = *it;

        if (textItem.m_weight < 0) {
            QNTRACE("Resource recognition index item contains text item with weight less than 0: " << textItem.m_text
                    << ", weight = " << textItem.m_weight);
            return false;
        }
    }

    for(auto it = m_objectItems.begin(), end = m_objectItems.end(); it != end; ++it)
    {
        const ObjectItem & objectItem = *it;

        if (objectItem.m_weight < 0) {
            QNTRACE("Resource recognition index item contains object item with weight less than 0: " << objectItem.m_objectType
                    << ", weight = " << objectItem.m_weight);
            return false;
        }

        if ((objectItem.m_objectType != "face") && (objectItem.m_objectType != "sky") && (objectItem.m_objectType != "ground") &&
            (objectItem.m_objectType != "water") && (objectItem.m_objectType != "lake") && (objectItem.m_objectType != "sea") &&
            (objectItem.m_objectType != "snow") && (objectItem.m_objectType != "mountains") && (objectItem.m_objectType != "verdure") &&
            (objectItem.m_objectType != "grass") && (objectItem.m_objectType != "trees") && (objectItem.m_objectType != "building") &&
            (objectItem.m_objectType != "road") && (objectItem.m_objectType != "car"))
        {
            QNTRACE("Resource recognition index object item has invalid object type: " << objectItem.m_objectType);
            return false;
        }
    }

    for(auto it = m_shapeItems.begin(), end = m_shapeItems.end(); it != end; ++it)
    {
        const ShapeItem & shapeItem = *it;

        if (shapeItem.m_weight < 0) {
            QNTRACE("Resource recognition index item contains shape item with weight less than 0: " << shapeItem.m_shapeType
                    << ", weight = " << shapeItem.m_weight);
            return false;
        }

        if ((shapeItem.m_shapeType != "circle") && (shapeItem.m_shapeType != "oval") && (shapeItem.m_shapeType != "rectangle") &&
            (shapeItem.m_shapeType != "triangle") && (shapeItem.m_shapeType != "line") && (shapeItem.m_shapeType != "arrow") &&
            (shapeItem.m_shapeType != "polyline"))
        {
            QNTRACE("Resource recognition index shape item has invalid shape type: " << shapeItem.m_shapeType);
            return false;
        }
    }

    for(auto it = m_barcodeItems.begin(), end = m_barcodeItems.end(); it != end; ++it)
    {
        const BarcodeItem & barcodeItem = *it;

        if (barcodeItem.m_weight < 0) {
            QNTRACE("Resource recognition index item contains barcode item with weight less than 0: " << barcodeItem.m_barcode
                    << ", weight = " << barcodeItem.m_weight);
            return false;
        }
    }

    return true;
}

} // namespace quentier
