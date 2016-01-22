#ifndef __LIB_QUTE_NOTE__TYPES__RESOURCE_RECOGNITION_INDEX_ITEM_H
#define __LIB_QUTE_NOTE__TYPES__RESOURCE_RECOGNITION_INDEX_ITEM_H

#include <qute_note/utility/Linkage.h>
#include <qute_note/utility/Printable.h>
#include <QByteArray>
#include <QSharedDataPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ResourceRecognitionIndexItemData)

class QUTE_NOTE_EXPORT ResourceRecognitionIndexItem: public Printable
{
public:
    ResourceRecognitionIndexItem();

    bool isValid() const;

    int x() const;
    void setX(const int x);

    int y() const;
    void setY(const int y);

    int h() const;
    void setH(const int h);

    int w() const;
    void setW(const int w);

    int offset() const;
    void setOffset(const int offset);

    int duration() const;
    void setDuration(const int duration);

    QVector<int> strokeList() const;
    void setStrokeList(const QVector<int> & strokeList);
    void reserveStrokeListSpace(const int numItems);
    void addStroke(const int stroke);
    bool removeStroke(const int stroke);
    bool removeStrokeAt(const int strokeIndex);

    struct TextItem
    {
        TextItem() : m_text(), m_weight(-1) {}

        bool operator==(const TextItem & other) const { return (m_text == other.m_text) && (m_weight == other.m_weight); }

        QString     m_text;
        int         m_weight;
    };

    QVector<TextItem> textItems() const;
    void setTextItems(const QVector<TextItem> & textItems);
    void reserveTextItemsSpace(const int numItems);
    void addTextItem(const TextItem & item);
    bool removeTextItem(const TextItem & item);
    bool removeTextItemAt(const int textItemIndex);

    struct ObjectItem
    {
        ObjectItem() : m_objectType(), m_weight(-1) {}

        bool operator==(const ObjectItem & other) const { return (m_objectType == other.m_objectType) && (m_weight == other.m_weight); }

        QString     m_objectType;
        int         m_weight;
    };

    QVector<ObjectItem> objectItems() const;
    void setObjectItems(const QVector<ObjectItem> & objectItems);
    void reserveObjectItemsSpace(const int numItems);
    void addObjectItem(const ObjectItem & item);
    bool removeObjectItem(const ObjectItem & item);
    bool removeObjectItemAt(const int objectItemIndex);

    struct ShapeItem
    {
        ShapeItem() : m_shapeType(), m_weight(-1) {}

        bool operator==(const ShapeItem & other) const { return (m_shapeType == other.m_shapeType) && (m_weight == other.m_weight); }

        QString     m_shapeType;
        int         m_weight;
    };

    QVector<ShapeItem> shapeItems() const;
    void setShapeItems(const QVector<ShapeItem> & shapeItems);
    void reserveShapeItemsSpace(const int numItems);
    void addShapeItem(const ShapeItem & item);
    bool removeShapeItem(const ShapeItem & item);
    bool removeShapeItemAt(const int shapeItemIndex);

    struct BarcodeItem
    {
        BarcodeItem() : m_barcode(), m_weight(-1) {}

        bool operator==(const BarcodeItem & other) const { return (m_barcode == other.m_barcode) && (m_weight == other.m_weight); }

        QString     m_barcode;
        int         m_weight;
    };

    QVector<BarcodeItem> barcodeItems() const;
    void setBarcodeItems(const QVector<BarcodeItem> & barcodeItems);
    void reserveBarcodeItemsSpace(const int numItems);
    void addBarcodeItem(const BarcodeItem & item);
    bool removeBarcodeItem(const BarcodeItem & item);
    bool removeBarcodeItemAt(const int barcodeItemIndex);

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<ResourceRecognitionIndexItemData> d;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__RESOURCE_RECOGNITION_INDEX_ITEM_H
