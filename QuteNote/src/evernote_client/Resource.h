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
     * @param name - name of the resource.
     * @param noteGuid - the guid of note to which the resource corresponds
     */
    Resource(const QString & name, const Guid & noteGuid);

    virtual ~Resource() final override;

    /**
     * @brief name - returns the name of the resource
     */
    const QString name() const;

    /**
     * @brief setData - assign binary data to the resource
     * @param resourceBinaryData - binary data to be associated with the resource
     * @param resourceMimeType - optional string representation of mime data of the resource.
     * For example, "text/plain", "text/html" etc.
     * @throw can throw QuteNoteNullPtrException if pointer to implementation is null by occasion
     */
    void setData(const QByteArray & resourceBinaryData,
                 const QString & resourceMimeType = QString());

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
     * @brief mimeData - returns the mime data of the resource
     * @throw can throw QuteNoteNullPtrException if pointer to implementation is null by occasion
     */
    const QMimeData & mimeData() const;

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
