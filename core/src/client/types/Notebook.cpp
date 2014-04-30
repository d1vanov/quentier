#include "Notebook.h"
#include "SharedNotebookAdapter.h"
#include "SharedNotebookWrapper.h"
#include "UserAdapter.h"
#include "QEverCloudHelpers.h"
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
    else if (m_qecNotebook.sharedNotebooks.isSet() != otherNotebook.sharedNotebooks.isSet()) {
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

    if (m_qecNotebook.sharedNotebooks.isSet()) {
        const QList<qevercloud::SharedNotebook> & sharedNotebooks = m_qecNotebook.sharedNotebooks;
        const QList<qevercloud::SharedNotebook> & otherSharedNotebooks = otherNotebook.sharedNotebooks;

        if (sharedNotebooks != otherSharedNotebooks) {
            return false;
        }
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

bool Notebook::hasPublishingAscending() const
{
    return m_qecNotebook.publishing.isSet() && m_qecNotebook.publishing->ascending.isSet();
}

bool Notebook::isPublishingAscending() const
{
    return m_qecNotebook.publishing->ascending;
}

void Notebook::setPublishingAscending(const bool ascending)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set publishing ascending for notebook: update is forbidden");
        return;
    }

    CHECK_AND_SET_PUBLISHING;
    m_qecNotebook.publishing->ascending = ascending;
}

bool Notebook::hasPublishingPublicDescription() const
{
    return m_qecNotebook.publishing.isSet() && m_qecNotebook.publishing->publicDescription.isSet();
}

const QString & Notebook::publishingPublicDescription() const
{
    return m_qecNotebook.publishing->publicDescription;
}

void Notebook::setPublishingPublicDescription(const QString & publicDescription)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set publishing public description for notebook: update is forbidden");
        return;
    }

    if (!publicDescription.isEmpty()) {
        CHECK_AND_SET_PUBLISHING;
    }

    m_qecNotebook.publishing->publicDescription = publicDescription;
}

#undef CHECK_AND_SET_PUBLISHING

bool Notebook::hasPublished() const
{
    return m_qecNotebook.published.isSet();
}

bool Notebook::isPublished() const
{
    return m_qecNotebook.published.isSet();
}

void Notebook::setPublished(const bool published)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set published flag for notebook: update is forbidden");
        return;
    }

    m_qecNotebook.published = published;
}

bool Notebook::hasStack() const
{
    return m_qecNotebook.stack.isSet();
}

const QString & Notebook::stack() const
{
    return m_qecNotebook.stack;
}

void Notebook::setStack(const QString & stack)
{
    if (!canSetNotebookStack()) {
        QNDEBUG("Can't set stack for notebook: setting stack is forbidden");
        return;
    }

    m_qecNotebook.stack = stack;
}

bool Notebook::hasSharedNotebooks()
{
    return m_qecNotebook.sharedNotebooks.isSet();
}

void Notebook::sharedNotebooks(QList<SharedNotebookAdapter> & notebooks) const
{
    notebooks.clear();

    if (!m_qecNotebook.sharedNotebooks.isSet()) {
        return;
    }

    const QList<qevercloud::SharedNotebook> & sharedNotebooks = m_qecNotebook.sharedNotebooks;
    foreach(const qevercloud::SharedNotebook & sharedNotebook, sharedNotebooks) {
        SharedNotebookAdapter sharedNotebookadapter(sharedNotebook);
        notebooks << sharedNotebookadapter;
    }
}

#define SHARED_NOTEBOOKS_SETTER(type) \
    void Notebook::setSharedNotebooks(QList<type> && notebooks) \
    { \
        if (!canCreateSharedNotebooks() || !canUpdateNotebook()) { \
            QNDEBUG("Can't set shared notebooks for notebook: restrictions apply"); \
            return; \
        } \
        \
        if (!m_qecNotebook.sharedNotebooks.isSet()) { \
            m_qecNotebook.sharedNotebooks = QList<qevercloud::SharedNotebook>(); \
        } \
        \
        m_qecNotebook.sharedNotebooks->clear(); \
        foreach(const type & sharedNotebook, notebooks) { \
            m_qecNotebook.sharedNotebooks.ref() << sharedNotebook.GetEnSharedNotebook(); \
        } \
    }

