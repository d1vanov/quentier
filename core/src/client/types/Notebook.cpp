#include "Notebook.h"
#include "SharedNotebookAdapter.h"
#include "UserAdapter.h"
#include "QEverCloudOptionalQString.hpp"
#include "../Utility.h"
#include "../Serialization.h"
#include <logging/QuteNoteLogger.h>

namespace qute_note {

Notebook::Notebook() :
    NoteStoreDataElement(),
    m_qecNotebook(),
    m_isLocal(true),
    m_isLastUsed(false)
{}

Notebook::Notebook(const qevercloud::Notebook & other) :
    NoteStoreDataElement(),
    m_qecNotebook(other),
    m_isLocal(true),
    m_isLastUsed(false)
{}

Notebook::Notebook(qevercloud::Notebook && other) :
    NoteStoreDataElement(),
    m_qecNotebook(std::move(other)),
    m_isLocal(true),
    m_isLastUsed(false)
{}

Notebook & Notebook::operator=(const qevercloud::Notebook & other)
{
    m_qecNotebook = other;
    return *this;
}

Notebook & Notebook::operator=(qevercloud::Notebook && other)
{
    m_qecNotebook = std::move(other);
    return *this;
}

Notebook::~Notebook()
{}

bool Notebook::operator==(const Notebook & other) const
{
    if (isDirty() != other.isDirty()) {
        return false;
    }
    else if (m_isLocal != other.m_isLocal) {
        return false;
    }
    else if (m_isLastUsed != other.m_isLastUsed) {
        return false;
    }

    const qevercloud::Notebook & otherNotebook = other.m_qecNotebook;

    if (m_qecNotebook.guid != otherNotebook.guid) {
        return false;
    }
    else if (m_qecNotebook.name != otherNotebook.name) {
        return false;
    }
    else if (m_qecNotebook.updateSequenceNum != otherNotebook.updateSequenceNum) {
        return false;
    }
    else if (m_qecNotebook.defaultNotebook != otherNotebook.defaultNotebook) {
        return false;
    }
    else if (m_qecNotebook.serviceCreated != otherNotebook.serviceCreated) {
        return false;
    }
    else if (m_qecNotebook.serviceUpdated != otherNotebook.serviceUpdated) {
        return false;
    }
    else if (m_qecNotebook.published != otherNotebook.published) {
        return false;
    }
    else if (m_qecNotebook.stack != otherNotebook.stack) {
        return false;
    }
    else if (m_qecNotebook.sharedNotebooks != otherNotebook.sharedNotebooks) {
        return false;
    }
    else if (m_qecNotebook.publishing.isSet() != otherNotebook.publishing.isSet()) {
        return false;
    }
    else if (m_qecNotebook.businessNotebook.isSet() != otherNotebook.businessNotebook.isSet()) {
        return false;
    }
    else if (m_qecNotebook.contact.isSet() != otherNotebook.contact.isSet()) {
        return false;
    }
    else if (m_qecNotebook.restrictions.isSet() != otherNotebook.restrictions.isSet()) {
        return false;
    }

    if (m_qecNotebook.publishing.isSet())
    {
        const qevercloud::Publishing & publishing = m_qecNotebook.publishing;
        const qevercloud::Publishing & otherPublishing = otherNotebook.publishing;

        if (publishing.uri != otherPublishing.uri) {
            return false;
        }
        else if (publishing.order != otherPublishing.order) {
            return false;
        }
        else if (publishing.ascending != otherPublishing.ascending) {
            return false;
        }
        else if (publishing.publicDescription != otherPublishing.publicDescription) {
            return false;
        }
    }

    if (m_qecNotebook.businessNotebook.isSet())
    {
        const qevercloud::BusinessNotebook & businessNotebook = m_qecNotebook.businessNotebook;
        const qevercloud::BusinessNotebook & otherBusinessNotebook = otherNotebook.businessNotebook;

        if (businessNotebook.notebookDescription != otherBusinessNotebook.notebookDescription) {
            return false;
        }
        else if (businessNotebook.privilege != otherBusinessNotebook.privilege) {
            return false;
        }
        else if (businessNotebook.recommended != otherBusinessNotebook.recommended) {
            return false;
        }
    }

    if (m_qecNotebook.contact.isSet())
    {
        UserAdapter contact(m_qecNotebook.contact);
        UserAdapter otherContact(otherNotebook.contact);

        if (contact != otherContact) {
            return false;
        }
    }

    if (m_qecNotebook.restrictions.isSet())
    {
        const qevercloud::NotebookRestrictions & restrictions = m_qecNotebook.restrictions;
        const qevercloud::NotebookRestrictions & otherRestrictions = otherNotebook.restrictions;

        if (restrictions.noReadNotes != otherRestrictions.noReadNotes) {
            return false;
        }
        else if (restrictions.noCreateNotes != otherRestrictions.noCreateNotes) {
            return false;
        }
        else if (restrictions.noUpdateNotes != otherRestrictions.noUpdateNotes) {
            return false;
        }
        else if (restrictions.noExpungeNotes != otherRestrictions.noExpungeNotes) {
            return false;
        }
        else if (restrictions.noShareNotes != otherRestrictions.noShareNotes) {
            return false;
        }
        else if (restrictions.noEmailNotes != otherRestrictions.noEmailNotes) {
            return false;
        }
        else if (restrictions.noSendMessageToRecipients != otherRestrictions.noSendMessageToRecipients) {
            return false;
        }
        else if (restrictions.noUpdateNotebook != otherRestrictions.noUpdateNotebook) {
            return false;
        }
        else if (restrictions.noExpungeNotebook != otherRestrictions.noExpungeNotebook) {
            return false;
        }
        else if (restrictions.noSetDefaultNotebook != otherRestrictions.noSetDefaultNotebook) {
            return false;
        }
        else if (restrictions.noSetNotebookStack != otherRestrictions.noSetNotebookStack) {
            return false;
        }
        else if (restrictions.noPublishToPublic != otherRestrictions.noPublishToPublic) {
            return false;
        }
        else if (restrictions.noPublishToBusinessLibrary != otherRestrictions.noPublishToBusinessLibrary) {
            return false;
        }
        else if (restrictions.noCreateTags != otherRestrictions.noCreateTags) {
            return false;
        }
        else if (restrictions.noUpdateTags != otherRestrictions.noUpdateTags) {
            return false;
        }
        else if (restrictions.noExpungeTags != otherRestrictions.noExpungeTags) {
            return false;
        }
        else if (restrictions.noSetParentTag != otherRestrictions.noSetParentTag) {
            return false;
        }
        else if (restrictions.noCreateSharedNotebooks != otherRestrictions.noCreateSharedNotebooks) {
            return false;
        }
        else if (restrictions.updateWhichSharedNotebookRestrictions != otherRestrictions.updateWhichSharedNotebookRestrictions) {
            return false;
        }
        else if (restrictions.expungeWhichSharedNotebookRestrictions != otherRestrictions.expungeWhichSharedNotebookRestrictions) {
            return false;
        }
    }

    return true;
}

bool Notebook::operator!=(const Notebook & other) const
{
    return !(*this == other);
}

void Notebook::clear()
{
    m_qecNotebook = qevercloud::Notebook();
}

bool Notebook::hasGuid() const
{
    return m_qecNotebook.guid.isSet();
}

const QString Notebook::guid() const
{
    return m_qecNotebook.guid;
}

void Notebook::setGuid(const QString & guid)
{
    m_qecNotebook.guid = guid;
}

bool Notebook::hasUpdateSequenceNumber() const
{
    return m_qecNotebook.updateSequenceNum.isSet();
}

qint32 Notebook::updateSequenceNumber() const
{
    return m_qecNotebook.updateSequenceNum;
}

void Notebook::setUpdateSequenceNumber(const qint32 usn)
{
    m_qecNotebook.updateSequenceNum = usn;
}

bool Notebook::checkParameters(QString & errorDescription) const
{
    if (!m_qecNotebook.guid.isSet()) {
        errorDescription = QObject::tr("Notebook's guid is not set");
        return false;
    }
    else if (!CheckGuid(m_qecNotebook.guid.ref())) {
        errorDescription = QObject::tr("Notebook's guid is invalid");
        return false;
    }

    if (!m_qecNotebook.updateSequenceNum.isSet()) {
        errorDescription = QObject::tr("Notebook's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(m_qecNotebook.updateSequenceNum)) {
        errorDescription = QObject::tr("Notebook's update sequence number is invalid");
        return false;
    }

    if (!m_qecNotebook.name.isSet()) {
        errorDescription = QObject::tr("Notebook's name is not set");
        return false;
    }

    size_t nameSize = m_qecNotebook.name->size();

    if ( (nameSize < qevercloud::EDAM_NOTEBOOK_NAME_LEN_MIN) ||
         (nameSize > qevercloud::EDAM_NOTEBOOK_NAME_LEN_MAX) )
    {
        errorDescription = QObject::tr("Notebook's name has invalid size");
        return false;
    }

    if (!m_qecNotebook.serviceCreated.isSet()) {
        errorDescription = QObject::tr("Notebook's creation timestamp is not set");
        return false;
    }

    if (!m_qecNotebook.serviceUpdated.isSet()) {
        errorDescription = QObject::tr("Notebook's modification timestamp is not set");
        return false;
    }

    if (m_qecNotebook.sharedNotebooks.isSet())
    {
        foreach(const qevercloud::SharedNotebook & sharedNotebook, m_qecNotebook.sharedNotebooks.ref())
        {
            if (!sharedNotebook.id.isSet()) {
                errorDescription = QObject::tr("Notebook has shared notebook without share id set");
                return false;
            }

            if (!sharedNotebook.notebookGuid.isSet()) {
                errorDescription = QObject::tr("Notebook has shared notebook without real notebook's guid set");
                return false;
            }
            else if (!CheckGuid(sharedNotebook.notebookGuid.ref())) {
                errorDescription = QObject::tr("Notebook has shared notebook with invalid guid");
                return false;
            }
        }
    }

    if (m_qecNotebook.businessNotebook.isSet())
    {
        if (!m_qecNotebook.businessNotebook->notebookDescription.isSet()) {
            errorDescription = QObject::tr("Description for business notebook is not set");
            return false;
        }

        size_t businessNotebookDescriptionSize = m_qecNotebook.businessNotebook->notebookDescription->size();

        if ( (businessNotebookDescriptionSize < qevercloud::EDAM_BUSINESS_NOTEBOOK_DESCRIPTION_LEN_MIN) ||
             (businessNotebookDescriptionSize > qevercloud::EDAM_BUSINESS_NOTEBOOK_DESCRIPTION_LEN_MAX) )
        {
            errorDescription = QObject::tr("Description for business notebook has invalid size");
            return false;
        }
    }

    return true;
}

bool Notebook::hasName() const
{
    return m_qecNotebook.name.isSet();
}

const QString & Notebook::name() const
{
    return m_qecNotebook.name;
}

void Notebook::setName(const QString & name)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set name for notebook: updating is forbidden");
        return;
    }

