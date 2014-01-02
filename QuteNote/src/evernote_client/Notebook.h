#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H

#include "../tools/TypeWithError.h"
#include "SynchronizedDataElement.h"
#include <ctime>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(INoteStore)
QT_FORWARD_DECLARE_CLASS(NotebookPrivate)

class Notebook final: public TypeWithError,
                      public SynchronizedDataElement
{
public:
    static Notebook Create(const QString & name, INoteStore & noteStore);

    Notebook();
    Notebook(const QString & name);
    Notebook(const Notebook & other);
    Notebook(Notebook && other);
    Notebook & operator=(const Notebook & other);
    Notebook & operator=(Notebook && other);
    virtual ~Notebook() override;

    virtual void Clear() override;
    virtual bool isEmpty() const override;

    Q_ENUMS(NoteSortOrder)
    Q_ENUMS(PublicNoteSortOrder)

    enum class NoteSortOrder
    {
        NOTE_SORT_ORDER_BY_CREATION_TIMESTAMP,
        NOTE_SORT_ORDER_BY_MODIFICATION_TIMESTAMP,
        NOTE_SORT_ORDER_BY_RELEVANCE,
        NOTE_SORT_ORDER_BY_UPDATE_SEQUENCE_NUMBER,
        NOTE_SORT_ORDER_BY_TITLE
    };

    enum class PublicNoteSortOrder
    {
        PUBLIC_NOTE_SORT_ORDER_ASCENDING,
        PUBLIC_NOTE_SORT_ORDER_DESCENDING
    };

    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(bool isDefault READ isDefault WRITE isDefault)
    Q_PROPERTY(time_t creationTimestamp READ creationTimestamp WRITE setCreationTimestamp)
    Q_PROPERTY(time_t modificationTimestamp READ modificationTimestamp WRITE setModificationTimestamp)
    Q_PROPERTY(NoteSortOrder noteSortOrder READ noteSortOrder WRITE setNoteSortOrder)
    Q_PROPERTY(PublicNoteSortOrder publicNoteSortOrder READ publicNoteSortOrder WRITE setPublicNoteSortOrder)
    Q_PROPERTY(QString publicDescription READ publicDescription WRITE setPublicDescription)
    Q_PROPERTY(bool isPublished READ isPublished WRITE setPublished)
    Q_PROPERTY(QString containerName READ containerName WRITE setContainerName)
    Q_PROPERTY(bool isLastUsed READ isLastUsed WRITE setLastUsed)

    /**
     * @return name of the notebook
     */
    const QString name() const;
    void setName(const QString & name);

    bool isDefault() const;
    void setDefault(const bool is_default);

    time_t creationTimestamp() const;
    void setCreationTimestamp();

    time_t modificationTimestamp() const;
    void setModificationTimestamp();

    const NoteSortOrder noteSortOrder() const;
    void setNoteSortOrder(const NoteSortOrder & note_sort_order);

    const PublicNoteSortOrder publicNoteSortOrder() const;
    void setPublicNoteSortOrder(const PublicNoteSortOrder & public_note_sort_order);

    const QString publicDescription() const;
    void setPublicDescription(const QString & publicDescription);

    bool isPublished() const;
    void setPublished(const bool is_published);

    const QString containerName() const;
    void setContainerName(const QString & containerName);

    bool isLastUsed() const;
    void setLastUsed(const bool is_last_used);

    virtual QTextStream & Print(QTextStream & strm) const override;

private:
    QScopedPointer<NotebookPrivate> d_ptr;
    Q_DECLARE_PRIVATE(Notebook)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H
