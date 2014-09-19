#include "LinkedNotebookData.h"
#include "../../Utility.h"

namespace qute_note {

LinkedNotebookData::LinkedNotebookData() :
    m_qecLinkedNotebook()
{}

LinkedNotebookData::LinkedNotebookData(const LinkedNotebookData & other) :
    m_qecLinkedNotebook(other.m_qecLinkedNotebook)
{}

LinkedNotebookData::LinkedNotebookData(LinkedNotebookData && other) :
    m_qecLinkedNotebook(std::move(other.m_qecLinkedNotebook))
{}

LinkedNotebookData::LinkedNotebookData(const qevercloud::LinkedNotebook & other) :
    m_qecLinkedNotebook(other)
{}

LinkedNotebookData::LinkedNotebookData(qevercloud::LinkedNotebook && other) :
    m_qecLinkedNotebook(std::move(other))
{}

LinkedNotebookData & LinkedNotebookData::operator=(const LinkedNotebookData & other)
{
    if (this != std::addressof(other)) {
        m_qecLinkedNotebook = other.m_qecLinkedNotebook;
    }

    return *this;
}

LinkedNotebookData & LinkedNotebookData::operator=(LinkedNotebookData && other)
{
    if (this != std::addressof(other)) {
        m_qecLinkedNotebook = std::move(other.m_qecLinkedNotebook);
    }

    return *this;
}

LinkedNotebookData & LinkedNotebookData::operator=(const qevercloud::LinkedNotebook & other)
{
    m_qecLinkedNotebook = other;
    return *this;
}

LinkedNotebookData::~LinkedNotebookData()
{}

void LinkedNotebookData::clear()
{
    m_qecLinkedNotebook = qevercloud::LinkedNotebook();
}

bool LinkedNotebookData::checkParameters(QString &errorDescription) const
{
    if (!m_qecLinkedNotebook.guid.isSet()) {
        errorDescription = QT_TR_NOOP("Linked notebook's guid is not set");
        return false;
    }
    else if (!CheckGuid(m_qecLinkedNotebook.guid.ref())) {
        errorDescription = QT_TR_NOOP("Linked notebook's guid is invalid");
        return false;
    }

    if (m_qecLinkedNotebook.shareName.isSet())
    {
        if (m_qecLinkedNotebook.shareName->isEmpty())
        {
            errorDescription = QT_TR_NOOP("Linked notebook's custom name is empty");
            return false;
        }
        else
        {
            QString simplifiedShareName = QString(m_qecLinkedNotebook.shareName.ref()).replace(" ", "");
            if (simplifiedShareName.isEmpty()) {
                errorDescription = QT_TR_NOOP("Linked notebook's custom name must contain non-space characters");
                return false;
            }
        }
    }

    return true;
}

bool LinkedNotebookData::operator==(const LinkedNotebookData & other) const
{
    return (m_qecLinkedNotebook == other.m_qecLinkedNotebook);
}

bool LinkedNotebookData::operator!=(const LinkedNotebookData & other) const
{
    return !(*this == other);
}

} // namespace qute_note