    m_qecNotebook.name = name;
}

bool Notebook::isDefaultNotebook() const
{
    return m_qecNotebook.defaultNotebook.isSet() && m_qecNotebook.defaultNotebook;
}

void Notebook::setDefaultNotebook(const bool defaultNotebook)
{
    if (!canSetDefaultNotebook()) {
        QNDEBUG("Can't set defaut notebook flag: setting default notebook is forbidden");
        return;
    }

    m_qecNotebook.defaultNotebook = defaultNotebook;
}

bool Notebook::hasCreationTimestamp() const
{
    return m_qecNotebook.serviceCreated.isSet();
}

qint64 Notebook::creationTimestamp() const
{
    return m_qecNotebook.serviceCreated;
}

void Notebook::setCreationTimestamp(const qint64 timestamp)
{
    m_qecNotebook.serviceCreated = timestamp;
}

bool Notebook::hasModificationTimestamp() const
{
    return m_qecNotebook.serviceUpdated.isSet();
}

qint64 Notebook::modificationTimestamp() const
{
    return m_qecNotebook.serviceUpdated;
}

void Notebook::setModificationTimestamp(const qint64 timestamp)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set modification timestamp for notebook: updating is forbidden");
        return;
    }

    m_qecNotebook.serviceUpdated = timestamp;
}