SHARED_NOTEBOOKS_SETTER(SharedNotebookAdapter)
SHARED_NOTEBOOKS_SETTER(SharedNotebookWrapper)

#undef SHARED_NOTEBOOKS_SETTER

void Notebook::addSharedNotebook(const ISharedNotebook & sharedNotebook)
{
    if (!canCreateSharedNotebooks() || !canUpdateNotebook()) {
        QNDEBUG("Can't add shared notebook for notebook: restrictions apply");
        return;
    }

    if (!m_qecNotebook.sharedNotebooks.isSet()) {
        m_qecNotebook.sharedNotebooks = QList<qevercloud::SharedNotebook>();
    }

    QList<qevercloud::SharedNotebook> & sharedNotebooks = m_qecNotebook.sharedNotebooks;
    const auto & enSharedNotebook = sharedNotebook.GetEnSharedNotebook();

    if (sharedNotebooks.indexOf(enSharedNotebook) != -1) {
        QNDEBUG("Can't add shared notebook: this shared notebook already exists within the notebook");
        return;
    }

    sharedNotebooks << enSharedNotebook;
}

void Notebook::removeSharedNotebook(const ISharedNotebook & sharedNotebook)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't remove shared notebook from notebook: updating is forbidden");
        return;
    }

    auto & sharedNotebooks = m_qecNotebook.sharedNotebooks;
    const auto & enSharedNotebook = sharedNotebook.GetEnSharedNotebook();

    int index = sharedNotebooks->indexOf(enSharedNotebook);
    if (index == -1) {
        QNDEBUG("Can't remove shared notebook: this shared notebook doesn't exists within the notebook");
        return;
    }

    sharedNotebooks->removeAt(index);
}

bool Notebook::hasBusinessNotebookDescription() const
{
    return m_qecNotebook.businessNotebook.isSet() && m_qecNotebook.businessNotebook->notebookDescription.isSet();
}

const QString & Notebook::businessNotebookDescription() const
{
    return m_qecNotebook.businessNotebook->notebookDescription;
}

#define CHECK_AND_SET_BUSINESS_NOTEBOOK \
    if (!m_qecNotebook.businessNotebook.isSet()) { \
        m_qecNotebook.businessNotebook = qevercloud::BusinessNotebook(); \
    }

void Notebook::setBusinessNotebookDescription(const QString & businessNotebookDescription)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set business notebook description for notebook: updating is forbidden");
        return;
    }

    if (!businessNotebookDescription.isEmpty()) {
        CHECK_AND_SET_BUSINESS_NOTEBOOK;
    }

    m_qecNotebook.businessNotebook->notebookDescription = businessNotebookDescription;
}

bool Notebook::hasBusinessNotebookPrivilegeLevel() const
{
    return m_qecNotebook.businessNotebook.isSet() && m_qecNotebook.businessNotebook->privilege.isSet();
}

qint8 Notebook::businessNotebookPrivilegeLevel() const
{
    return static_cast<qint8>(m_qecNotebook.businessNotebook->privilege);
}

void Notebook::setBusinessNotebookPrivilegeLevel(const qint8 privilegeLevel)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set business notebook privilege level for notebook: updating is forbidden");
        return;
    }

    if (privilegeLevel <= static_cast<qint8>(qevercloud::SharedNotebookPrivilegeLevel::BUSINESS_FULL_ACCESS)) {
        CHECK_AND_SET_BUSINESS_NOTEBOOK;
        m_qecNotebook.businessNotebook->privilege = static_cast<qevercloud::SharedNotebookPrivilegeLevel::type>(privilegeLevel);
    }
    else if (m_qecNotebook.businessNotebook.isSet()) {
        m_qecNotebook.businessNotebook->privilege.clear();
    }
}

bool Notebook::hasBusinessNotebookRecommended() const
{
    return m_qecNotebook.businessNotebook.isSet() && m_qecNotebook.businessNotebook->recommended.isSet();
}

bool Notebook::isBusinessNotebookRecommended() const
{
    return m_qecNotebook.businessNotebook->recommended;
}

void Notebook::setBusinessNotebookRecommended(const bool recommended)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set business notebook recommended flag for notebook: updating is forbidden");
        return;
    }

    CHECK_AND_SET_BUSINESS_NOTEBOOK;
    m_qecNotebook.businessNotebook->recommended = recommended;
}

