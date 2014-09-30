#include "NotebookData.h"
#include "../../Utility.h"

namespace qute_note {

NotebookData::NotebookData() :
    NoteStoreDataElementData(),
    m_qecNotebook(),
    m_isLocal(true),
    m_isLastUsed(false)
{}

NotebookData::NotebookData(const NotebookData & other) :
    NoteStoreDataElementData(other),
    m_qecNotebook(other.m_qecNotebook),
    m_isLocal(other.m_isLocal),
    m_isLastUsed(other.m_isLastUsed)
{}

NotebookData::NotebookData(NotebookData && other) :
    NoteStoreDataElementData(std::move(other)),
    m_qecNotebook(std::move(other.m_qecNotebook)),
    m_isLocal(std::move(other.m_isLocal)),
    m_isLastUsed(std::move(other.m_isLastUsed))
{}

NotebookData::NotebookData(const qevercloud::Notebook & other) :
    NoteStoreDataElementData(),
    m_qecNotebook(other),
    m_isLocal(true),
    m_isLastUsed(false)
{}

NotebookData::NotebookData(qevercloud::Notebook && other) :
    NoteStoreDataElementData(),
    m_qecNotebook(std::move(other)),
    m_isLocal(true),
    m_isLastUsed(false)
{}

NotebookData & NotebookData::operator=(const NotebookData & other)
{
    NoteStoreDataElementData::operator=(other);

    if (this != std::addressof(other)) {
        m_qecNotebook = other.m_qecNotebook;
        m_isLocal = other.m_isLocal;
        m_isLastUsed = other.m_isLastUsed;
    }

    return *this;
}

NotebookData & NotebookData::operator=(NotebookData && other)
{
    NoteStoreDataElementData::operator=(other);

    if (this != std::addressof(other)) {
        m_qecNotebook = std::move(other.m_qecNotebook);
        m_isLocal = std::move(other.m_isLocal);
        m_isLastUsed = std::move(other.m_isLastUsed);
    }

    return *this;
}

NotebookData & NotebookData::operator=(const qevercloud::Notebook & other)
{
    m_qecNotebook = other;
    return *this;
}

NotebookData & NotebookData::operator=(qevercloud::Notebook && other)
{
    m_qecNotebook = std::move(other);
    return *this;
}

NotebookData::~NotebookData()
{}

bool NotebookData::checkParameters(QString &errorDescription) const
{
    if (m_qecNotebook.guid.isSet() && !CheckGuid(m_qecNotebook.guid.ref())) {
        errorDescription = QT_TR_NOOP("Notebook's guid is invalid");
        return false;
    }

    if (m_qecNotebook.updateSequenceNum.isSet() && !CheckUpdateSequenceNumber(m_qecNotebook.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("Notebook's update sequence number is invalid");
        return false;
    }

    if (m_qecNotebook.name.isSet())
    {
        int nameSize = m_qecNotebook.name->size();
        if ( (nameSize < qevercloud::EDAM_NOTEBOOK_NAME_LEN_MIN) ||
             (nameSize > qevercloud::EDAM_NOTEBOOK_NAME_LEN_MAX) )
        {
            errorDescription = QT_TR_NOOP("Notebook's name has invalid size");
            return false;
        }
    }

    if (m_qecNotebook.sharedNotebooks.isSet())
    {
        foreach(const qevercloud::SharedNotebook & sharedNotebook, m_qecNotebook.sharedNotebooks.ref())
        {
            if (!sharedNotebook.id.isSet()) {
                errorDescription = QT_TR_NOOP("Notebook has shared notebook without share id set");
                return false;
            }

            if (sharedNotebook.notebookGuid.isSet() && !CheckGuid(sharedNotebook.notebookGuid.ref())) {
                errorDescription = QT_TR_NOOP("Notebook has shared notebook with invalid guid");
                return false;
            }
        }
    }

    if (m_qecNotebook.businessNotebook.isSet())
    {
        if (m_qecNotebook.businessNotebook->notebookDescription.isSet())
        {
            int businessNotebookDescriptionSize = m_qecNotebook.businessNotebook->notebookDescription->size();

            if ( (businessNotebookDescriptionSize < qevercloud::EDAM_BUSINESS_NOTEBOOK_DESCRIPTION_LEN_MIN) ||
                 (businessNotebookDescriptionSize > qevercloud::EDAM_BUSINESS_NOTEBOOK_DESCRIPTION_LEN_MAX) )
            {
                errorDescription = QT_TR_NOOP("Description for business notebook has invalid size");
                return false;
            }
        }
    }

    return true;
}

bool NotebookData::operator==(const NotebookData & other) const
{
    if (m_isLocal != other.m_isLocal) {
        return false;
    }
    else if (m_isLastUsed != other.m_isLastUsed) {
        return false;
    }
    else if (m_qecNotebook != other.m_qecNotebook) {
        return false;
    }

    return true;
}

bool NotebookData::operator!=(const NotebookData & other) const
{
    return !(*this == other);
}

} // namespace qute_note