bool Notebook::hasPublishingUri() const
{
    return m_qecNotebook.publishing.isSet() && m_qecNotebook.publishing->uri.isSet();
}

const QString & Notebook::publishingUri() const
{
    return m_qecNotebook.publishing->uri;
}

#define CHECK_AND_SET_PUBLISHING \
    if (!m_qecNotebook.publishing.isSet()) { \
        m_qecNotebook.publishing = qevercloud::Publishing(); \
    }

void Notebook::setPublishingUri(const QString & uri)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set publishing uri for notebook: update is forbidden");
        return;
    }

    if (!uri.isEmpty()) {
        CHECK_AND_SET_PUBLISHING;
    }

    m_qecNotebook.publishing->uri = uri;
}

bool Notebook::hasPublishingOrder() const
{
    return m_qecNotebook.publishing.isSet() && m_qecNotebook.publishing->order.isSet();
}

qint8 Notebook::publishingOrder() const
{
    return static_cast<qint8>(m_qecNotebook.publishing->order);
}

void Notebook::setPublishingOrder(const qint8 order)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set publishing order for notebook: update is forbidden");
        return;
    }

    if (order <= static_cast<qint8>(qevercloud::NoteSortOrder::TITLE)) {
        CHECK_AND_SET_PUBLISHING;
        m_qecNotebook.publishing->order = static_cast<qevercloud::NoteSortOrder::type>(order);
    }
    else if (m_qecNotebook.publishing.isSet()) {
        m_qecNotebook.publishing->order.clear();
    }
}

