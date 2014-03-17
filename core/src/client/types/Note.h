#ifndef __QUTE_NOTE__CLIENT__TYPES__NOTE_H
#define __QUTE_NOTE__CLIENT__TYPES__NOTE_H

#include "NoteStoreDataElement.h"
#include "ResourceAdapter.h"
#include <Types_types.h>

namespace qute_note {

class Note final: public NoteStoreDataElement
{
public:
    Note();
    Note(const Note & other);
    Note & operator=(const Note & other);
    virtual ~Note() final override;

    virtual bool hasGuid() const final override;
    virtual const QString guid() const final override;
    virtual void setGuid(const QString & guid) final override;

    virtual bool hasUpdateSequenceNumber() const final override;
    virtual qint32 updateSequenceNumber() const final override;
    virtual void setUpdateSequenceNumber(const qint32 usn) final override;

    virtual void clear() final override;

    virtual bool checkParameters(QString &errorDescription) const final override;

    bool hasTitle() const;
    const QString title() const;
    void setTitle(const QString & title);

    bool hasContent() const;
    const QString content() const;
    void setContent(const QString & content);

    bool hasContentHash() const;
    const QString contentHash() const;
    void setContentHash(const QString & contentHash);

    bool hasContentLength() const;
    qint32 contentLength() const;
    void setContentLength(const qint32 length);

    bool hasCreationTimestamp() const;
    qint64 creationTimestamp() const;
    void setCreationTimestamp(const qint64 timestamp);

    bool hasModificationTimestamp() const;
    qint64 modificationTimestamp() const;
    void setModificationTimestamp(const qint64 timestamp);

    bool hasDeletionTimestamp() const;
    qint64 deletionTimestamp() const;
    void setDeletionTimestamp(const qint64 timestamp);

    bool hasActive() const;
    bool active() const;
    void setActive(const bool active);

    bool hasNotebookGuid() const;
    const QString notebookGuid() const;
    void setNotebookGuid(const QString & guid);

    // TODO: what to do with tag guids? Convert them to QStrings?

    bool hasResources() const;
    void resources(std::vector<ResourceAdapter> & resources) const;
    void setResources(std::vector<IResource> & resources);
    void addResource(const IResource & resource);
    void removeResource(const IResource & resource);

    bool hasNoteAttributes() const;
    const QByteArray noteAttributes() const;
    void setNoteAttributes(const QByteArray & noteAttributes);

private:
    virtual QTextStream & Print(QTextStream & strm) const final override;

    evernote::edam::Note m_enNote;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__NOTE_H
