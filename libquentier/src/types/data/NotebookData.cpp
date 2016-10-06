/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NotebookData.h"
#include <quentier/types/Notebook.h>
#include <quentier/utility/Utility.h>

namespace quentier {

NotebookData::NotebookData() :
    FavoritableDataElementData(),
    m_qecNotebook(),
    m_isLastUsed(false),
    m_linkedNotebookGuid()
{}

NotebookData::NotebookData(const NotebookData & other) :
    FavoritableDataElementData(other),
    m_qecNotebook(other.m_qecNotebook),
    m_isLastUsed(other.m_isLastUsed),
    m_linkedNotebookGuid(other.m_linkedNotebookGuid)
{}

NotebookData::NotebookData(NotebookData && other) :
    FavoritableDataElementData(std::move(other)),
    m_qecNotebook(std::move(other.m_qecNotebook)),
    m_isLastUsed(std::move(other.m_isLastUsed)),
    m_linkedNotebookGuid(std::move(other.m_linkedNotebookGuid))
{}

NotebookData::NotebookData(const qevercloud::Notebook & other) :
    FavoritableDataElementData(),
    m_qecNotebook(other),
    m_isLastUsed(false),
    m_linkedNotebookGuid()
{}

NotebookData::NotebookData(qevercloud::Notebook && other) :
    FavoritableDataElementData(),
    m_qecNotebook(std::move(other)),
    m_isLastUsed(false),
    m_linkedNotebookGuid()
{}

NotebookData::~NotebookData()
{}

bool NotebookData::checkParameters(QNLocalizedString & errorDescription) const
{
    if (m_qecNotebook.guid.isSet() && !checkGuid(m_qecNotebook.guid.ref())) {
        errorDescription = QT_TR_NOOP("notebook's guid is invalid");
        errorDescription += QStringLiteral(": ");
        errorDescription += m_qecNotebook.guid;
        return false;
    }

    if (m_linkedNotebookGuid.isSet() && !checkGuid(m_linkedNotebookGuid.ref())) {
        errorDescription = QT_TR_NOOP("notebook's linked notebook guid is invalid");
        errorDescription += QStringLiteral(": ");
        errorDescription += m_linkedNotebookGuid;
        return false;
    }

    if (m_qecNotebook.updateSequenceNum.isSet() && !checkUpdateSequenceNumber(m_qecNotebook.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("notebook's update sequence number is invalid");
        errorDescription += QStringLiteral(": ");
        errorDescription += QString::number(m_qecNotebook.updateSequenceNum);
        return false;
    }

    if (m_qecNotebook.name.isSet() && !Notebook::validateName(m_qecNotebook.name, &errorDescription)) {
        return false;
    }

    if (m_qecNotebook.sharedNotebooks.isSet())
    {
        foreach(const qevercloud::SharedNotebook & sharedNotebook, m_qecNotebook.sharedNotebooks.ref())
        {
            if (!sharedNotebook.id.isSet()) {
                errorDescription = QT_TR_NOOP("notebook has shared notebook without share id set");
                return false;
            }

            if (sharedNotebook.notebookGuid.isSet() && !checkGuid(sharedNotebook.notebookGuid.ref())) {
                errorDescription = QT_TR_NOOP("notebook has shared notebook with invalid guid");
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
                errorDescription = QT_TR_NOOP("description for business notebook has invalid size");
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
    else if (m_isFavorited != other.m_isFavorited) {
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

} // namespace quentier