// TODO: continue from here

bool Notebook::hasPublishingAscending() const
{
    return m_enNotebook.__isset.publishing && m_enNotebook.publishing.__isset.ascending;
}

bool Notebook::isPublishingAscending() const
{
    return m_enNotebook.publishing.ascending;
}

void Notebook::setPublishingAscending(const bool ascending)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set publishing ascending for notebook: update is forbidden");
        return;
    }

    m_enNotebook.publishing.ascending = ascending;
    m_enNotebook.publishing.__isset.ascending = true;
    CHECK_AND_SET_PUBLISHING;
}

bool Notebook::hasPublishingPublicDescription() const
{
    return m_enNotebook.__isset.publishing && m_enNotebook.publishing.__isset.publicDescription;
}

const QString Notebook::publishingPublicDescription() const
{
    return std::move(QString::fromStdString(m_enNotebook.publishing.publicDescription));
}

void Notebook::setPublishingPublicDescription(const QString & publicDescription)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set publishing public description for notebook: update is forbidden");
        return;
    }

    m_enNotebook.publishing.publicDescription = publicDescription.toStdString();
    m_enNotebook.publishing.__isset.publicDescription = !publicDescription.isEmpty();
    CHECK_AND_SET_PUBLISHING;
}

bool Notebook::hasPublished() const
{
    return m_enNotebook.__isset.published;
}

#undef CHECK_AND_SET_PUBLISHING

bool Notebook::isPublished() const
{
    return m_enNotebook.__isset.published && m_enNotebook.published;
}

void Notebook::setPublished(const bool published)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set published flag for notebook: update is forbidden");
        return;
    }

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
    if (!canSetNotebookStack()) {
        QNDEBUG("Can't set stack for notebook: setting stack is forbidden");
        return;
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
    if (!canCreateSharedNotebooks() || !canUpdateNotebook()) {
        QNDEBUG("Can't set shared notebooks for notebook: restrictions apply");
        return;
    }

    size_t numNotebooks = notebooks.size();

    auto & sharedNotebooks = m_enNotebook.sharedNotebooks;
    sharedNotebooks.resize(numNotebooks);
    for(size_t i = 0; i < numNotebooks; ++i) {
        sharedNotebooks[i] = notebooks[i].GetEnSharedNotebook();
    }
}

void Notebook::addSharedNotebook(const ISharedNotebook & sharedNotebook)
{
    if (!canCreateSharedNotebooks() || !canUpdateNotebook()) {
        QNDEBUG("Can't add shared notebook for notebook: restrictions apply");
        return;
    }

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
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't remove shared notebook from notebook: updating is forbidden");
        return;
    }

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
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set business notebook description for notebook: updating is forbidden");
        return;
    }

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
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set business notebook privilege level for notebook: updating is forbidden");
        return;
    }

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

void Notebook::setBusinessNotebookRecommended(const bool recommended)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set business notebook recommended flag for notebook: updating is forbidden");
        return;
    }

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

UserAdapter Notebook::contact()
{
    return std::move(UserAdapter(m_enNotebook.contact));
}

void Notebook::setContact(const IUser & contact)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set contact for notebook: updating if forbidden");
        return;
    }

    m_enNotebook.contact = contact.GetEnUser();
    m_enNotebook.__isset.contact = (m_enNotebook.contact.id != 0);
}

bool Notebook::isLocal() const
{
    return m_isLocal;
}

void Notebook::setLocal(const bool local)
{
    m_isLocal = local;
}

bool Notebook::isLastUsed() const
{
    return m_isLastUsed;
}

void Notebook::setLastUsed(const bool lastUsed)
{
    m_isLastUsed = lastUsed;
}

bool Notebook::canReadNotes() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noReadNotes &&
             m_enNotebook.restrictions.noReadNotes);
}

void Notebook::setCanReadNotes(const bool canReadNotes)
{
    m_enNotebook.restrictions.noReadNotes = !canReadNotes;
    m_enNotebook.restrictions.__isset.noReadNotes = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canCreateNotes() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noCreateNotes &&
             m_enNotebook.restrictions.noCreateNotes);
}

