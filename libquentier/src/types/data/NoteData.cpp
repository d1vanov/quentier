/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NoteData.h"
#include <quentier/types/Note.h>
#include <quentier/utility/Utility.h>
#include <quentier/enml/ENMLConverter.h>
#include <quentier/logging/QuentierLogger.h>
#include <QXmlStreamReader>
#include <QMutexLocker>

namespace quentier {

NoteData::NoteData() :
    FavoritableDataElementData(),
    m_qecNote(),
    m_resourcesAdditionalInfo(),
    m_notebookLocalUid(),
    m_tagLocalUids(),
    m_thumbnail()
{}

NoteData::NoteData(const NoteData & other) :
    FavoritableDataElementData(other),
    m_qecNote(other.m_qecNote),
    m_resourcesAdditionalInfo(other.m_resourcesAdditionalInfo),
    m_notebookLocalUid(other.m_notebookLocalUid),
    m_tagLocalUids(other.m_tagLocalUids),
    m_thumbnail(other.m_thumbnail)
{}

NoteData::NoteData(NoteData && other) :
    FavoritableDataElementData(std::move(other)),
    m_qecNote(std::move(other.m_qecNote)),
    m_resourcesAdditionalInfo(std::move(other.m_resourcesAdditionalInfo)),
    m_notebookLocalUid(std::move(other.m_notebookLocalUid)),
    m_tagLocalUids(std::move(other.m_tagLocalUids)),
    m_thumbnail(std::move(other.m_thumbnail))
{}

NoteData::~NoteData()
{}

NoteData::NoteData(const qevercloud::Note & other) :
    FavoritableDataElementData(),
    m_qecNote(other),
    m_resourcesAdditionalInfo(),
    m_notebookLocalUid(),
    m_tagLocalUids(),
    m_thumbnail()
{}

bool NoteData::ResourceAdditionalInfo::operator==(const NoteData::ResourceAdditionalInfo & other) const
{
    return (localUid == other.localUid) &&
            (isDirty == other.isDirty);
}

bool NoteData::containsToDoImpl(const bool checked) const
{
    if (!m_qecNote.content.isSet()) {
        return false;
    }

    QXmlStreamReader reader(m_qecNote.content.ref());

    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (reader.isStartElement() && (reader.name() == QStringLiteral("en-todo")))
        {
            const QXmlStreamAttributes attributes = reader.attributes();
            if (checked && attributes.hasAttribute(QStringLiteral("checked")) && (attributes.value(QStringLiteral("checked")) == QStringLiteral("true"))) {
                return true;
            }

            if ( !checked &&
                 (!attributes.hasAttribute(QStringLiteral("checked")) || (attributes.value(QStringLiteral("checked")) == QStringLiteral("false"))) )
            {
                return true;
            }
        }
    }

    return false;
}

bool NoteData::containsEncryption() const
{
    if (!m_qecNote.content.isSet()) {
        return false;
    }

    QXmlStreamReader reader(m_qecNote.content.ref());
    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (reader.isStartElement() && (reader.name() == QStringLiteral("en-crypt"))) {
            return true;
        }
    }

    return false;
}

void NoteData::setContent(const QString & content)
{
    if (!content.isEmpty()) {
        m_qecNote.content = content;
    }
    else {
        m_qecNote.content.clear();
    }
}

void NoteData::clear()
{
    m_qecNote = qevercloud::Note();
    m_resourcesAdditionalInfo.clear();
    m_notebookLocalUid.clear();
    m_tagLocalUids.clear();
    m_thumbnail = QImage();
}

