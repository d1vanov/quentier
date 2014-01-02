#include "Notebook_p.h"

namespace qute_note {

NotebookPrivate & NotebookPrivate::operator=(const NotebookPrivate & other)
{
    if (this != &other)
    {
        m_name = other.m_name;
        m_isDefault = other.m_isDefault;
        m_creationTimestamp = other.m_creationTimestamp;
        m_modificationTimestamp = other.m_modificationTimestamp;
        m_noteSortOrder = other.m_noteSortOrder;
        m_publicNoteSortOrder = other.m_publicNoteSortOrder;
        m_publicDescription = other.m_publicDescription;
        m_isPublished = other.m_isPublished;
        m_containerName = other.m_containerName;
        m_isLastUsed = other.m_isLastUsed;
    }

    return *this;
}

NotebookPrivate & NotebookPrivate::operator=(NotebookPrivate && other)
{
    if (this != &other)
    {
        m_name = other.m_name;
        m_isDefault = other.m_isDefault;
        m_creationTimestamp = other.m_creationTimestamp;
        m_modificationTimestamp = other.m_modificationTimestamp;
        m_noteSortOrder = other.m_noteSortOrder;
        m_publicNoteSortOrder = other.m_publicNoteSortOrder;
        m_publicDescription = other.m_publicDescription;
        m_isPublished = other.m_isPublished;
        m_containerName = other.m_containerName;
        m_isLastUsed = other.m_isLastUsed;
    }

    return *this;
}

}
