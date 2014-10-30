#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H

#include <tools/Linkage.h>
#include <tools/qt4helper.h>
#include <QScopedPointer>
#include <QTextObjectInterface>

namespace qevercloud {
QT_FORWARD_DECLARE_STRUCT(Resource)
}

class QUTE_NOTE_EXPORT ResourceTextObject: public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit ResourceTextObject(const qevercloud::Resource & resource);
    ResourceTextObject(const ResourceTextObject & other);
    ResourceTextObject & operator =(const ResourceTextObject & other);
    virtual ~ResourceTextObject();

    bool isValid() const;

    // QTextObjectInterface
    virtual void drawObject(QPainter * pPainter, const QRectF & rect,
                            QTextDocument * pDoc, int positionInDocument,
                            const QTextFormat & format) Q_DECL_OVERRIDE;
    virtual QSizeF intrinsicSize(QTextDocument * pDoc, int positionInDocument,
                                 const QTextFormat & format) Q_DECL_OVERRIDE;

private:
    ResourceTextObject() Q_DECL_DELETE;

    const QScopedPointer<qevercloud::Resource> m_pResource;
    QImage m_resourceImage;
};

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__RESOURCE_TEXT_OBJECT_H
