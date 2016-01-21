#ifndef __LIB_QUTE_NOTE__TYPES__DATA__RESOURCE_RECOGNITION_INDICES_DATA_H
#define __LIB_QUTE_NOTE__TYPES__DATA__RESOURCE_RECOGNITION_INDICES_DATA_H

#include <qute_note/types/ResourceRecognitionIndexItem.h>
#include <QString>
#include <QByteArray>
#include <QSharedData>

namespace qute_note {

class ResourceRecognitionIndicesData: public QSharedData
{
public:
    ResourceRecognitionIndicesData();

    bool isValid() const;
    void setData(const QByteArray & rawRecognitionIndicesData);

    bool        m_isNull;

    QString     m_objectId;
    QString     m_objectType;
    QString     m_recoType;
    QString     m_engineVersion;
    QString     m_docType;
    QString     m_lang;
    int         m_objectHeight;
    int         m_objectWidth;

    QVector<ResourceRecognitionIndexItem>   m_items;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__DATA__RESOURCE_RECOGNITION_INDICES_DATA_H

