#include "Notebook.h"
#include "SharedNotebookAdapter.h"
#include "UserAdapter.h"
#include "../Utility.h"
#include "../local_storage/Serialization.h"
#include <logging/QuteNoteLogger.h>
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

bool Notebook::hasName() const
{
    return m_enNotebook.__isset.name;
}

const QString Notebook::name() const
{
    return std::move(QString::fromStdString(m_enNotebook.name));
}

void Notebook::setName(const QString & name)
{
    m_enNotebook.name = name.toStdString();
    m_enNotebook.__isset.name = !name.isEmpty();
}

bool Notebook::isDefaultNotebook() const
{
    return (m_enNotebook.__isset.defaultNotebook && m_enNotebook.defaultNotebook);
}

void Notebook::setDefaultNotebookFlag(const bool defaultNotebook)
{
    m_enNotebook.defaultNotebook = defaultNotebook;
    m_enNotebook.__isset.defaultNotebook = true;
}

bool Notebook::hasCreationTimestamp() const
{
    return m_enNotebook.__isset.serviceCreated;
}

qint64 Notebook::creationTimestamp() const
{
    return m_enNotebook.serviceCreated;
}

void Notebook::setCreationTimestamp(const qint64 timestamp)
{
    m_enNotebook.serviceCreated = timestamp;
    m_enNotebook.__isset.serviceCreated = true;
}

bool Notebook::hasModificationTimestamp() const
{
    return m_enNotebook.__isset.serviceUpdated;
}

qint64 Notebook::modificationTimestamp() const
{
    return m_enNotebook.serviceUpdated;
}

void Notebook::setModificationTimestamp(const qint64 timestamp)
{
    m_enNotebook.serviceUpdated = timestamp;
    m_enNotebook.__isset.serviceUpdated = true;
}

bool Notebook::hasPublishingUri() const
{
    return m_enNotebook.__isset.publishing && m_enNotebook.publishing.__isset.uri;
}

const QString Notebook::publishingUri() const
{
    return std::move(QString::fromStdString(m_enNotebook.publishing.uri));
}

#define CHECK_AND_SET_PUBLISHING \
    if (m_enNotebook.publishing.__isset.uri || m_enNotebook.publishing.__isset.order || \
        m_enNotebook.publishing.__isset.ascending || m_enNotebook.publishing.__isset.publicDescription)  \
    { \
        m_enNotebook.__isset.publishing = true; \
    }

void Notebook::setPublishingUri(const QString & uri)
{
    m_enNotebook.publishing.uri = uri.toStdString();
    m_enNotebook.publishing.__isset.uri = !uri.isEmpty();
    CHECK_AND_SET_PUBLISHING;
}

bool Notebook::hasPublishingOrder() const
{
    return m_enNotebook.__isset.publishing && m_enNotebook.publishing.__isset.order;
}

qint8 Notebook::publishingOrder() const
{
    return static_cast<qint8>(m_enNotebook.publishing.order);
}

void Notebook::setPublishingOrder(const qint8 order)
{
    m_enNotebook.publishing.order = static_cast<evernote::edam::NoteSortOrder::type>(order);
    m_enNotebook.publishing.__isset.order = true;
    CHECK_AND_SET_PUBLISHING;
}

bool Notebook::hasPublishingAscending() const
{
    return m_enNotebook.__isset.publishing && m_enNotebook.publishing.__isset.ascending;
}

bool Notebook::isPublishingAscending() const
{
    return m_enNotebook.publishing.ascending;
}

void Notebook::setPublisingAscending(const bool ascending)
{
    m_enNotebook.publishing.ascending = ascending;
    m_enNotebook.publishing.__isset.ascending = true;
    CHECK_AND_SET_PUBLISHING;
}

bool Notebook::hasPublicDescription() const
{
    return m_enNotebook.__isset.publishing && m_enNotebook.publishing.__isset.publicDescription;
}

const QString Notebook::publicDescription() const
{
    return std::move(QString::fromStdString(m_enNotebook.publishing.publicDescription));
}

void Notebook::setPublicDescription(const QString & publicDescription)
{
    m_enNotebook.publishing.publicDescription = publicDescription.toStdString();
    m_enNotebook.publishing.__isset.publicDescription = !publicDescription.isEmpty();
    CHECK_AND_SET_PUBLISHING;
}

#undef CHECK_AND_SET_PUBLISHING

bool Notebook::isPublished() const
{
    return m_enNotebook.__isset.published && m_enNotebook.published;
}

void Notebook::setPublishedFlag(const bool published)
{
    m_enNotebook.published = published;
    m_enNotebook.__isset.published = true;
}

bool Notebook::hasStack() const
{
    return m_enNotebook.__isset.stack;
}

const QString Notebook::stack() const
{
    return std::move(QString::fromStdString(m_enNotebook.stack));
}

void Notebook::setStack(const QString & stack)
{
    if (m_enNotebook.__isset.restrictions)
    {
        if (m_enNotebook.restrictions.__isset.noSetNotebookStack &&
            m_enNotebook.restrictions.noSetDefaultNotebook)
        {
            QNDEBUG("Can't set stack for Notebook due to specified restrictions");
            return;
        }
    }

    m_enNotebook.stack = stack.toStdString();
    m_enNotebook.__isset.stack = !stack.isEmpty();
}

bool Notebook::hasSharedNotebooks()
{
    return m_enNotebook.__isset.sharedNotebooks;
}