bool NoteData::checkParameters(QNLocalizedString & errorDescription) const
{
    if (m_qecNote.guid.isSet() && !checkGuid(m_qecNote.guid.ref())) {
        errorDescription = QT_TR_NOOP("Note's guid is invalid");
        return false;
    }

    if (m_qecNote.updateSequenceNum.isSet() && !checkUpdateSequenceNumber(m_qecNote.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("Note's update sequence number is invalid");
        return false;
    }

    if (m_qecNote.title.isSet())
    {
        bool res = Note::validateTitle(m_qecNote.title.ref(), &errorDescription);
        if (!res) {
            return false;
        }
    }

    if (m_qecNote.content.isSet())
    {
        int contentSize = m_qecNote.content->size();

        if ( (contentSize < qevercloud::EDAM_NOTE_CONTENT_LEN_MIN) ||
             (contentSize > qevercloud::EDAM_NOTE_CONTENT_LEN_MAX) )
        {
            errorDescription = QT_TR_NOOP("Note's content length is invalid");
            return false;
        }
    }

    if (m_qecNote.contentHash.isSet())
    {
        int contentHashSize = m_qecNote.contentHash->size();

        if (contentHashSize != qevercloud::EDAM_HASH_LEN) {
            errorDescription = QT_TR_NOOP("Note's content hash size is invalid");
            return false;
        }
    }

    if (m_qecNote.notebookGuid.isSet() && !checkGuid(m_qecNote.notebookGuid.ref())) {
        errorDescription = QT_TR_NOOP("Note's notebook guid is invalid");
        return false;
    }

    if (m_qecNote.tagGuids.isSet())
    {
        int numTagGuids = m_qecNote.tagGuids->size();

        if (numTagGuids > qevercloud::EDAM_NOTE_TAGS_MAX) {
            errorDescription = QT_TR_NOOP("Note has too many tags, max allowed ");
            errorDescription += QString::number(qevercloud::EDAM_NOTE_TAGS_MAX);
            return false;
        }
    }

    if (m_qecNote.resources.isSet())
    {
        int numResources = m_qecNote.resources->size();

        if (numResources > qevercloud::EDAM_NOTE_RESOURCES_MAX) {
            errorDescription = QT_TR_NOOP("Note has too many resources, max allowed ");
            errorDescription += QString::number(qevercloud::EDAM_NOTE_RESOURCES_MAX);
            return false;
        }
    }


    if (m_qecNote.attributes.isSet())
    {
        const qevercloud::NoteAttributes & attributes = m_qecNote.attributes;

#define CHECK_NOTE_ATTRIBUTE(name) \
    if (attributes.name.isSet()) { \
        int name##Size = attributes.name->size(); \
        \
        if ( (name##Size < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) || \
             (name##Size > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ) \
        { \
            errorDescription = QT_TR_NOOP("Note attributes field has invalid size"); \
            errorDescription += QStringLiteral(": "); \
            errorDescription += QStringLiteral( #name ); \
            return false; \
        } \
    }

        CHECK_NOTE_ATTRIBUTE(author);
        CHECK_NOTE_ATTRIBUTE(source);
        CHECK_NOTE_ATTRIBUTE(sourceURL);
        CHECK_NOTE_ATTRIBUTE(sourceApplication);

#undef CHECK_NOTE_ATTRIBUTE

        if (attributes.contentClass.isSet())
        {
            int contentClassSize = attributes.contentClass->size();
            if ( (contentClassSize < qevercloud::EDAM_NOTE_CONTENT_CLASS_LEN_MIN) ||
                 (contentClassSize > qevercloud::EDAM_NOTE_CONTENT_CLASS_LEN_MAX) )
            {
                errorDescription = QT_TR_NOOP("Note attributes' content class has invalid size");
                return false;
            }
        }

        if (attributes.applicationData.isSet())
        {
            const qevercloud::LazyMap & applicationData = attributes.applicationData;

            if (applicationData.keysOnly.isSet())
            {
                const QSet<QString> & keysOnly = applicationData.keysOnly;
                foreach(const QString & key, keysOnly) {
                    int keySize = key.size();
                    if ( (keySize < qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                         (keySize > qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
                    {
                        errorDescription = QT_TR_NOOP("Note's attributes application data has invalid key (in keysOnly part)");
                        return false;
                    }
                }
            }

            if (applicationData.fullMap.isSet())
            {
                const QMap<QString, QString> & fullMap = applicationData.fullMap;
                for(QMap<QString, QString>::const_iterator it = fullMap.constBegin(); it != fullMap.constEnd(); ++it)
                {
                    int keySize = it.key().size();
                    if ( (keySize < qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                         (keySize > qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
                    {
                        errorDescription = QT_TR_NOOP("Note's attributes application data has invalid key (in fullMap part)");
                        return false;
                    }

                    int valueSize = it.value().size();
                    if ( (valueSize < qevercloud::EDAM_APPLICATIONDATA_VALUE_LEN_MIN) ||
                         (valueSize > qevercloud::EDAM_APPLICATIONDATA_VALUE_LEN_MAX) )
                    {
                        errorDescription = QT_TR_NOOP("Note's attributes application data has invalid value");
                        return false;
                    }

                    int sumSize = keySize + valueSize;
                    if (sumSize > qevercloud::EDAM_APPLICATIONDATA_ENTRY_LEN_MAX) {
                        errorDescription = QT_TR_NOOP("Note's attributes application data has invalid entry size");
                        return false;
                    }
                }
            }
        }

        if (attributes.classifications.isSet())
        {
            const QMap<QString, QString> & classifications = attributes.classifications;
            for(QMap<QString, QString>::const_iterator it = classifications.constBegin();
                it != classifications.constEnd(); ++it)
            {
                const QString & value = it.value();
                if (!value.startsWith(QStringLiteral("CLASSIFICATION_"))) {
                    errorDescription = QT_TR_NOOP("Note's attributes classifications has invalid classification value");
                    return false;
                }
            }
        }
    }

    return true;
}

QString NoteData::plainText(QNLocalizedString * pErrorMessage) const
{
    if (!m_qecNote.content.isSet()) {
        if (pErrorMessage) {
            *pErrorMessage = QT_TR_NOOP("Note content is not set");
        }
        return QString();
    }

    QString plainText;
    QNLocalizedString error;
    bool res = ENMLConverter::noteContentToPlainText(m_qecNote.content.ref(),
                                                     plainText, error);
    if (!res) {
        QNWARNING(error);
        if (pErrorMessage) {
            *pErrorMessage = error;
        }
        return QString();
    }

    return plainText;
}

QStringList NoteData::listOfWords(QNLocalizedString * pErrorMessage) const
{
    QStringList result;
    QNLocalizedString error;
    bool res = ENMLConverter::noteContentToListOfWords(m_qecNote.content.ref(),
                                                       result, error);
    if (!res) {
        QNWARNING(error);
        if (pErrorMessage) {
            *pErrorMessage = error;
        }
        return QStringList();
    }

    return result;
}

std::pair<QString, QStringList> NoteData::plainTextAndListOfWords(QNLocalizedString * pErrorMessage) const
{
    std::pair<QString, QStringList> result;

    QNLocalizedString error;
    bool res = ENMLConverter::noteContentToListOfWords(m_qecNote.content.ref(),
                                                       result.second,
                                                       error, &result.first);
    if (!res) {
        QNWARNING(error);
        if (pErrorMessage) {
            *pErrorMessage = error;
        }
        return std::pair<QString, QStringList>();
    }

    return result;
}

} // namespace quentier
