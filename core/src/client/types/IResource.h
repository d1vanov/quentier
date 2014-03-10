#ifndef __QUTE_NOTE__CLIENT__TYPES__IRESOURCE_H
#define __QUTE_NOTE__CLIENT__TYPES__IRESOURCE_H

#include "NoteStoreDataElement.h"

namespace qute_note {

class IResource: public NoteStoreDataElement
{
public:
    IResource();
    IResource(const bool isFreeAccount);
    virtual ~IResource();

    virtual void clear() final override;

    virtual bool hasGuid() const final override;
    virtual const QString guid() const final override;
    virtual void setGuid(const QString & guid) final override;

    virtual bool hasUpdateSequenceNumber() const final override;
    virtual qint32 updateSequenceNumber() const final override;
    virtual void setUpdateSequenceNumber(const qint32 updateSequenceNumber) final override;

    virtual bool checkParameters(QString & errorDescription) const final override;

    bool isFreeAccount() const;
    void setFreeAccount(const bool isFreeAccount);

    bool hasNoteGuid() const;
    const QString noteGuid() const;
    void setNoteGuid(const QString & guid);

    bool hasData() const;

    bool hasDataHash() const;
    const QString dataHash() const;
    void setDataHash(const QString & hash);

    bool hasDataSize() const;
    qint32 dataSize() const;
    void setDataSize(const qint32 size);

    bool hasDataBody() const;
    const QString dataBody() const;
    void setDataBody(const QString & body);

    bool hasMime() const;
    const QString mime() const;
    void setMime(const QString & mime);

    bool hasWidth() const;
    qint16 width() const;
    void setWidth(const qint16 width);

    bool hasHeight() const;
    qint16 height() const;
    void setHeight(const qint16 height);

    bool hasRecognitionData() const;

    bool hasRecognitionDataHash() const;
    const QString recognitionDataHash() const;
    void setRecognitionDataHash(const QString & hash);

    bool hasRecognitionDataSize() const;
    qint32 recognitionDataSize() const;
    void setRecognitionDataSize(const qint32 size);

    bool hasRecognitionDataBody() const;
    const QString recognitionDataBody() const;
    void setRecognitionDataBody(const QString & body);

    bool hasAlternateData() const;

    bool hasAlternateDataHash() const;
    const QString alternateDataHash() const;
    void setAlternateDataHash(const QString & hash);

    bool hasAlternateDataSize() const;
    qint32 alternateDataSize() const;
    void setAlternateDataSize(const qint32 size);

    bool hasAlternateDataBody() const;
    const QString alternateDataBody() const;
    void setAlternateDataBody(const QString & body);

    bool hasResourceAttributes() const;
    const QByteArray resourceAttributes() const;
    void setResourceAttributes(const QByteArray & resourceAttributes);

protected:
    IResource(const IResource & other);
    IResource & operator=(const IResource & other);

    virtual const evernote::edam::Resource & GetEnResource() const = 0;
    virtual evernote::edam::Resource & GetEnResource() = 0;

    virtual QTextStream & Print(QTextStream & strm) const;

private:    
    bool m_isFreeAccount;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__IRESOURCE_H
