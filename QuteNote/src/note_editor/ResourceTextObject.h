#ifndef __QUTE_NOTE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H
#define __QUTE_NOTE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H

#include "../evernote_client/ResourceMetadata.h"
#include <QTextObjectInterface>

class ResourceTextObject: public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit ResourceTextObject(const qute_note::ResourceMetadata & resourceMetadata,
                                const QByteArray & resourceBinaryData);
    ResourceTextObject(const ResourceTextObject & other);
    ResourceTextObject & operator =(const ResourceTextObject & other);
    virtual ~ResourceTextObject() final override;

    bool isValid() const;

    // QTextObjectInterface
    virtual void drawObject(QPainter * pPainter, const QRectF & rect,
                            QTextDocument * pDoc, int positionInDocument,
                            const QTextFormat & format) final override;
    virtual QSizeF intrinsicSize(QTextDocument * pDoc, int positionInDocument,
                                 const QTextFormat & format) final override;

private:
    ResourceTextObject() = delete;

    qute_note::ResourceMetadata m_resourceMetadata;
    QImage m_resourceImage;
};

#endif // __QUTE_NOTE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H
