#include "NotebookData.h"
#include <qute_note/utility/Utility.h>

namespace qute_note {

NotebookData::NotebookData() :
    DataElementWithShortcutData(),
    m_qecNotebook(),
    m_isLastUsed(false),
    m_linkedNotebookGuid()
{}

NotebookData::NotebookData(const NotebookData & other) :
    DataElementWithShortcutData(other),
    m_qecNotebook(other.m_qecNotebook),
    m_isLastUsed(other.m_isLastUsed),
    m_linkedNotebookGuid(other.m_linkedNotebookGuid)
{}

NotebookData::NotebookData(NotebookData && other) :
    DataElementWithShortcutData(std::move(other)),
    m_qecNotebook(std::move(other.m_qecNotebook)),
    m_isLastUsed(std::move(other.m_isLastUsed)),
    m_linkedNotebookGuid(std::move(other.m_linkedNotebookGuid))
{}

NotebookData::NotebookData(const qevercloud::Notebook & other) :
    DataElementWithShortcutData(),
    m_qecNotebook(other),
    m_isLastUsed(false),
    m_linkedNotebookGuid()
{}

NotebookData::NotebookData(qevercloud::Notebook && other) :
    DataElementWithShortcutData(),
    m_qecNotebook(std::move(other)),
    m_isLastUsed(false),
    m_linkedNotebookGuid()
{}

NotebookData::~NotebookData()
{}

bool NotebookData::checkParameters(QString & errorDescription) const
{
    if (m_qecNotebook.guid.isSet() && !CheckGuid(m_qecNotebook.guid.ref())) {
        errorDescription = QT_TR_NOOP("Notebook's guid is invalid: ") + m_qecNotebook.guid;
        return false;
    }

    if (m_linkedNotebookGuid.isSet() && !CheckGuid(m_linkedNotebookGuid.ref())) {
        errorDescription = QT_TR_NOOP("Notebook's linked notebook guid is invalid: ") + m_linkedNotebookGuid;
        return false;
    }

    if (m_qecNotebook.updateSequenceNum.isSet() && !CheckUpdateSequenceNumber(m_qecNotebook.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("Notebook's update sequence number is invalid: ") +
                           QString::number(m_qecNotebook.updateSequenceNum);
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
    else if (m_isDirty != other.m_isDirty) {
        return false;
    }
    else if (m_hasShortcut != other.m_hasShortcut) {
        return false;
    }
    else if (m_isLastUsed != other.m_isLastUsed) {
        return false;
    }
    else if (m_qecNotebook != other.m_qecNotebook) {
        return false;
    }
    else if (!m_linkedNotebookGuid.isEqual(other.m_linkedNotebookGuid)) {
        return false;
    }

    return true;
}

bool NotebookData::operator!=(const NotebookData & other) const
{
    return !(*this == other);
}

} // namespace qute_note
