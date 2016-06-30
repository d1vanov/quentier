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
