#include "Tag.h"
#include "Guid.h"

namespace qute_note {

class TagPrivate
{
public:
    TagPrivate(const QString & name);
    TagPrivate(const QString & name, const Guid & parentGuid);

    QString m_name;
    Guid    m_parentGuid;

private:
    TagPrivate() = delete;
    TagPrivate(const TagPrivate & other) = delete;
    TagPrivate & operator=(const TagPrivate & other) = delete;
};

Tag::Tag(const QString & name) :
    d_ptr(new TagPrivate(name))
{}

Tag::Tag(const QString &name, const Tag & parent) :
    d_ptr(new TagPrivate(name, parent.guid()))
{}

Tag::Tag(const Tag & other) :
    d_ptr(new TagPrivate(other.name(), other.parentGuid()))
{}

Tag & Tag::operator=(const Tag & other)
{
    if (this != &other)
    {
        Q_D(Tag);
        d->m_name = other.name();
        d->m_parentGuid = other.parentGuid();
    }

    return *this;
}

Tag::~Tag()
{}

const Guid Tag::parentGuid() const
{
    Q_D(const Tag);
    return d->m_parentGuid;
}

const QString Tag::name() const
{
    Q_D(const Tag);
    return d->m_name;
}

void Tag::rename(const QString & name)
{
    Q_D(Tag);
    d->m_name = name;
}

void Tag::setParent(const Tag & parent)
{
    Q_D(Tag);
    d->m_parentGuid = parent.guid();
}

TagPrivate::TagPrivate(const QString & name) :
    m_name(name)
{}

TagPrivate::TagPrivate(const QString & name, const Guid & parentGuid) :
    m_name(name),
    m_parentGuid(parentGuid)
{}

}