void Notebook::setCanCreateNotes(const bool canCreateNotes)
{
    m_enNotebook.restrictions.noCreateNotes = !canCreateNotes;
    m_enNotebook.restrictions.__isset.noCreateNotes = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canUpdateNotes() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noUpdateNotes &&
             m_enNotebook.restrictions.noUpdateNotes);
}

void Notebook::setCanUpdateNotes(const bool canUpdateNotes)
{
    m_enNotebook.restrictions.noUpdateNotes = !canUpdateNotes;
    m_enNotebook.restrictions.__isset.noUpdateNotes = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canExpungeNotes() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noExpungeNotes &&
             m_enNotebook.restrictions.noExpungeNotes);
}

void Notebook::setCanExpungeNotes(const bool canExpungeNotes)
{
    m_enNotebook.restrictions.noExpungeNotes = !canExpungeNotes;
    m_enNotebook.restrictions.__isset.noExpungeNotes = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canShareNotes() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noShareNotes &&
             m_enNotebook.restrictions.noShareNotes);
}

void Notebook::setCanShareNotes(const bool canShareNotes)
{
    m_enNotebook.restrictions.noShareNotes = !canShareNotes;
    m_enNotebook.restrictions.__isset.noShareNotes = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canEmailNotes() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noEmailNotes &&
             m_enNotebook.restrictions.noEmailNotes);
}

void Notebook::setCanEmailNotes(const bool canEmailNotes)
{
    m_enNotebook.restrictions.noEmailNotes = !canEmailNotes;
    m_enNotebook.restrictions.__isset.noEmailNotes = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canSendMessageToRecipients() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noSendMessageToRecipients &&
             m_enNotebook.restrictions.noSendMessageToRecipients);
}

void Notebook::setCanSendMessageToRecipients(const bool canSendMessageToRecipients)
{
    m_enNotebook.restrictions.noSendMessageToRecipients = !canSendMessageToRecipients;
    m_enNotebook.restrictions.__isset.noSendMessageToRecipients = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canUpdateNotebook() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noUpdateNotebook &&
             m_enNotebook.restrictions.noUpdateNotebook);
}

void Notebook::setCanUpdateNotebook(const bool canUpdateNotebook)
{
    m_enNotebook.restrictions.noUpdateNotebook = !canUpdateNotebook;
    m_enNotebook.restrictions.__isset.noUpdateNotebook = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canExpungeNotebook() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noExpungeNotebook &&
             m_enNotebook.restrictions.noExpungeNotebook);
}

void Notebook::setCanExpungeNotebook(const bool canExpungeNotebook)
{
    m_enNotebook.restrictions.noExpungeNotebook = !canExpungeNotebook;
    m_enNotebook.restrictions.__isset.noExpungeNotebook = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canSetDefaultNotebook() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noSetDefaultNotebook &&
             m_enNotebook.restrictions.noSetDefaultNotebook);
}

void Notebook::setCanSetDefaultNotebook(const bool canSetDefaultNotebook)
{
    m_enNotebook.restrictions.noSetDefaultNotebook = !canSetDefaultNotebook;
    m_enNotebook.restrictions.__isset.noSetDefaultNotebook = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canSetNotebookStack() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noSetNotebookStack &&
             m_enNotebook.restrictions.noSetNotebookStack);
}

void Notebook::setCanSetNotebookStack(const bool canSetNotebookStack)
{
    m_enNotebook.restrictions.noSetNotebookStack = !canSetNotebookStack;
    m_enNotebook.restrictions.__isset.noSetNotebookStack = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canPublishToPublic() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noPublishToPublic &&
             m_enNotebook.restrictions.noPublishToPublic);
}

void Notebook::setCanPublishToPublic(const bool canPublishToPublic)
{
    m_enNotebook.restrictions.noPublishToPublic = !canPublishToPublic;
    m_enNotebook.restrictions.__isset.noPublishToPublic = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canPublishToBusinessLibrary() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noPublishToBusinessLibrary &&
             m_enNotebook.restrictions.noPublishToBusinessLibrary);
}

void Notebook::setCanPublishToBusinessLibrary(const bool canPublishToBusinessLibrary)
{
    m_enNotebook.restrictions.noPublishToBusinessLibrary = !canPublishToBusinessLibrary;
    m_enNotebook.restrictions.__isset.noPublishToBusinessLibrary = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canCreateTags() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noCreateTags &&
             m_enNotebook.restrictions.noCreateTags);
}

