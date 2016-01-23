#include "ResourceRecognitionIndicesData.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <QXmlStreamReader>
#include <QStringList>

namespace qute_note {

ResourceRecognitionIndicesData::ResourceRecognitionIndicesData() :
    m_isNull(true),
    m_objectId(),
    m_objectType(),
    m_recoType(),
    m_engineVersion(),
    m_docType(),
    m_lang(),
    m_objectHeight(-1),
    m_objectWidth(-1),
    m_items()
{}

bool ResourceRecognitionIndicesData::isValid() const
{
    if (m_objectId.isEmpty()) {
        QNTRACE("Resource recognition indices' object id is not set");
        return false;
    }

    if (m_objectType.isEmpty())
    {
        QNTRACE("Resource recognition indices' object type is not set");
        return false;
    }
    else if ((m_objectType != "image") && (m_objectType != "ink") && (m_objectType != "audio") &&
        (m_objectType != "video") && (m_objectType != "document"))
    {
        QNTRACE("Resource recognition indices' object type is not valid");
        return false;
    }

    if (m_recoType.isEmpty()) {
        QNTRACE("Resource recognition indices' recognition type is not set");
        return false;
    }
    else if ((m_recoType != "service") && (m_recoType != "client")) {
        QNTRACE("Resource recognition indices' recognition type is not valid");
        return false;
    }

    if (m_docType.isEmpty())
    {
        QNTRACE("Resource recognition indices' doc type is not set");
        return false;
    }
    else if ((m_docType != "printed") && (m_docType != "speech") && (m_docType != "handwritten") &&
             (m_docType != "picture") && (m_docType != "unknown"))
    {
        QNTRACE("Resource recognition indices' doc type is not valid");
        return false;
    }

    return true;
}

void ResourceRecognitionIndicesData::setData(const QByteArray & rawRecognitionIndicesData)
{
    QNTRACE("ResourceRecognitionIndicesData::setData: " << rawRecognitionIndicesData);

    QXmlStreamReader reader(rawRecognitionIndicesData);

    QString lastElementName;
    QXmlStreamAttributes lastElementAttributes;

    ResourceRecognitionIndicesData backup(*this);
    clear();

    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (reader.isStartDocument()) {
            continue;
        }

        if (reader.isDTD()) {
            continue;
        }

        if (reader.isEndDocument()) {
            break;
        }

        if (reader.isStartElement())
        {
            lastElementName = reader.name().toString();
            lastElementAttributes = reader.attributes();

            if (lastElementName == "recoIndex") {
                parseRecoIndexAttributes(lastElementAttributes);
                continue;
            }

            if (lastElementName == "item") {
                ResourceRecognitionIndexItem item;
                parseCommonItemAttributes(lastElementAttributes, item);
                m_items << item;
                continue;
            }

            if (m_items.isEmpty()) {
                continue;
            }

            ResourceRecognitionIndexItem & item = m_items.last();

            if (lastElementName == "t") {
                parseTextItemAttributes(lastElementAttributes, item);
            }
            else if (lastElementName == "object") {
                parseObjectItemAttributes(lastElementAttributes, item);
            }
            else if (lastElementName == "shape") {
                // TODO: parse shape item's attributes into the last item
            }
            else if (lastElementName == "barcode") {
                // TODO: parse barcode item's attributes into the last item
            }
            else {
                continue;
            }
        }

        if (reader.isCharacters())
        {
            if (lastElementName == "t") {
                // TODO: parse text data
            }
            else if (lastElementName == "barcode") {
                // TODO: parse barcode data
            }
            else {
                continue;
            }
        }
    }

    if (reader.hasError()) {
        QNWARNING("Failed to parse resource recognition indices data: " << reader.errorString()
                 << " (error code " << reader.error() << ", original raw data: " << rawRecognitionIndicesData);
        restoreFrom(backup);
        return;
    }

    m_isNull = false;
    // TODO: put parsed stuff to the object's members
}

void ResourceRecognitionIndicesData::clear()
{
    QNTRACE("ResourceRecognitionIndicesData::clear");

    m_objectId.resize(0);
    m_objectType.resize(0);
    m_recoType.resize(0);
    m_engineVersion.resize(0);
    m_docType.resize(0);
    m_lang.resize(0);
    m_objectHeight = -1;
    m_objectWidth = -1;
    m_items.clear();

    m_isNull = true;
}

void ResourceRecognitionIndicesData::parseRecoIndexAttributes(const QXmlStreamAttributes & attributes)
{
    QNTRACE("ResourceRecognitionIndicesData::parseRecoIndexAttributes: " << attributes);

    for(auto it = attributes.begin(), end = attributes.end(); it != end; ++it)
    {
        const QXmlStreamAttribute & attribute = *it;

        const QStringRef & name = attribute.name();
        const QStringRef & value = attribute.value();

        if (name == "objID") {
            m_objectId = value.toString();
        }
        else if (name == "objType") {
            m_objectType = value.toString();
        }
        else if (name == "recoType") {
            m_recoType = value.toString();
        }
        else if (name == "engineVersion") {
            m_engineVersion = value.toString();
        }
        else if (name == "docType") {
            m_docType = value.toString();
        }
        else if (name == "lang") {
            m_lang = value.toString();
        }
        else if (name == "objHeight")
        {
            bool conversionResult = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
            int objectHeight = value.toInt(&conversionResult);
#else
            int objectHeight = value.toString().toInt(&conversionResult);
#endif
            if (conversionResult) {
                m_objectHeight = objectHeight;
            }
        }
        else if (name == "objWidth")
        {
            bool conversionResult = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
            int objectWidth = value.toInt(&conversionResult);
#else
            int objectWidth = value.toString().toInt(&conversionResult);
#endif
            if (conversionResult) {
                m_objectWidth = objectWidth;
            }
        }
    }
}

