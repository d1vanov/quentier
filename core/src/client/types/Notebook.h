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
    void setDefaultNotebookFlag(const bool defaultNotebook);

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
    void setPublisingAscending(const bool ascending);

    bool hasPublicDescription() const;
    const QString publicDescription() const;
    void setPublicDescription(const QString & publicDescription);

    bool isPublished() const;
    void setPublishedFlag(const bool published);

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
    void setBusinessNotebookRecommendedFlag(const bool recommended);

    bool hasContact() const;
    const UserAdapter contact() const;
    void setContact(const IUser & contact);

    // Restrictions
    bool canReadNotes() const;
    bool canCreateNotes() const;
    bool canUpdateNotes() const;
    bool canExpungeNotes() const;
    bool canShareNotes() const;
    bool canEmailNotes() const;

    bool canSendMessageToRecipients() const;

    bool canUpdateNotebook() const;
    bool canExpungeNotebook() const;
    bool canSetDefaultNotebook() const;
    bool canSetNotebookStack() const;

    bool canPublishToPublic() const;
    bool canPublishToBusinessLibrary() const;

    bool canCreateTags() const;
    bool canUpdateTags() const;
    bool canExpungeTags() const;
    bool canSetParentTag() const;

    bool canCreateSharedNotebooks() const;

    void setRestrictions(const evernote::edam::NotebookRestrictions & restrictions);

private:
    virtual QTextStream & Print(QTextStream & strm) const final override;

    evernote::edam::Notebook m_enNotebook;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__NOTEBOOK_H
