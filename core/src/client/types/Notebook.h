#ifndef __QUTE_NOTE__CLIENT__TYPES__NOTEBOOK_H
#define __QUTE_NOTE__CLIENT__TYPES__NOTEBOOK_H

#include "NoteStoreDataElement.h"
#include <Types_types.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ISharedNotebook)
QT_FORWARD_DECLARE_CLASS(SharedNotebookAdapter)
QT_FORWARD_DECLARE_CLASS(IUser)
QT_FORWARD_DECLARE_CLASS(UserAdapter)

class Notebook final: public NoteStoreDataElement
{
public:
    Notebook();
    Notebook(const Notebook & other);
    Notebook & operator =(const Notebook & other);
    virtual ~Notebook() final override;

    bool operator==(const Notebook & other) const;
    bool operator!=(const Notebook & other) const;

    virtual void clear() final override;

    virtual bool hasGuid() const final override;
    virtual const QString guid() const final override;
    virtual void setGuid(const QString & guid) final override;

    virtual bool hasUpdateSequenceNumber() const final override;
    virtual qint32 updateSequenceNumber() const final override;
    virtual void setUpdateSequenceNumber(const qint32 usn) final override;

    virtual bool checkParameters(QString & errorDescription) const final override;

    bool hasName() const;
    const QString name() const;
    void setName(const QString & name);

    bool isDefaultNotebook() const;
    void setDefaultNotebook(const bool defaultNotebook);

    bool hasCreationTimestamp() const;
    qint64 creationTimestamp() const;
    void setCreationTimestamp(const qint64 timestamp);

    bool hasModificationTimestamp() const;
    qint64 modificationTimestamp() const;
    void setModificationTimestamp(const qint64 timestamp);

    bool hasPublishingUri() const;
    const QString publishingUri() const;
    void setPublishingUri(const QString & uri);

    bool hasPublishingOrder() const;
    qint8 publishingOrder() const;
    void setPublishingOrder(const qint8 order);

    bool hasPublishingAscending() const;
    bool isPublishingAscending() const;
    void setPublishingAscending(const bool ascending);

    bool hasPublishingPublicDescription() const;
    const QString publishingPublicDescription() const;
    void setPublishingPublicDescription(const QString & publishingPublicDescription);

    bool hasPublished() const;
    bool isPublished() const;
    void setPublished(const bool published);

    bool hasStack() const;
    const QString stack() const;
    void setStack(const QString & stack);

    bool hasSharedNotebooks();
    void sharedNotebooks(std::vector<SharedNotebookAdapter> & notebooks) const;
    void setSharedNotebooks(std::vector<ISharedNotebook> & notebooks);
    void addSharedNotebook(const ISharedNotebook & sharedNotebook);
    void removeSharedNotebook(const ISharedNotebook & sharedNotebook);

    bool hasBusinessNotebookDescription() const;
    const QString businessNotebookDescription() const;
    void setBusinessNotebookDescription(const QString & businessNotebookDescription);

    bool hasBusinessNotebookPrivilegeLevel() const;
    qint8 businessNotebookPrivilegeLevel() const;
    void setBusinessNotebookPrivilegeLevel(const qint8 privilegeLevel);

    bool hasBusinessNotebookRecommended() const;
    bool isBusinessNotebookRecommended() const;
    void setBusinessNotebookRecommended(const bool recommended);

    bool hasContact() const;
    const UserAdapter contact() const;
    void setContact(const IUser & contact);

    bool isLocal() const;
    void setLocal(const bool local);

    bool isLastUsed() const;
    void setLastUsed(const bool lastUsed);

    // Restrictions
    bool canReadNotes() const;
    void setCanReadNotes(const bool canReadNotes);

    bool canCreateNotes() const;
    void setCanCreateNotes(const bool canCreateNotes);

    bool canUpdateNotes() const;
    void setCanUpdateNotes(const bool canUpdateNotes);

    bool canExpungeNotes() const;
    void setCanExpungeNotes(const bool canExpungeNotes);

    bool canShareNotes() const;
    void setCanShareNotes(const bool canShareNotes);

    bool canEmailNotes() const;
    void setCanEmailNotes(const bool canEmailNotes);

    bool canSendMessageToRecipients() const;
    void setCanSendMessageToRecipients(const bool canSendMessageToRecipients);

    bool canUpdateNotebook() const;
    void setCanUpdateNotebook(const bool canUpdateNotebook);

    bool canExpungeNotebook() const;
    void setCanExpungeNotebook(const bool canExpungeNotebook);

    bool canSetDefaultNotebook() const;
    void setCanSetDefaultNotebook(const bool canSetDefaultNotebook);

    bool canSetNotebookStack() const;
    void setCanSetNotebookStack(const bool canSetNotebookStack);

    bool canPublishToPublic() const;
    void setCanPublishToPublic(const bool canPublishToPublic);

    bool canPublishToBusinessLibrary() const;
    void setCanPublishToBusinessLibrary(const bool canPublishToBusinessLibrary);

    bool canCreateTags() const;
    void setCanCreateTags(const bool canCreateTags);

    bool canUpdateTags() const;
    void setCanUpdateTags(const bool canUpdateTags);

    bool canExpungeTags() const;
    void setCanExpungeTags(const bool canExpungeTags);

    bool canSetParentTag() const;
    void setCanSetParentTag(const bool canSetParentTag);

    bool canCreateSharedNotebooks() const;
    void setCanCreateSharedNotebooks(const bool canCreateSharedNotebooks);

    bool hasUpdateWhichSharedNotebookRestrictions() const;
    qint8 updateWhichSharedNotebookRestrictions() const;
    void setUpdateWhichSharedNotebookRestrictions(const qint8 which);

    bool hasExpungeWhichSharedNotebookRestrictions() const;
    qint8 expungeWhichSharedNotebookRestrictions() const;
    void setExpungeWhichSharedNotebookRestrictions(const qint8 which);

    bool hasRestrictions() const;
    const evernote::edam::NotebookRestrictions & restrictions() const;

private:
    virtual QTextStream & Print(QTextStream & strm) const final override;

    evernote::edam::Notebook m_enNotebook;
    bool   m_isLocal;
    bool   m_isLastUsed;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__NOTEBOOK_H
