#include "ResourceTextObject.h"
#include "../evernote_client/Resource.h"
#include "../tools/QuteNoteCheckPtr.h"
#include <QPainter>

using namespace qute_note;

ResourceTextObject::ResourceTextObject(const Resource *pResource) :
    m_pResource(pResource)
{}

ResourceTextObject::ResourceTextObject(const ResourceTextObject & other) :
    m_pResource(other.m_pResource)
{}

ResourceTextObject & ResourceTextObject::operator =(const ResourceTextObject & other)
{
    if (this != &other) {
        m_pResource = other.m_pResource;
    }

    return *this;
}

ResourceTextObject::~ResourceTextObject()
{}

bool ResourceTextObject::isValid() const
{
    // TODO: check whether I can use any other useful information from the resource
    if (m_pResource != nullptr) {
        return true;
    }
    else {
        return false;
    }
}

void ResourceTextObject::drawObject(QPainter * /* pPainter */, const QRectF & /* rect */,
                                    QTextDocument * /* pDoc */, int /* positionInDocument */,
                                    const QTextFormat & /* format */)
{
    if (m_pResource == nullptr) {
        return;
    }

    // FIXME: this approach is really bad since mime data is non-copyable. Will rethink it.
    /*
    const QMimeData & mimeData = m_pResource->mimeData();
    bool hasImage = mimeData.hasImage();
    if (hasImage)
    {
        QImage img = qvariant_cast<QImage>(mimeData.imageData());

        QUTE_NOTE_CHECK_PTR(pPainter, "Pointer to QPainter is null when trying to draw ResourceTextObject");
        pPainter->drawImage(rect, img);
        return;
    }
    */

    // TODO: add some default drawing algorithm for non-image resources: name and type, for example
}

QSizeF ResourceTextObject::intrinsicSize(QTextDocument * /* pDoc */, int /* positionInDocument */,
                                         const QTextFormat & /* format */)
{
    if (m_pResource == nullptr) {
        return QSizeF();
    }

    int width  = static_cast<int>(m_pResource->width());
    int height = static_cast<int>(m_pResource->height());
    return QSizeF(QSize(width, height));
}

