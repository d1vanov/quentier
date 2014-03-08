#include "EnWrappers.h"
#include "Utility.h"
#include <Limits_constants.h>
#include <QObject>

namespace qute_note {



bool Notebook::CheckParameters(QString & errorDescription) const
{
    if (!en_notebook.__isset.guid) {
        errorDescription = QObject::tr("Notebook's guid is not set");
        return false;
    }
    else if (!CheckGuid(en_notebook.guid)) {
        errorDescription = QObject::tr("Notebook's guid is invalid");
        return false;
    }

    if (!en_notebook.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Notebook's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(en_notebook.updateSequenceNum)) {
        errorDescription = QObject::tr("Notebook's update sequence number is invalid");
        return false;
    }

    if (!en_notebook.__isset.name) {
        errorDescription = QObject::tr("Notebook's name is not set");
        return false;
    }
    else {
        size_t nameSize = en_notebook.name.size();

        if ( (nameSize < evernote::limits::g_Limits_constants.EDAM_NOTEBOOK_NAME_LEN_MIN) ||
             (nameSize > evernote::limits::g_Limits_constants.EDAM_NOTEBOOK_NAME_LEN_MAX) )
        {
            errorDescription = QObject::tr("Notebook's name has invalid size");
            return false;
        }
    }

    if (!en_notebook.__isset.serviceCreated) {
        errorDescription = QObject::tr("Notebook's creation timestamp is not set");
        return false;
    }

    if (!en_notebook.__isset.serviceUpdated) {
        errorDescription = QObject::tr("Notebook's modification timestamp is not set");
        return false;
    }

    if (en_notebook.__isset.sharedNotebooks)
    {
        for(const auto & sharedNotebook: en_notebook.sharedNotebooks)
        {
            if (!sharedNotebook.__isset.id) {
                errorDescription = QObject::tr("Notebook has shared notebook without share id set");
                return false;
            }

            if (!sharedNotebook.__isset.notebookGuid) {
                errorDescription = QObject::tr("Notebook has shared notebook without real notebook's guid set");
                return false;
            }
            else if (!CheckGuid(sharedNotebook.notebookGuid)) {
                errorDescription = QObject::tr("Notebook has shared notebook with invalid guid");
                return false;
            }
        }
    }

    if (en_notebook.__isset.businessNotebook)
    {
        if (!en_notebook.businessNotebook.__isset.notebookDescription) {
            errorDescription = QObject::tr("Description for business notebook is not set");
            return false;
        }
        else {
            size_t businessNotebookDescriptionSize = en_notebook.businessNotebook.notebookDescription.size();

            if ( (businessNotebookDescriptionSize < evernote::limits::g_Limits_constants.EDAM_BUSINESS_NOTEBOOK_DESCRIPTION_LEN_MIN) ||
                 (businessNotebookDescriptionSize > evernote::limits::g_Limits_constants.EDAM_BUSINESS_NOTEBOOK_DESCRIPTION_LEN_MAX) )
            {
                errorDescription = QObject::tr("Description for business notebook has invalid size");
                return false;
            }
        }
    }

    return true;
}

bool Note::CheckParameters(QString & errorDescription) const
{
    if (!en_note.__isset.guid) {
        errorDescription = QObject::tr("Note's guid is not set");
        return false;
    }
    else if (!CheckGuid(en_note.guid)) {
        errorDescription = QObject::tr("Note's guid is invalid");
        return false;
    }

    if (!en_note.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Note's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(en_note.updateSequenceNum)) {
        errorDescription = QObject::tr("Note's update sequence number is invalid");
        return false;
    }

    if (!en_note.__isset.title) {
        errorDescription = QObject::tr("Note's title is not set");
        return false;
    }
    else {
        size_t titleSize = en_note.title.size();

        if ( (titleSize < evernote::limits::g_Limits_constants.EDAM_NOTE_TITLE_LEN_MIN) ||
             (titleSize > evernote::limits::g_Limits_constants.EDAM_NOTE_TITLE_LEN_MAX) )
        {
            errorDescription = QObject::tr("Note's title length is invalid");
            return false;
        }
    }

    if (!en_note.__isset.content) {
        errorDescription = QObject::tr("Note's content is not set");
        return false;
    }
    else {
        size_t contentSize = en_note.content.size();

        if ( (contentSize < evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_LEN_MIN) ||
             (contentSize > evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_LEN_MAX) )
        {
            errorDescription = QObject::tr("Note's content length is invalid");
            return false;
        }
    }

    if (en_note.__isset.contentHash) {
        size_t contentHashSize = en_note.contentHash.size();

        if (contentHashSize != evernote::limits::g_Limits_constants.EDAM_HASH_LEN) {
            errorDescription = QObject::tr("Note's content hash size is invalid");
            return false;
        }
    }

    if (!en_note.__isset.notebookGuid) {
        errorDescription = QObject::tr("Note's notebook Guid is not set");
        return false;
    }
    else if (!CheckGuid(en_note.notebookGuid)) {
        errorDescription = QObject::tr("Note's notebook guid is invalid");
        return false;
    }

    if (en_note.__isset.tagGuids) {
        size_t numTagGuids = en_note.tagGuids.size();

        if (numTagGuids > evernote::limits::g_Limits_constants.EDAM_NOTE_TAGS_MAX) {
            errorDescription = QObject::tr("Note has too many tags, max allowed ");
            errorDescription.append(QString::number(evernote::limits::g_Limits_constants.EDAM_NOTE_TAGS_MAX));
            return false;
        }
    }

    if (en_note.__isset.resources) {
        size_t numResources = en_note.resources.size();

        if (numResources > evernote::limits::g_Limits_constants.EDAM_NOTE_RESOURCES_MAX) {
            errorDescription = QObject::tr("Note has too many resources, max allowed ");
            errorDescription.append(QString::number(evernote::limits::g_Limits_constants.EDAM_NOTE_RESOURCES_MAX));
            return false;
        }
    }

    if (!en_note.__isset.created) {
        errorDescription = QObject::tr("Note's creation timestamp is not set");
        return false;
    }

    if (!en_note.__isset.updated) {
        errorDescription = QObject::tr("Note's modification timestamp is not set");
        return false;
    }

    if (!en_note.__isset.attributes) {
        return true;
    }

    const auto & attributes = en_note.attributes;

#define CHECK_NOTE_ATTRIBUTE(name) \
    if (attributes.__isset.name) { \
        size_t name##Size = attributes.name.size(); \
        \
        if ( (name##Size < evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MIN) || \
             (name##Size > evernote::limits::g_Limits_constants.EDAM_ATTRIBUTE_LEN_MAX) ) \
        { \
            errorDescription = QObject::tr("Note attributes' " #name " field has invalid size"); \
            return false; \
        } \
    }

    CHECK_NOTE_ATTRIBUTE(author);
    CHECK_NOTE_ATTRIBUTE(source);
    CHECK_NOTE_ATTRIBUTE(sourceURL);
    CHECK_NOTE_ATTRIBUTE(sourceApplication);

#undef CHECK_NOTE_ATTRIBUTE

    if (attributes.__isset.contentClass) {
        size_t contentClassSize = attributes.contentClass.size();

        if ( (contentClassSize < evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_CLASS_LEN_MIN) ||
             (contentClassSize > evernote::limits::g_Limits_constants.EDAM_NOTE_CONTENT_CLASS_LEN_MAX) )
        {
            errorDescription = QObject::tr("Note attributes' content class has invalid size");
            return false;
        }
    }

    if (attributes.__isset.applicationData) {
        const auto & applicationData = attributes.applicationData;
        const auto & keysOnly = applicationData.keysOnly;
        const auto & fullMap = applicationData.fullMap;

        for(const auto & key: keysOnly) {
            size_t keySize = key.size();
            if ( (keySize < evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                 (keySize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid key (in keysOnly part)");
                return false;
            }
        }

        for(const auto & pair: fullMap) {
            size_t keySize = pair.first.size();
            if ( (keySize < evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                 (keySize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid key (in fullMap part)");
                return false;
            }

            size_t valueSize = pair.second.size();
            if ( (valueSize < evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_VALUE_LEN_MIN) ||
                 (valueSize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_VALUE_LEN_MAX) )
            {
                errorDescription = QObject::tr("Note's attributes application data has invalid value");
                return false;
            }

            size_t sumSize = keySize + valueSize;
            if (sumSize > evernote::limits::g_Limits_constants.EDAM_APPLICATIONDATA_ENTRY_LEN_MAX) {
                errorDescription = QObject::tr("Note's attributes application data has invalid entry size");
                return false;
            }
        }
    }

    if (attributes.__isset.classifications) {
        const auto & classifications = attributes.classifications;
        for(const auto & pair: classifications) {
            const auto & value = pair.second;
            size_t pos = value.find("CLASSIFICATION_");
            if (pos == std::string::npos || pos != 0) {
                errorDescription = QObject::tr("Note's attributes classifications has invalid classification value");
                return false;
            }
        }
    }

    return true;
}

bool Tag::CheckParameters(QString & errorDescription) const
{
    if (!en_tag.__isset.guid) {
        errorDescription = QObject::tr("Tag's guid is not set");
        return false;
    }
    else if (!CheckGuid(en_tag.guid)) {
        errorDescription = QObject::tr("Tag's guid is invalid: ");
        errorDescription.append(QString::fromStdString(en_tag.guid));
        return false;
    }

    if (!en_tag.__isset.name) {
        errorDescription = QObject::tr("Tag's name is not set");
        return false;
    }
    else {
        size_t nameSize = en_tag.name.size();

        if ( (nameSize < evernote::limits::g_Limits_constants.EDAM_TAG_NAME_LEN_MIN) ||
             (nameSize > evernote::limits::g_Limits_constants.EDAM_TAG_NAME_LEN_MAX) )
        {
            errorDescription = QObject::tr("Tag's name exceeds allowed size: ");
            errorDescription.append(QString::fromStdString(en_tag.name));
            return false;
        }

        if (en_tag.name.at(0) == ' ') {
            errorDescription = QObject::tr("Tag's name can't begin from space: ");
            errorDescription.append(QString::fromStdString(en_tag.name));
            return false;
        }
        else if (en_tag.name.at(nameSize - 1) == ' ') {
            errorDescription = QObject::tr("Tag's name can't end with space: ");
            errorDescription.append(QString::fromStdString(en_tag.name));
            return false;
        }

        if (en_tag.name.find(',') != en_tag.name.npos) {
            errorDescription = QObject::tr("Tag's name can't contain comma: ");
            errorDescription.append(QString::fromStdString(en_tag.name));
            return false;
        }
    }

    if (!en_tag.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Tag's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(en_tag.updateSequenceNum)) {
        errorDescription = QObject::tr("Tag's update sequence number is invalid: ");
        errorDescription.append(en_tag.updateSequenceNum);
        return false;
    }

    if (en_tag.__isset.parentGuid && !CheckGuid(en_tag.parentGuid)) {
        errorDescription = QObject::tr("Tag's parent guid is invalid: ");
        errorDescription.append(QString::fromStdString(en_tag.parentGuid));
        return false;
    }

    return true;
}

bool SavedSearch::CheckParameters(QString &errorDescription) const
{
    if (!en_search.__isset.guid) {
        errorDescription = QObject::tr("Saved search's guid is not set");
        return false;
    }
    else if (!CheckGuid(en_search.guid)) {
        errorDescription = QObject::tr("Saved search's guid is invalid: ");
        errorDescription.append(QString::fromStdString(en_search.guid));
        return false;
    }

    if (!en_search.__isset.name) {
        errorDescription = QObject::tr("Saved search's name is not set");
        return false;
    }
    else {
        size_t nameSize = en_search.name.size();

        if ( (nameSize < evernote::limits::g_Limits_constants.EDAM_SAVED_SEARCH_NAME_LEN_MIN) ||
             (nameSize > evernote::limits::g_Limits_constants.EDAM_SAVED_SEARCH_NAME_LEN_MAX) )
        {
            errorDescription = QObject::tr("Saved search's name exceeds allowed size: ");
            errorDescription.append(QString::fromStdString(en_search.name));
            return false;
        }

        if (en_search.name.at(0) == ' ') {
            errorDescription = QObject::tr("Saved search's name can't begin from space: ");
            errorDescription.append(QString::fromStdString(en_search.name));
            return false;
        }
        else if (en_search.name.at(nameSize - 1) == ' ') {
            errorDescription = QObject::tr("Saved search's name can't end with space: ");
            errorDescription.append(QString::fromStdString(en_search.name));
            return false;
        }
    }

    if (!en_search.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Saved search's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(en_search.updateSequenceNum)) {
        errorDescription = QObject::tr("Saved search's update sequence number is invalid: ");
        errorDescription.append(en_search.updateSequenceNum);
        return false;
    }

    if (!en_search.__isset.query) {
        errorDescription = QObject::tr("Saved search's query is not set");
        return false;
    }
    else {
        size_t querySize = en_search.query.size();

        if ( (querySize < evernote::limits::g_Limits_constants.EDAM_SEARCH_QUERY_LEN_MIN) ||
             (querySize > evernote::limits::g_Limits_constants.EDAM_SEARCH_QUERY_LEN_MAX) )
        {
            errorDescription = QObject::tr("Saved search's query exceeds allowed size: ");
            errorDescription.append(QString::fromStdString(en_search.query));
            return false;
        }
    }

    if (!en_search.__isset.format) {
        errorDescription = QObject::tr("Saved search's format is not set");
        return false;
    }
    else if (en_search.format != evernote::edam::QueryFormat::USER) {
        errorDescription = QObject::tr("Saved search has unsupported query format");
        return false;
    }

    if (!en_search.__isset.scope) {
        errorDescription = QObject::tr("Saved search's scope is not set");
        return false;
    }
    else {
        const auto & scopeIsSet = en_search.scope.__isset;

        if (!scopeIsSet.includeAccount) {
            errorDescription = QObject::tr("Include account option in saved search's scope is not set");
            return false;
        }

        if (!scopeIsSet.includePersonalLinkedNotebooks) {
            errorDescription = QObject::tr("Include personal linked notebooks option in saved search's scope is not set");
            return false;
        }

        if (!scopeIsSet.includeBusinessLinkedNotebooks) {
            errorDescription = QObject::tr("Include business linked notebooks option in saved search's scope is not set");
            return false;
        }
    }

    return true;
}

bool LinkedNotebook::CheckParameters(QString & errorDescription) const
{
    if (!en_linked_notebook.__isset.guid) {
        errorDescription = QObject::tr("Linked notebook's guid is not set");
        return false;
    }
    else if (!CheckGuid(en_linked_notebook.guid)) {
        errorDescription = QObject::tr("Linked notebook's guid is invalid");
        return false;
    }

    if (!en_linked_notebook.__isset.shareName) {
        errorDescription = QObject::tr("Linked notebook's custom name is not set");
        return false;
    }
    else if (en_linked_notebook.shareName.empty()) {
        errorDescription = QObject::tr("Linked notebook's custom name is empty");
        return false;
    }
    else
    {
        QString simplifiedShareName = QString::fromStdString(en_linked_notebook.shareName).replace(" ", "");
        if (simplifiedShareName.isEmpty()) {
            errorDescription = QObject::tr("Linked notebook's custom must contain non-space characters");
            return false;
        }
    }

    return true;
}



}
