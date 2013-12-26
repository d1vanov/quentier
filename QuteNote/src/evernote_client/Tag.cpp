#include "Tag.h"
#include "Guid.h"
#include "INoteStore.h"

namespace qute_note {

class TagPrivate
{
public:
    TagPrivate(const QString & name);
    TagPrivate(const QString & name, const Guid & parentGuid);
    bool operator==(const TagPrivate & other) const;

    QString m_name;
    Guid    m_parentGuid;

private:
    TagPrivate() = delete;
    TagPrivate(const TagPrivate & other) = delete;
    TagPrivate & operator=(const TagPrivate & other) = delete;
};

Tag Tag::Create(const QString & name, INoteStore & noteStore, const Tag * parent)
{
    Tag tag(name, parent);

    if (tag.name().isEmpty()) {
        tag.SetError("Name is empty");
        return std::move(tag);
    }

    QString errorDescription;
    Guid tagGuid = noteStore.CreateTagGuid(tag, errorDescription);
    if (tagGuid.isEmpty()) {
        tag.SetError(errorDescription);
    }
    else {
        tag.assignGuid(tagGuid);
    }

    return std::move(tag);
}

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

bool Tag::operator==(const Tag & other) const
{
    if (this == &other) {
        return true;
    }

    Q_D(const Tag);
    if (other.d_func() != nullptr) {
        return *(d_func()) == *(other.d_func());
    }
    else if (d_func() != nullptr) {
        return false;
    }
    else {
        return true;
    }
}

Tag::~Tag()
{}

void Tag::Clear()
{
    ClearError();
    SynchronizedDataElement::Clear();

    Q_D(Tag);
    d->m_name.clear();
    d->m_parentGuid.Clear();
}

bool Tag::isEmpty() const
{
    Q_D(const Tag);
    return (SynchronizedDataElement::isEmpty() || d->m_name.isEmpty());
}

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

Tag::Tag(const QString & name, const Tag * parent) :
    d_ptr((parent == nullptr ? new TagPrivate(name) : new TagPrivate(name, parent->guid())))
{}

TagPrivate::TagPrivate(const QString & name) :
    m_name(name)
{}

TagPrivate::TagPrivate(const QString & name, const Guid & parentGuid) :
    m_name(name),
    m_parentGuid(parentGuid)
{}

bool TagPrivate::operator==(const TagPrivate & other) const
{
    if (m_name != other.m_name) {
        return false;
    }

    if (m_parentGuid.isEmpty() && other.m_parentGuid.isEmpty()) {
        return true;
    }
    else if (m_parentGuid == other.m_parentGuid) {
        return true;
    }
    else {
        return false;
    }
}

}
