#include "ResourceRecognitionIndicesData.h"
#include <qute_note/logging/QuteNoteLogger.h>

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

    if (m_recoType.isEmpty())
    {
        QNTRACE("Resource recognition indices' recognition type is not set");
        return false;
    }
    else if ((m_recoType != "service") && (m_recoType != "client"))
    {
        QNTRACE("Resource recognition indices' recognition type is not valid");
    }

    // TODO: continue from here

    return true;
}

} // namespace qute_note
