#include "Notebook.h"
#include "SharedNotebookAdapter.h"
#include "UserAdapter.h"
#include "../Utility.h"
#include <Limits_constants.h>
#include <QDateTime>

namespace qute_note {

Notebook::Notebook() :
    NoteStoreDataElement()
{}

Notebook::Notebook(const Notebook &other) :
    NoteStoreDataElement(other),
    m_enNotebook(other.m_enNotebook)
{}

Notebook & Notebook::operator=(const Notebook & other)
{
    if (this != &other) {
        NoteStoreDataElement::operator=(other);
        m_enNotebook = other.m_enNotebook;
    }

    return *this;
}

Notebook::~Notebook()
{}

void Notebook::clear()
{
    m_enNotebook = evernote::edam::Notebook();
}

bool Notebook::hasGuid() const
{
    return m_enNotebook.__isset.guid;
}

const QString Notebook::guid() const
{
    return std::move(QString::fromStdString(m_enNotebook.guid));
}

void Notebook::setGuid(const QString & guid)
{
    m_enNotebook.guid = guid.toStdString();
    m_enNotebook.__isset.guid = !guid.isEmpty();
}

bool Notebook::hasUpdateSequenceNumber() const
{
    return m_enNotebook.__isset.updateSequenceNum;
}

qint32 Notebook::updateSequenceNumber() const
{
    return static_cast<qint32>(m_enNotebook.updateSequenceNum);
}

void Notebook::setUpdateSequenceNumber(const qint32 usn)
{
    m_enNotebook.updateSequenceNum = static_cast<int32_t>(usn);
    m_enNotebook.__isset.updateSequenceNum = true;
}

bool Notebook::checkParameters(QString & errorDescription) const
{
    if (!m_enNotebook.__isset.guid) {
        errorDescription = QObject::tr("Notebook's guid is not set");
        return false;
    }
    else if (!CheckGuid(m_enNotebook.guid)) {
        errorDescription = QObject::tr("Notebook's guid is invalid");
        return false;
    }

    if (!m_enNotebook.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Notebook's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(m_enNotebook.updateSequenceNum)) {
        errorDescription = QObject::tr("Notebook's update sequence number is invalid");
        return false;
    }

    if (!m_enNotebook.__isset.name) {
        errorDescription = QObject::tr("Notebook's name is not set");
        return false;
    }
    else {
        size_t nameSize = m_enNotebook.name.size();

        if ( (nameSize < evernote::limits::g_Limits_constants.EDAM_NOTEBOOK_NAME_LEN_MIN) ||
             (nameSize > evernote::limits::g_Limits_constants.EDAM_NOTEBOOK_NAME_LEN_MAX) )
        {
            errorDescription = QObject::tr("Notebook's name has invalid size");
            return false;
        }
    }

    if (!m_enNotebook.__isset.serviceCreated) {
        errorDescription = QObject::tr("Notebook's creation timestamp is not set");
        return false;
    }

    if (!m_enNotebook.__isset.serviceUpdated) {
        errorDescription = QObject::tr("Notebook's modification timestamp is not set");
        return false;
    }

    if (m_enNotebook.__isset.sharedNotebooks)
    {
        for(const auto & sharedNotebook: m_enNotebook.sharedNotebooks)
        {
            if (!sharedNotebook.__isset.id) {
                errorDescription = QObject::tr("Notebook has shared notebook without share id set");
                return false;
            }

            if (!sharedNotebook.__isset.notebookGuid) {
                errorDescription = QObject::tr("Notebook has shared notebook without real notebook's guid set");
                return false;
            }
            else if (!CheckGuid(sharedNotebook.notebookGuid)) {
                errorDescription = QObject::tr("Notebook has shared notebook with invalid guid");
                return false;
            }
        }
    }

    if (m_enNotebook.__isset.businessNotebook)
    {
        if (!m_enNotebook.businessNotebook.__isset.notebookDescription) {
            errorDescription = QObject::tr("Description for business notebook is not set");
            return false;
        }
        else {
            size_t businessNotebookDescriptionSize = m_enNotebook.businessNotebook.notebookDescription.size();

            if ( (businessNotebookDescriptionSize < evernote::limits::g_Limits_constants.EDAM_BUSINESS_NOTEBOOK_DESCRIPTION_LEN_MIN) ||
                 (businessNotebookDescriptionSize > evernote::limits::g_Limits_constants.EDAM_BUSINESS_NOTEBOOK_DESCRIPTION_LEN_MAX) )
            {
                errorDescription = QObject::tr("Description for business notebook has invalid size");
                return false;
            }
        }
    }

    return true;
}

QTextStream & Notebook::Print(QTextStream & strm) const
{
    strm << "Notebook { \n";

    const auto & isSet = m_enNotebook.__isset;

#define INSERT_DELIMITER \
    strm << "; \n"

    if (isSet.guid) {
        strm << "guid: " << QString::fromStdString(m_enNotebook.guid);
    }
    else {
        strm << "guid is not set";
    }
    INSERT_DELIMITER;

    if (isSet.name) {
        strm << "name: " << QString::fromStdString(m_enNotebook.name);
    }
    else {
        strm << "name is not set";
    }
    INSERT_DELIMITER;

    if (isSet.updateSequenceNum) {
        strm << "updateSequenceNumber: " << m_enNotebook.updateSequenceNum;
    }
    else {
        strm << "updateSequenceNumber is not set";
    }
    INSERT_DELIMITER;

    if (isSet.defaultNotebook) {
        strm << "defaultNotebook: " << (m_enNotebook.defaultNotebook ? "true" : "false");
    }
    else {
        strm << "defaultNotebook is not set";
    }
    INSERT_DELIMITER;

    if (isSet.serviceCreated) {
        QDateTime createdDatetime;
        createdDatetime.setTime_t(m_enNotebook.serviceCreated);
        strm << "creationTimestamp: " << m_enNotebook.serviceCreated << ", datetime: "
             << createdDatetime.toString(Qt::ISODate);
    }
    else {
        strm << "creationTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (isSet.serviceUpdated) {
        QDateTime updatedDatetime;
        updatedDatetime.setTime_t(m_enNotebook.serviceUpdated);
        strm << "modificationTimestamp: " << m_enNotebook.serviceUpdated << ", datetime: "
             << updatedDatetime.toString(Qt::ISODate);
    }
    else {
        strm << "modificationTimestamp is not set";
    }

    if (isSet.publishing)
    {
        const auto & publishing = m_enNotebook.publishing;
        const auto & isSetPublishing = publishing.__isset;

        if (isSetPublishing.uri) {
            strm << "publishingUri: " << QString::fromStdString(publishing.uri);
        }
        else {
            strm << "publishingUri is not set";
        }
        INSERT_DELIMITER;

        if (isSetPublishing.order) {
            strm << "publishingOrder: " << publishing.order;
        }
        else {
            strm << "publishingOrder is not set";
        }
        INSERT_DELIMITER;

        if (isSetPublishing.ascending) {
            strm << "publishingAscending: " << (publishing.ascending ? "true" : "false");
        }
        else {
            strm << "publishingAscending is not set";
        }
        INSERT_DELIMITER;

        if (isSetPublishing.publicDescription) {
            strm << "publishingPublicDescription: " << QString::fromStdString(publishing.publicDescription);
        }
        else {
            strm << "publishingPublicDescription is not set";
        }
        INSERT_DELIMITER;
    }
    else {
        strm << "publishing is not set";
        INSERT_DELIMITER;
    }

    if (isSet.published) {
        strm << "published: " << (m_enNotebook.published ? "true" : "false");
    }
    else {
        strm << "published is not set";
    }
    INSERT_DELIMITER;

    if (isSet.stack) {
        strm << "stack: " << QString::fromStdString(m_enNotebook.stack);
    }
    else {
        strm << "stack is not set";
    }
    INSERT_DELIMITER;

    if (isSet.sharedNotebooks) {
        strm << "sharedNotebooks: \n";
        for(const auto & sharedNotebook: m_enNotebook.sharedNotebooks) {
            strm << SharedNotebookAdapter(sharedNotebook).ToQString();
        }
    }
    else {
        strm << "sharedNotebooks are not set";
    }
    INSERT_DELIMITER;

    if (isSet.businessNotebook)
    {
        const auto & businessNotebook = m_enNotebook.businessNotebook;
        const auto & isSetBusinessNotebook = businessNotebook.__isset;

        if (isSetBusinessNotebook.notebookDescription) {
            strm << "businessNotebookDescription: " << QString::fromStdString(businessNotebook.notebookDescription);
        }
        else {
            strm << "businessNotebookDescription is not set";
        }
        INSERT_DELIMITER;

        if (isSetBusinessNotebook.privilege) {
            strm << "businessNotebookPrivilegeLevel: " << businessNotebook.privilege;
        }
        else {
            strm << "businessNotebookPrivilegeLevel is not set";
        }
        INSERT_DELIMITER;

        if (isSetBusinessNotebook.recommended) {
            strm << "businessNotebookRecommended = " << (businessNotebook.recommended ? "true" : "false");
        }
        else {
            strm << "businessNotebookRecommended is not set";
        }
        INSERT_DELIMITER;
    }
    else {
        strm << "businessNotebook is not set";
        INSERT_DELIMITER;
    }

    if (isSet.contact) {
        strm << "contact: \n";
        strm << UserAdapter(m_enNotebook.contact).ToQString();
    }
    else {
        strm << "contact is not set";
    }
    INSERT_DELIMITER;

    if (isSet.restrictions) {
        strm << "restrictions: " << m_enNotebook.restrictions;
    }
    else {
        strm << "restrictions are not set";
    }
    INSERT_DELIMITER;

    return strm;
}

} // namespace qute_note
