#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_H

#include "types/SynchronizedDataElement.h"
#include <QByteArray>
#include <QString>
#include <cstddef>

namespace qute_note {

class Resource: public SynchronizedDataElement
{
public:
    /**
     * @brief Resource - constructor accepting the guid of note to which the resource corresponds
     * @param noteGuid - the guid of note to which the resource corresponds
     */
    Resource(const Guid & noteGuid);

    /**
     * @brief data - returns the actual binary data corresponding to the resource
     */
    const QByteArray & data() const;

    /**
     * @brief dataHash - returns the MD5-hash of binary data corresponding to the resource
     */
    const QString & dataHash() const;

    /**
     * @brief dataSize - the number of bytes in binary data corresponding to the resource
     */
    int dataSize() const;

    /**
     * @brief noteGuid - returns the guid of the note with which the resource is associated
     */
    const Guid noteGuid() const;

    /**
     * @brief mimeType - returns the mime type of the resource
     * @return
     */
    const QString & mimeType() const;

    /**
     * @brief width - if set, the displayed width of the resource in pixels, zero otherwise.
     */
    const std::size_t width() const;

    /**
     * @brief height - if set, the displayed height of the resource
     */
    const std::size_t height() const;

private:
    Resource() = delete;

};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_H
