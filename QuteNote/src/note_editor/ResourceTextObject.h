#ifndef __QUTE_NOTE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H
#define __QUTE_NOTE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H

#include <QTextObjectInterface>

namespace qute_note {
class Resource;
}

class ResourceTextObject: public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit ResourceTextObject(const qute_note::Resource * pResource);
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

    const qute_note::Resource * m_pResource;
};

#endif // __QUTE_NOTE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H
