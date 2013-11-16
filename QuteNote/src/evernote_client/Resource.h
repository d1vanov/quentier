#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_H

#include "SynchronizedDataElement.h"
#include <QByteArray>
#include <QString>
#include <QMimeData>
#include <cstddef>
#include <memory>

namespace qute_note {

/**
 * @brief The Resource class - interface to the binary resource assigned to a particular note
 * within user's account. NOTE: recognition of binary data is not supported yet.
 */
class Resource: public SynchronizedDataElement
{
public:
    /**
     * @brief Resource - constructor accepting the guid of note to which the resource corresponds
     * and binary data of the resource in the form of QByteArray object
     * @param noteGuid - the guid of note to which the resource corresponds
     * @param resourceBinaryData - binary data to be associated with the resource
     * @param resourceMimeType - optional string representation of mime data of the resource.
     * For example, "text/plain", "text/html" etc.
     */
    Resource(const Guid & noteGuid, const QByteArray & resourceBinaryData,
             const QString & resourceMimeType);

    /**
     * @brief Resource - constructor accepting the guid of note to which the resource corresponds
     * and binary data of the resource in the form of QMimeData
     * @param noteGuid - the guid of note to which the resource corresponds
     * @param resourceMimeData - binary data to be associated with the resource
     */
    Resource(const Guid & noteGuid, const QMimeData & resourceMimeData);

    Resource(const Resource & other);
    Resource & operator=(const Resource & other);
    virtual ~Resource() final override;

    /**
     * @brief dataHash - returns the MD5-hash of binary data corresponding to the resource
     */
    const QString dataHash() const;

    /**
     * @brief dataSize - the number of bytes in binary data corresponding to the resource
     */
    std::size_t dataSize() const;

    /**
     * @brief noteGuid - returns the guid of the note with which the resource is associated
     */
    const Guid noteGuid() const;

    /**
     * @brief mimeData - returns the reference to mime data of the resource
     * @throw can throw QuteNoteNullPtrException if pointer to implementation is null by occasion
     *
     * NOTE: this method actually uses lazy initialization, i.e. the actual binary data
     * exists in memory only after this method has been called at least once. The binary data
     * is stored in SQL database and does not exist in memory unless this method has been called.
     * After it's been called, the binary data extracted from the SQL database is kept in memory
     * until the resource object is destroyed
     */
    const QMimeData & mimeData() const;

    /**
     * @brief mimeType - returns string representation of the mime tye of the resource
     */
    const QString mimeType() const;

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

    class ResourceImpl;
    std::unique_ptr<ResourceImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_H
