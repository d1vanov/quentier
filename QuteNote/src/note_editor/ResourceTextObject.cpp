#include "ResourceTextObject.h"
#include "../tools/QuteNoteCheckPtr.h"
#include "../evernote_sdk/src/NoteStore.h"
#include <QPainter>

using namespace evernote::edam;

using namespace qute_note;

ResourceTextObject::ResourceTextObject(const Resource & resource) :
    m_pResource(new Resource(resource)),
    m_resourceImage()
{
    if (resource.__isset.mime && resource.__isset.data) {
        QString mimeType = QString::fromStdString(resource.mime);
        if (mimeType.contains("image/") && resource.data.__isset.body) {
            m_resourceImage.loadFromData(QByteArray(resource.data.body.c_str(), resource.data.body.length()));
        }
    }
}

ResourceTextObject::ResourceTextObject(const ResourceTextObject & other) :
    m_pResource(new Resource(*(other.m_pResource))),
    m_resourceImage(other.m_resourceImage)
{}

ResourceTextObject & ResourceTextObject::operator =(const ResourceTextObject & other)
{
    if (this != &other) {
        *m_pResource = *other.m_pResource;
        m_resourceImage = other.m_resourceImage;
    }

    return *this;
}

ResourceTextObject::~ResourceTextObject()
{}

bool ResourceTextObject::isValid() const
{
    // TODO: check whether I can obtain any other useful information from the resource
    if (m_pResource->data.__isset.body && !m_pResource->data.body.empty()) {
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
    if (!m_pResource->data.__isset.body || m_pResource->data.body.empty()) {
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
    if (!m_pResource->data.__isset.body || m_pResource->data.body.empty()) {
        return QSizeF();
    }

    int width  = static_cast<int>(m_pResource->width);
    int height = static_cast<int>(m_pResource->height);
    return QSizeF(QSize(width, height));
}