#undef CHECK_AND_SET_BUSINESS_NOTEBOOK

bool Notebook::hasContact() const
{
    return m_qecNotebook.contact.isSet();
}

const UserAdapter Notebook::contact() const
{
    return std::move(UserAdapter(m_qecNotebook.contact));
}

UserAdapter Notebook::contact()
{
    return std::move(UserAdapter(m_qecNotebook.contact));
}

void Notebook::setContact(const IUser & contact)
{
    if (!canUpdateNotebook()) {
        QNDEBUG("Can't set contact for notebook: updating if forbidden");
        return;
    }

    m_qecNotebook.contact = contact.GetEnUser();
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
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noReadNotes.isSet() &&
             m_qecNotebook.restrictions->noReadNotes);
}

#define CHECK_AND_SET_NOTEBOOK_RESTRICTIONS \
    if (!m_qecNotebook.restrictions.isSet()) { \
        m_qecNotebook.restrictions = qevercloud::NotebookRestrictions(); \
    }

void Notebook::setCanReadNotes(const bool canReadNotes)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noReadNotes = !canReadNotes;
}

bool Notebook::canCreateNotes() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noCreateNotes.isSet() &&
             m_qecNotebook.restrictions->noCreateNotes);
}

void Notebook::setCanCreateNotes(const bool canCreateNotes)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noCreateNotes = !canCreateNotes;
}

bool Notebook::canUpdateNotes() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noUpdateNotes.isSet() &&
             m_qecNotebook.restrictions->noUpdateNotes);
}

void Notebook::setCanUpdateNotes(const bool canUpdateNotes)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noUpdateNotes = !canUpdateNotes;
}

bool Notebook::canExpungeNotes() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noExpungeNotes.isSet() &&
             m_qecNotebook.restrictions->noExpungeNotes);
}

void Notebook::setCanExpungeNotes(const bool canExpungeNotes)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noExpungeNotes = !canExpungeNotes;
}

bool Notebook::canShareNotes() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noShareNotes.isSet() &&
             m_qecNotebook.restrictions->noShareNotes);
}

void Notebook::setCanShareNotes(const bool canShareNotes)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noShareNotes = !canShareNotes;
}

bool Notebook::canEmailNotes() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noEmailNotes.isSet() &&
             m_qecNotebook.restrictions->noEmailNotes);
}

void Notebook::setCanEmailNotes(const bool canEmailNotes)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noEmailNotes = !canEmailNotes;
}

bool Notebook::canSendMessageToRecipients() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noSendMessageToRecipients.isSet() &&
             m_qecNotebook.restrictions->noSendMessageToRecipients);
}

void Notebook::setCanSendMessageToRecipients(const bool canSendMessageToRecipients)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noSendMessageToRecipients = !canSendMessageToRecipients;
}

bool Notebook::canUpdateNotebook() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noUpdateNotebook.isSet() &&
             m_qecNotebook.restrictions->noUpdateNotebook);
}

void Notebook::setCanUpdateNotebook(const bool canUpdateNotebook)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noUpdateNotebook = !canUpdateNotebook;
}

bool Notebook::canExpungeNotebook() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noExpungeNotebook.isSet() &&
             m_qecNotebook.restrictions->noExpungeNotebook);
}

void Notebook::setCanExpungeNotebook(const bool canExpungeNotebook)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noExpungeNotebook = !canExpungeNotebook;
}

bool Notebook::canSetDefaultNotebook() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noSetDefaultNotebook.isSet() &&
             m_qecNotebook.restrictions->noSetDefaultNotebook);
}

void Notebook::setCanSetDefaultNotebook(const bool canSetDefaultNotebook)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noSetDefaultNotebook = !canSetDefaultNotebook;
}

bool Notebook::canSetNotebookStack() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noSetNotebookStack.isSet() &&
             m_qecNotebook.restrictions->noSetNotebookStack);
}

void Notebook::setCanSetNotebookStack(const bool canSetNotebookStack)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noSetNotebookStack = !canSetNotebookStack;
}

bool Notebook::canPublishToPublic() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noPublishToPublic.isSet() &&
             m_qecNotebook.restrictions->noPublishToPublic);
}

void Notebook::setCanPublishToPublic(const bool canPublishToPublic)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noPublishToPublic = !canPublishToPublic;
}

