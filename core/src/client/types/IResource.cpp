#include "IResource.h"
#include "../Utility.h"
#include <Limits_constants.h>
#include <QObject>

namespace qute_note {

IResource::IResource() :
    m_isDirty(true)
{}

IResource::~IResource()
{}

void IResource::Clear()
{
    SetDirty();
    GetEnResource() = evernote::edam::Resource();
}

bool IResource::IsDirty() const
{
    return m_isDirty;
}

void IResource::SetDirty()
{
    m_isDirty = true;
}

void IResource::SetClean()
{
    m_isDirty = false;
}

bool IResource::CheckParameters(QString & errorDescription, const bool isFreeAccount) const
{
    const auto & enResource = GetEnResource();

    if (!enResource.__isset.guid) {
        errorDescription = QObject::tr("Resource's guid is not set");
        return false;
    }
    else if (!CheckGuid(enResource.guid)) {
        errorDescription = QObject::tr("Resource's guid is invalid");
        return false;
    }

    if (!enResource.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Resource's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(enResource.updateSequenceNum)) {
        errorDescription = QObject::tr("Resource's update sequence number is invalid");
        return false;
    }

    if (enResource.__isset.noteGuid && !CheckGuid(enResource.noteGuid)) {
        errorDescription = QObject::tr("Resource's note guid is invalid");
        return false;
    }

#define CHECK_RESOURCE_DATA(name) \
    if (enResource.__isset.name) \
    { \
        if (!enResource.name.__isset.body) { \
            errorDescription = QObject::tr("Resource's " #name " body is not set"); \
            return false; \
        } \
        \
        int32_t dataSize = static_cast<int32_t>(enResource.name.body.size()); \
        int32_t allowedSize = (isFreeAccount \
                               ? evernote::limits::g_Limits_constants.EDAM_RESOURCE_SIZE_MAX_FREE \
                               : evernote::limits::g_Limits_constants.EDAM_RESOURCE_SIZE_MAX_PREMIUM); \
        if (dataSize > allowedSize) { \
            errorDescription = QObject::tr("Resource's " #name " body size is too large"); \
            return false; \
        } \
        \
        if (!enResource.name.__isset.size) { \
            errorDescription = QObject::tr("Resource's " #name " size is not set"); \
            return false; \
        } \
        \
        if (!enResource.name.__isset.bodyHash) { \
            errorDescription = QObject::tr("Resource's " #name " hash is not set"); \
            return false; \
        } \
        else { \
            int32_t hashSize = static_cast<int32_t>(enResource.name.bodyHash.size()); \
            if (hashSize != evernote::limits::g_Limits_constants.EDAM_HASH_LEN) { \
                errorDescription = QObject::tr("Invalid " #name " hash size"); \
                return false; \
            } \
        } \
    }

    CHECK_RESOURCE_DATA(data);
    CHECK_RESOURCE_DATA(recognition);
    CHECK_RESOURCE_DATA(alternateData);

#undef CHECK_RESOURCE_DATA

    if (!enResource.__isset.data && enResource.__isset.alternateData) {
        errorDescription = QObject::tr("Resource has no data set but alternate data is present");
        return false;
    }

    if (!enResource.__isset.mime)
    {
        errorDescription = QObject::tr("Resource's mime type is not set");
        return false;
    }
    else
    {
        int32_t mimeSize = static_cast<int32_t>(enResource.mime.size());
        if ( (mimeSize < evernote::limits::g_Limits_constants.EDAM_MIME_LEN_MIN) ||
             (mimeSize > evernote::limits::g_Limits_constants.EDAM_MIME_LEN_MAX) )
        {
            errorDescription = QObject::tr("Resource's mime type has invalid length");
            return false;
        }
    }

    return true;
}

IResource::IResource(const IResource & other) :
    m_isDirty(other.m_isDirty)
{}

}