void Notebook::setCanCreateTags(const bool canCreateTags)
{
    m_enNotebook.restrictions.noCreateTags = !canCreateTags;
    m_enNotebook.restrictions.__isset.noCreateTags = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canUpdateTags() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noUpdateTags &&
             m_enNotebook.restrictions.noUpdateTags);
}

void Notebook::setCanUpdateTags(const bool canUpdateTags)
{
    m_enNotebook.restrictions.noUpdateTags = !canUpdateTags;
    m_enNotebook.restrictions.__isset.noUpdateTags = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canExpungeTags() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noExpungeTags &&
             m_enNotebook.restrictions.noExpungeTags);
}

void Notebook::setCanExpungeTags(const bool canExpungeTags)
{
    m_enNotebook.restrictions.noExpungeTags = !canExpungeTags;
    m_enNotebook.restrictions.__isset.noExpungeTags = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canSetParentTag() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noSetParentTag &&
             m_enNotebook.restrictions.noSetParentTag);
}

void Notebook::setCanSetParentTag(const bool canSetParentTag)
{
    m_enNotebook.restrictions.noSetParentTag = !canSetParentTag;
    m_enNotebook.restrictions.__isset.noSetParentTag = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::canCreateSharedNotebooks() const
{
    return !(m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.noCreateSharedNotebooks &&
             m_enNotebook.restrictions.noCreateSharedNotebooks);
}

void Notebook::setCanCreateSharedNotebooks(const bool canCreateSharedNotebooks)
{
    m_enNotebook.restrictions.noCreateSharedNotebooks = !canCreateSharedNotebooks;
    m_enNotebook.restrictions.__isset.noCreateSharedNotebooks = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::hasUpdateWhichSharedNotebookRestrictions() const
{
    return m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.updateWhichSharedNotebookRestrictions;
}

qint8 Notebook::updateWhichSharedNotebookRestrictions() const
{
    return static_cast<qint8>(m_enNotebook.restrictions.updateWhichSharedNotebookRestrictions);
}

void Notebook::setUpdateWhichSharedNotebookRestrictions(const qint8 which)
{
    m_enNotebook.restrictions.updateWhichSharedNotebookRestrictions = static_cast<evernote::edam::SharedNotebookInstanceRestrictions::type>(which);
    m_enNotebook.restrictions.__isset.updateWhichSharedNotebookRestrictions = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::hasExpungeWhichSharedNotebookRestrictions() const
{
    return m_enNotebook.__isset.restrictions && m_enNotebook.restrictions.__isset.expungeWhichSharedNotebookRestrictions;
}

qint8 Notebook::expungeWhichSharedNotebookRestrictions() const
{
    return static_cast<qint8>(m_enNotebook.restrictions.expungeWhichSharedNotebookRestrictions);
}

void Notebook::setExpungeWhichSharedNotebookRestrictions(const qint8 which)
{
    m_enNotebook.restrictions.expungeWhichSharedNotebookRestrictions = static_cast<evernote::edam::SharedNotebookInstanceRestrictions::type>(which);
    m_enNotebook.restrictions.__isset.expungeWhichSharedNotebookRestrictions = true;
    m_enNotebook.__isset.restrictions = true;
}

bool Notebook::hasRestrictions() const
{
    return m_enNotebook.__isset.restrictions;
}

const evernote::edam::NotebookRestrictions & Notebook::restrictions() const
{
    return m_enNotebook.restrictions;
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
        strm << "creationTimestamp: " << m_enNotebook.serviceCreated << ", datetime: "
             << PrintableDateTimeFromTimestamp(m_enNotebook.serviceCreated);
    }
    else {
        strm << "creationTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (isSet.serviceUpdated) {
        strm << "modificationTimestamp: " << m_enNotebook.serviceUpdated << ", datetime: "
             << PrintableDateTimeFromTimestamp(m_enNotebook.serviceUpdated);
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

    if (isDirty()) {
        strm << "notebook is dirty";
    }
    else {
        strm << "notebook is synchronized";
    }
    INSERT_DELIMITER;

    if (isLocal()) {
        strm << "notebook is local";
    }
    else {
        strm << "notebook is not local";
    }
    INSERT_DELIMITER;

    if (isLastUsed()) {
        strm << "notebook is last used";
    }
    else {
        strm << "notebook is not last used";
    }
    INSERT_DELIMITER;

#undef INSERT_DELIMITER

    return strm;
}

} // namespace qute_note