bool Notebook::canPublishToBusinessLibrary() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noPublishToBusinessLibrary.isSet() &&
             m_qecNotebook.restrictions->noPublishToBusinessLibrary);
}

void Notebook::setCanPublishToBusinessLibrary(const bool canPublishToBusinessLibrary)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noPublishToBusinessLibrary = !canPublishToBusinessLibrary;
}

bool Notebook::canCreateTags() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noCreateTags.isSet() &&
             m_qecNotebook.restrictions->noCreateTags);
}

void Notebook::setCanCreateTags(const bool canCreateTags)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noCreateTags = !canCreateTags;
}

bool Notebook::canUpdateTags() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noUpdateTags.isSet() &&
             m_qecNotebook.restrictions->noUpdateTags);
}

void Notebook::setCanUpdateTags(const bool canUpdateTags)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noUpdateTags = !canUpdateTags;
}

bool Notebook::canExpungeTags() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noExpungeTags.isSet() &&
             m_qecNotebook.restrictions->noExpungeTags);
}

void Notebook::setCanExpungeTags(const bool canExpungeTags)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noExpungeTags = !canExpungeTags;
}

bool Notebook::canSetParentTag() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noSetParentTag.isSet() &&
             m_qecNotebook.restrictions->noSetParentTag);
}

void Notebook::setCanSetParentTag(const bool canSetParentTag)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noSetParentTag = !canSetParentTag;
}

bool Notebook::canCreateSharedNotebooks() const
{
    return !(m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->noCreateSharedNotebooks.isSet() &&
             m_qecNotebook.restrictions->noCreateSharedNotebooks);
}

void Notebook::setCanCreateSharedNotebooks(const bool canCreateSharedNotebooks)
{
    CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
    m_qecNotebook.restrictions->noCreateSharedNotebooks = !canCreateSharedNotebooks;
}

bool Notebook::hasUpdateWhichSharedNotebookRestrictions() const
{
    return m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->updateWhichSharedNotebookRestrictions.isSet();
}

qint8 Notebook::updateWhichSharedNotebookRestrictions() const
{
    return static_cast<qint8>(m_qecNotebook.restrictions->updateWhichSharedNotebookRestrictions);
}

void Notebook::setUpdateWhichSharedNotebookRestrictions(const qint8 which)
{
    if (which <= static_cast<qint8>(qevercloud::SharedNotebookInstanceRestrictions::ONLY_JOINED_OR_PREVIEW)) {
        CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
        m_qecNotebook.restrictions->updateWhichSharedNotebookRestrictions = static_cast<qevercloud::SharedNotebookInstanceRestrictions::type>(which);
    }
    else if (m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->updateWhichSharedNotebookRestrictions.isSet()) {
        m_qecNotebook.restrictions->updateWhichSharedNotebookRestrictions.clear();
    }
}

bool Notebook::hasExpungeWhichSharedNotebookRestrictions() const
{
    return m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->expungeWhichSharedNotebookRestrictions.isSet();
}

qint8 Notebook::expungeWhichSharedNotebookRestrictions() const
{
    return static_cast<qint8>(m_qecNotebook.restrictions->expungeWhichSharedNotebookRestrictions);
}

void Notebook::setExpungeWhichSharedNotebookRestrictions(const qint8 which)
{
    if (which <= static_cast<qint8>(qevercloud::SharedNotebookInstanceRestrictions::ONLY_JOINED_OR_PREVIEW)) {
        CHECK_AND_SET_NOTEBOOK_RESTRICTIONS;
        m_qecNotebook.restrictions->expungeWhichSharedNotebookRestrictions = static_cast<qevercloud::SharedNotebookInstanceRestrictions::type>(which);
    }
    else if (m_qecNotebook.restrictions.isSet() && m_qecNotebook.restrictions->expungeWhichSharedNotebookRestrictions.isSet()) {
        m_qecNotebook.restrictions->expungeWhichSharedNotebookRestrictions.clear();
    }
}

#undef CHECK_AND_SET_NOTEBOOK_RESTRICTIONS

bool Notebook::hasRestrictions() const
{
    return m_qecNotebook.restrictions.isSet();
}

const qevercloud::NotebookRestrictions & Notebook::restrictions() const
{
    return m_qecNotebook.restrictions;
}

