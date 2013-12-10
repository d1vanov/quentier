#include "Notebook.h"
#include <QDateTime>

namespace qute_note {

class NotebookPrivate
{
public:
    NotebookPrivate();
    NotebookPrivate(const NotebookPrivate & other);
    NotebookPrivate & operator=(const NotebookPrivate & other);

    QString m_name;
    time_t  m_createdTimestamp;
    time_t  m_updatedTimestamp;
    bool    m_isDefault;
    bool    m_isLastUsed;

private:
    void initializeTimestamps();
    void updateTimestamp();
};

Notebook::Notebook() :
    d_ptr(new NotebookPrivate)
{}

Notebook::Notebook(const Notebook & other) :
    d_ptr(((other.d_func() !=nullptr)
           ? new NotebookPrivate(*(other.d_func()))
           : nullptr))
{}

Notebook & Notebook::operator=(const Notebook & other)
{
    if (this != &other) {
        if (other.d_func() != nullptr) {
            *(d_func()) = *(other.d_func());
        }
    }

    return *this;
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

time_t Notebook::createdTimestamp() const
{
    Q_D(const Notebook);
    return d->m_createdTimestamp;
}

time_t Notebook::updatedTimestamp() const
{
    Q_D(const Notebook);
    return d->m_updatedTimestamp;
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

NotebookPrivate::NotebookPrivate()
{
    initializeTimestamps();
}

NotebookPrivate::NotebookPrivate(const NotebookPrivate & other) :
    m_name(other.m_name),
    m_createdTimestamp(other.m_createdTimestamp),
    m_updatedTimestamp(other.m_updatedTimestamp),
    m_isDefault(other.m_isDefault),
    m_isLastUsed(other.m_isLastUsed)
{}

NotebookPrivate & NotebookPrivate::operator=(const NotebookPrivate & other)
{
    if (this != &other) {
        m_name = other.m_name;
        m_createdTimestamp = other.m_createdTimestamp;
        m_isDefault = other.m_isDefault;
        m_isLastUsed = other.m_isLastUsed;
    }

    updateTimestamp();

    return *this;
}

void NotebookPrivate::initializeTimestamps()
{
    m_createdTimestamp = static_cast<time_t>(QDateTime::currentMSecsSinceEpoch());
    m_updatedTimestamp = m_createdTimestamp;
}

void NotebookPrivate::updateTimestamp()
{
    m_updatedTimestamp = static_cast<time_t>(QDateTime::currentMSecsSinceEpoch());
}

}


