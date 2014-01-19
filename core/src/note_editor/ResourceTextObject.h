#ifndef __QUTE_NOTE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H
#define __QUTE_NOTE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H

#include <QScopedPointer>
#include <QTextObjectInterface>

namespace evernote {
namespace edam {

QT_FORWARD_DECLARE_CLASS(Resource)

}
}

class ResourceTextObject: public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit ResourceTextObject(const evernote::edam::Resource & resource);
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

    const QScopedPointer<evernote::edam::Resource> m_pResource;
    QImage m_resourceImage;
};

#endif // __QUTE_NOTE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H
