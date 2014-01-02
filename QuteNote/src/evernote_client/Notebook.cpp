#include "Notebook.h"
#include "INoteStore.h"
#include "../evernote_client_private/Notebook_p.h"
#include <QDateTime>

namespace qute_note {

Notebook Notebook::Create(const QString & name, INoteStore & noteStore)
{
    Notebook notebook(name);

    if (name.isEmpty()) {
        notebook.SetError("Notebook name is empty");
        return std::move(notebook);
    }

    QString errorDescription;
    Guid notebookGuid = noteStore.CreateNotebookGuid(notebook, errorDescription);
    if (notebookGuid.isEmpty()) {
        notebook.SetError(errorDescription);
    }
    else {
        notebook.assignGuid(notebookGuid);
    }

    return std::move(notebook);
}

Notebook::Notebook() :
    d_ptr(new NotebookPrivate)
{}

Notebook::Notebook(const QString & name) :
    d_ptr(new NotebookPrivate)
{
    d_ptr->m_name = name;
}

Notebook::Notebook(const Notebook & other) :
    d_ptr(new NotebookPrivate(*(other.d_ptr)))
{}

Notebook::Notebook(Notebook && other) :
    SynchronizedDataElement(other)
{
    d_ptr.swap(other.d_ptr);
}

Notebook & Notebook::operator=(const Notebook & other)
{
    if (this != &other) {
        SynchronizedDataElement::operator =(other);
        *d_ptr = *(other.d_ptr);
    }

    return *this;
}

Notebook & Notebook::operator=(Notebook && other)
{
    if (this != &other) {
        SynchronizedDataElement::operator =(other);
        d_ptr.swap(other.d_ptr);
    }

    return *this;
}

Notebook::~Notebook()
{}

void Notebook::Clear()
{
    QString notebookName = name();

    ClearError();
    SynchronizedDataElement::Clear();
    d_ptr.reset(new NotebookPrivate);
    d_ptr->m_name = notebookName;
}

bool Notebook::isEmpty() const
{
    Q_D(const Notebook);
    return (SynchronizedDataElement::isEmpty() || d->m_name.isEmpty());
}

const QString Notebook::name() const
{
    Q_D(const Notebook);
    return d->m_name;
}

time_t Notebook::creationTimestamp() const
{
    Q_D(const Notebook);
    return d->m_creationTimestamp;
}

time_t Notebook::modificationTimestamp() const
{
    Q_D(const Notebook);
    return d->m_modificationTimestamp;
}

bool Notebook::isDefault() const
{
    Q_D(const Notebook);
    return d->m_isDefault;
}

bool Notebook::isLastUsed() const
{
    Q_D(const Notebook);
    return d->m_isLastUsed;
}

QTextStream & Notebook::Print(QTextStream & strm) const
{
    Q_D(const Notebook);
    strm << "Notebook: { \n";
    strm << "name = " << d->m_name << ", \n ";
    strm << "created timestamp = " << d->m_creationTimestamp << ", \n ";
    strm << "updated timestamp = " << d->m_modificationTimestamp << ", \n ";
    strm << "is " << (d->m_isDefault ? " " : "not ") << "default, \n ";
    strm << "is " << (d->m_isLastUsed ? " " : "not ") << "last used \n";
    strm << "}; \n";

    return strm;
}

}


