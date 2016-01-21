#include "ResourceRecognitionIndexItemData.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

ResourceRecognitionIndexItemData::ResourceRecognitionIndexItemData() :
    m_x(-1),
    m_y(-1),
    m_h(-1),
    m_w(-1),
    m_offset(-1),
    m_duration(-1),
    m_strokeList(),
    m_type(),
    m_objectType(),
    m_shapeType(),
    m_content()
{}

bool ResourceRecognitionIndexItemData::isValid() const
{
    if (m_type.isEmpty()) {
        QNTRACE("Resource recognition index item's type is empty");
        return false;
    }
    else if (m_type == "t") {
        return true;
    }
    else if ((m_type == "object") && m_objectType.isEmpty()) {
        QNTRACE("Resource recognition index object item doesn't have the object type set");
        return false;
    }
    else if ((m_type == "shape") && m_shapeType.isEmpty()) {
        QNTRACE("Resoource recognition index shape item doesn't have the shape type set");
        return false;
    }

    return true;
}

} // namespace qute_note
