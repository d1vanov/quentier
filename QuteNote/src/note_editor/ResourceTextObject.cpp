#include "ResourceTextObject.h"
#include "../tools/QuteNoteCheckPtr.h"
#include <QPainter>

using namespace qute_note;

ResourceTextObject::ResourceTextObject(const ResourceMetadata & resourceMetadata,
                                       const QByteArray & resourceBinaryData) :
    m_resourceMetadata(resourceMetadata),
    m_resourceImage()
{
    if (m_resourceMetadata.mimeType().contains("image")) {
        m_resourceImage.loadFromData(resourceBinaryData);
    }
}

ResourceTextObject::ResourceTextObject(const ResourceTextObject & other) :
    m_resourceMetadata(other.m_resourceMetadata),
    m_resourceImage(other.m_resourceImage)
{}

ResourceTextObject & ResourceTextObject::operator =(const ResourceTextObject & other)
{
    if (this != &other) {
        m_resourceMetadata = other.m_resourceMetadata;
        m_resourceImage = other.m_resourceImage;
    }

    return *this;
}

ResourceTextObject::~ResourceTextObject()
{}

bool ResourceTextObject::isValid() const
{
    // TODO: check whether I can obtain any other useful information from the resource
    if (!m_resourceMetadata.isEmpty()) {
        return true;
    }
    else {
        return false;
    }
}

void ResourceTextObject::drawObject(QPainter * pPainter, const QRectF & rect,
                                    QTextDocument * /* pDoc */, int /* positionInDocument */,
                                    const QTextFormat & /* format */)
{
    if (m_resourceMetadata.isEmpty()) {
        return;
    }

    if (!m_resourceImage.isNull())
    {
        QUTE_NOTE_CHECK_PTR(pPainter, "Pointer to QPainter is null when trying to draw ResourceTextObject");
        pPainter->drawImage(rect, m_resourceImage);
    }

    // TODO: add some default drawing algorithm for non-image resources: name and type, for example
}

QSizeF ResourceTextObject::intrinsicSize(QTextDocument * /* pDoc */, int /* positionInDocument */,
                                         const QTextFormat & /* format */)
{
    if (m_resourceMetadata.isEmpty()) {
        return QSizeF();
    }

    int width  = static_cast<int>(m_resourceMetadata.width());
    int height = static_cast<int>(m_resourceMetadata.height());
    return QSizeF(QSize(width, height));
}