void ResourceRecognitionIndicesData::parseCommonItemAttributes(const QXmlStreamAttributes & attributes,
                                                                ResourceRecognitionIndexItem & item) const
{
    QNTRACE("ResourceRecognitionIndicesData::parseCommonItemAttributes: " << attributes);

    for(auto it = attributes.begin(), end = attributes.end(); it != end; ++it)
    {
        const QXmlStreamAttribute & attribute = *it;

        const QStringRef & name = attribute.name();
        const QStringRef & value = attribute.value();

        if (name == "x") {
            bool conversionResult = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
            int x = value.toInt(&conversionResult);
#else
            int x = value.toString().toInt(&conversionResult);
#endif
            if (conversionResult) {
                item.setX(x);
            }
        }
        else if (name == "y") {
            bool conversionResult = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
            int y = value.toInt(&conversionResult);
#else
            int y = value.toString().toInt(&conversionResult);
#endif
            if (conversionResult) {
                item.setY(y);
            }
        }
        else if (name == "h") {
            bool conversionResult = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
            int h = value.toInt(&conversionResult);
#else
            int h = value.toString().toInt(&conversionResult);
#endif
            if (conversionResult) {
                item.setH(h);
            }
        }
        else if (name == "w") {
            bool conversionResult = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
            int w = value.toInt(&conversionResult);
#else
            int w = value.toString().toInt(&conversionResult);
#endif
            if (conversionResult) {
                item.setW(w);
            }
        }
        else if (name == "offset") {
            bool conversionResult = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
            int offset = value.toInt(&conversionResult);
#else
            int offset = value.toString().toInt(&conversionResult);
#endif
            if (conversionResult) {
                item.setOffset(offset);
            }
        }
        else if (name == "duration") {
            bool conversionResult = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
            int duration = value.toInt(&conversionResult);
#else
            int duration = value.toString().toInt(&conversionResult);
#endif
            if (conversionResult) {
                item.setDuration(duration);
            }
        }
        else if (name == "strokeList") {
            QString valueStr = value.toString();
            QStringList valueList = valueStr.split(",", QString::SkipEmptyParts);
            const int numValues = valueList.size();
            for(int i = 0; i < numValues; ++i) {
                const QString & strokeStr = valueList[i];
                bool conversionResult = false;
                int stroke = strokeStr.toInt(&conversionResult);
                if (conversionResult) {
                    item.addStroke(stroke);
                }
            }
        }
    }
}

void ResourceRecognitionIndicesData::parseTextItemAttributes(const QXmlStreamAttributes & attributes,
                                                             ResourceRecognitionIndexItem & item) const
{
    QNTRACE("ResourceRecognitionIndicesData::parseTextItemAttributes: " << attributes);

    int weight = -1;

    for(auto it = attributes.begin(), end = attributes.end(); it != end; ++it)
    {
        const QXmlStreamAttribute & attribute = *it;

        const QStringRef & name = attribute.name();
        const QStringRef & value = attribute.value();

        if (name == "w") {
            bool conversionResult = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
            int parsedWeight = value.toInt(&conversionResult);
#else
            int parsedWeight = value.toString().toInt(&conversionResult);
#endif
            if (conversionResult) {
                weight = parsedWeight;
            }
        }
    }

    if (weight < 0) {
        return;
    }

    ResourceRecognitionIndexItem::TextItem textItem;
    textItem.m_weight = weight;
    item.addTextItem(textItem);
}

void ResourceRecognitionIndicesData::parseObjectItemAttributes(const QXmlStreamAttributes & attributes,
                                                               ResourceRecognitionIndexItem & item) const
{
    QNTRACE("ResourceRecognitionIndicesData::parseObjectItemAttributes: " << attributes);

    QString objectType;
    int weight = -1;

    for(auto it = attributes.begin(), end = attributes.end(); it != end; ++it)
    {
        const QXmlStreamAttribute & attribute = *it;

        const QStringRef & name = attribute.name();
        const QStringRef & value = attribute.value();

        if (name == "type") {
            objectType = value.toString();
        }
        else if (name == "w") {
            bool conversionResult = false;
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
            int parsedWeight = value.toInt(&conversionResult);
#else
            int parsedWeight = value.toString().toInt(&conversionResult);
#endif
            if (conversionResult) {
                weight = parsedWeight;
            }
        }
    }

    if (weight < 0) {
        return;
    }

    ResourceRecognitionIndexItem::ObjectItem objectItem;
    objectItem.m_objectType = objectType;
    objectItem.m_weight = weight;
    item.addObjectItem(objectItem);
    QNTRACE("Added object item: type = " << objectType << ", weight = " << weight);
}

void ResourceRecognitionIndicesData::restoreFrom(const ResourceRecognitionIndicesData & data)
{
    QNTRACE("ResourceRecognitionIndicesData::restoreFrom");

    m_isNull = data.m_isNull;

    m_objectId = data.m_objectId;
    m_objectType = data.m_objectType;
    m_recoType = data.m_recoType;
    m_engineVersion = data.m_engineVersion;
    m_docType = data.m_docType;
    m_lang = data.m_lang;
    m_objectHeight = data.m_objectHeight;
    m_objectWidth = data.m_objectWidth;
    m_items = data.m_items;
}

} // namespace qute_note
