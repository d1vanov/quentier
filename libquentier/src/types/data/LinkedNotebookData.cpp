#include "LinkedNotebookData.h"
#include <quentier/utility/Utility.h>

namespace quentier {

LinkedNotebookData::LinkedNotebookData() :
    QSharedData(),
    m_qecLinkedNotebook(),
    m_isDirty(true)
{}

LinkedNotebookData::LinkedNotebookData(const LinkedNotebookData & other) :
    QSharedData(other),
    m_qecLinkedNotebook(other.m_qecLinkedNotebook),
    m_isDirty(other.m_isDirty)
{}

LinkedNotebookData::LinkedNotebookData(LinkedNotebookData && other) :
    QSharedData(std::move(other)),
    m_qecLinkedNotebook(std::move(other.m_qecLinkedNotebook)),
    m_isDirty(std::move(other.m_isDirty))
{}

LinkedNotebookData::LinkedNotebookData(const qevercloud::LinkedNotebook & other) :
    QSharedData(),
    m_qecLinkedNotebook(other),
    m_isDirty(true)
{}

LinkedNotebookData::LinkedNotebookData(qevercloud::LinkedNotebook && other) :
    QSharedData(),
    m_qecLinkedNotebook(std::move(other)),
    m_isDirty(true)
{}

LinkedNotebookData::~LinkedNotebookData()
{}

void LinkedNotebookData::clear()
{
    m_qecLinkedNotebook = qevercloud::LinkedNotebook();
}

bool LinkedNotebookData::checkParameters(QString & errorDescription) const
{
    if (!m_qecLinkedNotebook.guid.isSet()) {
        errorDescription = QT_TR_NOOP("Linked notebook's guid is not set");
        return false;
    }
    else if (!checkGuid(m_qecLinkedNotebook.guid.ref())) {
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
            QLatin1Char spaceChar(' ');
            const QString & name = m_qecLinkedNotebook.shareName.ref();
            const int size = name.size();

            bool nonSpaceCharFound = false;
            for(int i = 0; i < size; ++i)
            {
                if (name[i] != spaceChar) {
                    nonSpaceCharFound = true;
                    break;
                }
            }

            if (!nonSpaceCharFound) {
                errorDescription = QT_TR_NOOP("Linked notebook's custom name must contain non-space characters");
                return false;
            }
        }
    }

    return true;
}

bool LinkedNotebookData::operator==(const LinkedNotebookData & other) const
{
    return (m_qecLinkedNotebook == other.m_qecLinkedNotebook) && (m_isDirty == other.m_isDirty);
}

bool LinkedNotebookData::operator!=(const LinkedNotebookData & other) const
{
    return !(*this == other);
}

} // namespace quentier
