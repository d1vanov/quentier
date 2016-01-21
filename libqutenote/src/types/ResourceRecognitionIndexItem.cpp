#include <qute_note/types/ResourceRecognitionIndexItem.h>
#include "data/ResourceRecognitionIndexItemData.h"

namespace qute_note {

ResourceRecognitionIndexItem::ResourceRecognitionIndexItem() :
    d(new ResourceRecognitionIndexItemData)
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

void ResourceRecognitionIndexItem::setStrokeList(const QVector<int> & strokeList)
{
    d->m_strokeList = strokeList;
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

    d->m_strokeList.removeAt(index);
    return true;
}

bool ResourceRecognitionIndexItem::removeStrokeAt(const int strokeIndex)
{
    const QVector<int> & strokeList = d.constData()->m_strokeList;
    if (strokeList.size() <= strokeIndex) {
        return false;
    }

    d->m_strokeList.removeAt(strokeIndex);
    return true;
}

QString ResourceRecognitionIndexItem::type() const
{
    return d->m_type;
}

void ResourceRecognitionIndexItem::setType(const QString & type)
{
    d->m_type = type;
}

QString ResourceRecognitionIndexItem::objectType() const
{
    return d->m_objectType;
}

void ResourceRecognitionIndexItem::setObjectType(const QString & objectType)
{
    d->m_objectType = objectType;
}

QString ResourceRecognitionIndexItem::shapeType() const
{
    return d->m_shapeType;
}

void ResourceRecognitionIndexItem::setShapeType(const QString & shapeType)
{
    d->m_shapeType = shapeType;
}

QString ResourceRecognitionIndexItem::content() const
{
    return d->m_content;
}

void ResourceRecognitionIndexItem::setContent(const QString & content)
{
    d->m_content = content;
}

} // namespace qute_note