QTextStream & Notebook::Print(QTextStream & strm) const
{
    strm << "Notebook { \n";

#define INSERT_DELIMITER \
    strm << "; \n"

    if (m_qecNotebook.guid.isSet()) {
        strm << "guid: " << m_qecNotebook.guid;
    }
    else {
        strm << "guid is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNotebook.name.isSet()) {
        strm << "name: " << m_qecNotebook.name;
    }
    else {
        strm << "name is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNotebook.updateSequenceNum.isSet()) {
        strm << "updateSequenceNumber: " << m_qecNotebook.updateSequenceNum;
    }
    else {
        strm << "updateSequenceNumber is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNotebook.defaultNotebook.isSet()) {
        strm << "defaultNotebook: " << (m_qecNotebook.defaultNotebook ? "true" : "false");
    }
    else {
        strm << "defaultNotebook is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNotebook.serviceCreated.isSet()) {
        strm << "creationTimestamp: " << m_qecNotebook.serviceCreated << ", datetime: "
             << PrintableDateTimeFromTimestamp(m_qecNotebook.serviceCreated);
    }
    else {
        strm << "creationTimestamp is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNotebook.serviceUpdated.isSet()) {
        strm << "modificationTimestamp: " << m_qecNotebook.serviceUpdated << ", datetime: "
             << PrintableDateTimeFromTimestamp(m_qecNotebook.serviceUpdated);
    }
    else {
        strm << "modificationTimestamp is not set";
    }

    if (m_qecNotebook.publishing.isSet())
    {
        const qevercloud::Publishing & publishing = m_qecNotebook.publishing;

        if (publishing.uri.isSet()) {
            strm << "publishingUri: " << publishing.uri;
        }
        else {
            strm << "publishingUri is not set";
        }
        INSERT_DELIMITER;

        if (publishing.order.isSet()) {
            // TODO: (re)implement printing of NoteSortOrder::type
            strm << "publishingOrder: " << QString::number(publishing.order);
        }
        else {
            strm << "publishingOrder is not set";
        }
        INSERT_DELIMITER;

        if (publishing.ascending.isSet()) {
            strm << "publishingAscending: " << (publishing.ascending ? "true" : "false");
        }
        else {
            strm << "publishingAscending is not set";
        }
        INSERT_DELIMITER;

        if (publishing.publicDescription.isSet()) {
            strm << "publishingPublicDescription: " << publishing.publicDescription;
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

    if (m_qecNotebook.published.isSet()) {
        strm << "published: " << (m_qecNotebook.published ? "true" : "false");
    }
    else {
        strm << "published is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNotebook.stack.isSet()) {
        strm << "stack: " << m_qecNotebook.stack;
    }
    else {
        strm << "stack is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNotebook.sharedNotebooks.isSet()) {
        strm << "sharedNotebooks: \n";
        foreach(const qevercloud::SharedNotebook & sharedNotebook, m_qecNotebook.sharedNotebooks.ref()) {
            strm << SharedNotebookAdapter(sharedNotebook);
        }
    }
    else {
        strm << "sharedNotebooks are not set";
    }
    INSERT_DELIMITER;

    if (m_qecNotebook.businessNotebook.isSet())
    {
        const qevercloud::BusinessNotebook & businessNotebook = m_qecNotebook.businessNotebook;

        if (businessNotebook.notebookDescription.isSet()) {
            strm << "businessNotebookDescription: " << businessNotebook.notebookDescription;
        }
        else {
            strm << "businessNotebookDescription is not set";
        }
        INSERT_DELIMITER;

        if (businessNotebook.privilege.isSet()) {
            // TODO: (re)implement printing of SharedNotebookPrivilegeLevel::type
            strm << "businessNotebookPrivilegeLevel: " << QString::number(businessNotebook.privilege);
        }
        else {
            strm << "businessNotebookPrivilegeLevel is not set";
        }
        INSERT_DELIMITER;

        if (businessNotebook.recommended.isSet()) {
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

    if (m_qecNotebook.contact.isSet()) {
        strm << "contact: \n";
        strm << UserAdapter(m_qecNotebook.contact);
    }
    else {
        strm << "contact is not set";
    }
    INSERT_DELIMITER;

    if (m_qecNotebook.restrictions.isSet()) {
        // TODO: (re)implement printing of NotebookRestrictions
        // strm << "restrictions: " << m_qecNotebook.restrictions;
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