void Notebook::sharedNotebooks(std::vector<SharedNotebookAdapter> & notebooks) const
{
    notebooks.clear();

    for(const auto & sharedNotebook: m_enNotebook.sharedNotebooks) {
        SharedNotebookAdapter sharedNotebookadapter(sharedNotebook);
        notebooks.push_back(sharedNotebookadapter);
    }
}

void Notebook::setSharedNotebooks(std::vector<ISharedNotebook> & notebooks)
{
    size_t numNotebooks = notebooks.size();

    auto & sharedNotebooks = m_enNotebook.sharedNotebooks;
    sharedNotebooks.resize(numNotebooks);
    for(size_t i = 0; i < numNotebooks; ++i) {
        sharedNotebooks[i] = notebooks[i].GetEnSharedNotebook();
    }
}

void Notebook::addSharedNotebook(const ISharedNotebook & sharedNotebook)
{
    auto & sharedNotebooks = m_enNotebook.sharedNotebooks;
    auto it = std::find(sharedNotebooks.cbegin(), sharedNotebooks.cend(), sharedNotebook.GetEnSharedNotebook());
    if (it != sharedNotebooks.cend()) {
        QNDEBUG("Can't add shared notebook: this shared notebook already exists within the notebook");
        return;
    }

    sharedNotebooks.push_back(sharedNotebook.GetEnSharedNotebook());
}

void Notebook::removeSharedNotebook(const ISharedNotebook & sharedNotebook)
{
    auto & sharedNotebooks = m_enNotebook.sharedNotebooks;
    auto it = std::find(sharedNotebooks.begin(), sharedNotebooks.end(), sharedNotebook.GetEnSharedNotebook());
    if (it == sharedNotebooks.end()) {
        QNDEBUG("Can't remove shared notebook: this shared notebook doesn't exists within the notebook");
        return;
    }

    sharedNotebooks.erase(it);
}

bool Notebook::hasBusinessNotebookDescription() const
{
    return m_enNotebook.__isset.businessNotebook && m_enNotebook.businessNotebook.__isset.notebookDescription;
}

const QString Notebook::businessNotebookDescription() const
{
    return std::move(QString::fromStdString(m_enNotebook.businessNotebook.notebookDescription));
}

#define CHECK_AND_SET_BUSINESS_NOTEBOOK \
    if (m_enNotebook.businessNotebook.__isset.notebookDescription || \
        m_enNotebook.businessNotebook.__isset.privilege || \
        m_enNotebook.businessNotebook.__isset.recommended) \
    { \
        m_enNotebook.__isset.businessNotebook = true; \
    }

void Notebook::setBusinessNotebookDescription(const QString & businessNotebookDescription)
{
    m_enNotebook.businessNotebook.notebookDescription = businessNotebookDescription.toStdString();
    m_enNotebook.businessNotebook.__isset.notebookDescription = !businessNotebookDescription.isEmpty();
    CHECK_AND_SET_BUSINESS_NOTEBOOK;
}

bool Notebook::hasBusinessNotebookPrivilegeLevel() const
{
    return m_enNotebook.__isset.businessNotebook && m_enNotebook.businessNotebook.__isset.privilege;
}

qint8 Notebook::businessNotebookPrivilegeLevel() const
{
    return static_cast<qint8>(m_enNotebook.businessNotebook.privilege);
}

void Notebook::setBusinessNotebookPrivilegeLevel(const qint8 privilegeLevel)
{
    m_enNotebook.businessNotebook.privilege = static_cast<evernote::edam::SharedNotebookPrivilegeLevel::type>(privilegeLevel);
    m_enNotebook.businessNotebook.__isset.privilege = true;
    CHECK_AND_SET_BUSINESS_NOTEBOOK;
}

bool Notebook::hasBusinessNotebookRecommended() const
{
    return m_enNotebook.__isset.businessNotebook && m_enNotebook.businessNotebook.__isset.recommended;
}

bool Notebook::isBusinessNotebookRecommended() const
{
    return m_enNotebook.businessNotebook.recommended;
}

void Notebook::setBusinessNotebookRecommendedFlag(const bool recommended)
{
    m_enNotebook.businessNotebook.recommended = recommended;
    m_enNotebook.businessNotebook.__isset.recommended = true;
    CHECK_AND_SET_BUSINESS_NOTEBOOK;
}

#undef CHECK_AND_SET_BUSINESS_NOTEBOOK

bool Notebook::hasContact() const
{
    return m_enNotebook.__isset.contact;
}

const UserAdapter Notebook::contact() const
{
    return std::move(UserAdapter(m_enNotebook.contact));
}

void Notebook::setContact(const IUser & contact)
{
    m_enNotebook.contact = contact.GetEnUser();
    m_enNotebook.__isset.contact = true;
}

bool Notebook::hasRestrictions() const
{
    return m_enNotebook.__isset.restrictions;
}

const QByteArray Notebook::restrictions() const
{
    // FIXME: implement serialization for NotebookRestrictions
    return QByteArray();
}

void Notebook::setRestrictions(const QByteArray &restrictions)
{
    // FIXME: implement
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
            strm << SharedNotebookAdapter(sharedNotebook);
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
        strm << UserAdapter(m_enNotebook.contact);
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

#undef INSERT_DELIMITER

    return strm;
}

} // namespace qute_note
