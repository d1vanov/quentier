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

#ifndef LIB_QUENTIER_TYPES_NOTE_H
#define LIB_QUENTIER_TYPES_NOTE_H

#include "IFavoritableDataElement.h"
#include "SharedNote.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

#include <QImage>
#include <QSharedDataPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(ResourceAdapter)
QT_FORWARD_DECLARE_CLASS(ResourceWrapper)
QT_FORWARD_DECLARE_CLASS(NoteData)

class QUENTIER_EXPORT Note: public IFavoritableDataElement
{
public:
    QN_DECLARE_LOCAL_UID
    QN_DECLARE_DIRTY
    QN_DECLARE_FAVORITED
    QN_DECLARE_LOCAL

public:
    Note();
    Note(const Note & other);
    Note(Note && other);
    Note & operator=(const Note & other);
    Note & operator=(Note && other);

    Note(const qevercloud::Note & other);
    Note & operator=(const qevercloud::Note & other);

    virtual ~Note();

    bool operator==(const Note & other) const;
    bool operator!=(const Note & other) const;

    operator const qevercloud::Note&() const;
    operator qevercloud::Note&();

    virtual bool hasGuid() const Q_DECL_OVERRIDE;
    virtual const QString & guid() const Q_DECL_OVERRIDE;
    virtual void setGuid(const QString & guid) Q_DECL_OVERRIDE;

    virtual bool hasUpdateSequenceNumber() const Q_DECL_OVERRIDE;
    virtual qint32 updateSequenceNumber() const Q_DECL_OVERRIDE;
    virtual void setUpdateSequenceNumber(const qint32 usn) Q_DECL_OVERRIDE;

    virtual void clear() Q_DECL_OVERRIDE;

    virtual bool checkParameters(QNLocalizedString & errorDescription) const Q_DECL_OVERRIDE;

    bool hasTitle() const;
    const QString & title() const;
    void setTitle(const QString & title);

    bool hasContent() const;
    const QString & content() const;
    void setContent(const QString & content);

    bool hasContentHash() const;
    const QByteArray & contentHash() const;
    void setContentHash(const QByteArray & contentHash);

    bool hasContentLength() const;
    qint32 contentLength() const;
    void setContentLength(const qint32 length);

    bool hasCreationTimestamp() const;
    qint64 creationTimestamp() const;
    void setCreationTimestamp(const qint64 timestamp);

    bool hasModificationTimestamp() const;
    qint64 modificationTimestamp() const;
    void setModificationTimestamp(const qint64 timestamp);

    bool hasDeletionTimestamp() const;
    qint64 deletionTimestamp() const;
    void setDeletionTimestamp(const qint64 timestamp);

    bool hasActive() const;
    bool active() const;
    void setActive(const bool active);

    bool hasNotebookGuid() const;
    const QString & notebookGuid() const;
    void setNotebookGuid(const QString & guid);

    bool hasNotebookLocalUid() const;
    const QString & notebookLocalUid() const;
    void setNotebookLocalUid(const QString & notebookLocalUid);

    bool hasTagGuids() const;
    const QStringList tagGuids() const;
    void setTagGuids(const QStringList & guids);
    void addTagGuid(const QString & guid);
    void removeTagGuid(const QString & guid);

    bool hasTagLocalUids() const;
    const QStringList & tagLocalUids() const;
    void setTagLocalUids(const QStringList & localUids);
    void addTagLocalUid(const QString & localUid);
    void removeTagLocalUid(const QString & localUid);

    bool hasResources() const;
    QList<ResourceAdapter> resourceAdapters() const;
    QList<ResourceWrapper> resources() const;
    void setResources(const QList<ResourceAdapter> & resources);
    void setResources(const QList<ResourceWrapper> & resources);
    void addResource(const IResource & resource);
    bool updateResource(const IResource & resource);
    bool removeResource(const IResource & resource);

    bool hasNoteAttributes() const;
    const qevercloud::NoteAttributes & noteAttributes() const;
    qevercloud::NoteAttributes & noteAttributes();

    bool hasSharedNotes() const;
    QList<SharedNote> sharedNotes() const;
    void setSharedNotes(const QList<SharedNote> & sharedNotes);
    void addSharedNote(const SharedNote & sharedNote);

    // NOTE: the shared note is recognized by its index in note in the following two methods
    bool updateSharedNote(const SharedNote & sharedNote);
    bool removeSharedNote(const SharedNote & sharedNote);

    bool hasNoteRestrictions() const;
    const qevercloud::NoteRestrictions & noteRestrictions() const;
    qevercloud::NoteRestrictions & noteRestrictions();

    bool hasNoteLimits() const;
    const qevercloud::NoteLimits & noteLimits() const;
    qevercloud::NoteLimits & noteLimits();

    QImage thumbnail() const;
    void setThumbnail(const QImage & thumbnail);

    bool isInkNote() const;

    QString plainText(QNLocalizedString * pErrorMessage = Q_NULLPTR) const;
    QStringList listOfWords(QNLocalizedString * pErrorMessage = Q_NULLPTR) const;
    std::pair<QString, QStringList> plainTextAndListOfWords(QNLocalizedString * pErrorMessage = Q_NULLPTR) const;

    bool containsCheckedTodo() const;
    bool containsUncheckedTodo() const;
    bool containsTodo() const;
    bool containsEncryption() const;

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<NoteData> d;
};

} // namespace quentier

Q_DECLARE_METATYPE(quentier::Note)

#endif // LIB_QUENTIER_TYPES_NOTE_H
