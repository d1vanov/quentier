#include "NoteSearchQuery.h"
#include "NoteSearchQuery_p.h"

namespace qute_note {

NoteSearchQuery::NoteSearchQuery() :
    d_ptr(new NoteSearchQueryPrivate)
{}

NoteSearchQuery::~NoteSearchQuery()
{
    if (d_ptr) {
        delete d_ptr;
    }
}

bool NoteSearchQuery::isEmpty() const
{
    Q_D(const NoteSearchQuery);
    return d->m_queryString.isEmpty();
}

const QString NoteSearchQuery::queryString() const
{
    Q_D(const NoteSearchQuery);
    return d->m_queryString;
}

bool NoteSearchQuery::setQueryString(const QString & queryString, QString & error)
{
    Q_D(NoteSearchQuery);
    return d->parseQueryString(queryString, error);
}

const QString NoteSearchQuery::notebookModifier() const
{
    Q_D(const NoteSearchQuery);
    return d->m_notebookModifier;
}

bool NoteSearchQuery::hasAnyModifier() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyModifier;
}

const QStringList & NoteSearchQuery::tagNames() const
{
    Q_D(const NoteSearchQuery);
    return d->m_tagNames;
}

const QStringList & NoteSearchQuery::negatedTagNames() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedTagNames;
}

const QVector<qint64> & NoteSearchQuery::creationTimestamps() const
{
    Q_D(const NoteSearchQuery);
    return d->m_creationTimestamps;
}

const QVector<qint64> & NoteSearchQuery::negatedCreationTimestamps() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedCreationTimestamps;
}

// TODO: continue from here

QTextStream & NoteSearchQuery::Print(QTextStream & strm) const
{
    Q_D(const NoteSearchQuery);
    return d->Print(strm);
}

} // namespace qute_note
