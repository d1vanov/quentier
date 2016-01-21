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
    void addStroke(const int stroke);
    bool removeStroke(const int stroke);
    bool removeStrokeAt(const int strokeIndex);

    QString type() const;
    void setType(const QString & type);

    QString objectType() const;
    void setObjectType(const QString & objectType);

    QString shapeType() const;
    void setShapeType(const QString & shapeType);

    QString content() const;
    void setContent(const QString & content);

    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<ResourceRecognitionIndexItemData> d;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__RESOURCE_RECOGNITION_INDEX_ITEM_H
