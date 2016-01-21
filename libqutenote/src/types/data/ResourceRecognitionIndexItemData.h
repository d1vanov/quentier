#ifndef __LIB_QUTE_NOTE__TYPES__DATA__RESOURCE_RECOGNITION_INDEX_ITEM_DATA_H
#define __LIB_QUTE_NOTE__TYPES__DATA__RESOURCE_RECOGNITION_INDEX_ITEM_DATA_H

#include <QSharedData>
#include <QVector>
#include <QString>

namespace qute_note {

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
    QString         m_type;
    QString         m_objectType;
    QString         m_shapeType;
    QString         m_content;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__DATA__RESOURCE_RECOGNITION_INDEX_ITEM_DATA_H
