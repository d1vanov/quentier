#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H

#include <Types_types.h>
#include <NoteStore.h>
#include <QString>
#include <QDataStream>
#include <QByteArray>

namespace qute_note {

QDataStream & operator<<(QDataStream & out, const evernote::edam::BusinessUserInfo & info);
QDataStream & operator>>(QDataStream & in, evernote::edam::BusinessUserInfo & info);

const QByteArray GetSerializedBusinessUserInfo(const evernote::edam::BusinessUserInfo & info);
const evernote::edam::BusinessUserInfo GetDeserializedBusinessUserInfo(const QByteArray & data);

QDataStream & operator<<(QDataStream & out, const evernote::edam::PremiumInfo & info);
QDataStream & operator>>(QDataStream & in, evernote::edam::PremiumInfo & info);

const QByteArray GetSerializedPremiumInfo(const evernote::edam::PremiumInfo & info);
const evernote::edam::PremiumInfo GetDeserializedPremiumInfo(const QByteArray & data);

QDataStream & operator<<(QDataStream & out, const evernote::edam::Accounting & accounting);
QDataStream & operator>>(QDataStream & in, evernote::edam::Accounting & accounting);

const QByteArray GetSerializedAccounting(const evernote::edam::Accounting & accounting);
const evernote::edam::Accounting GetDeserializedAccounting(const QByteArray & data);

QDataStream & operator<<(QDataStream & out, const evernote::edam::UserAttributes & userAttributes);
QDataStream & operator>>(QDataStream & in, evernote::edam::UserAttributes & userAttributes);

const QByteArray GetSerializedUserAttributes(const evernote::edam::UserAttributes & userAttributes);
const evernote::edam::UserAttributes GetDeserializedUserAttributes(const QByteArray & data);

QDataStream & operator<<(QDataStream & out, const evernote::edam::NoteAttributes & noteAttributes);
QDataStream & operator>>(QDataStream & in, evernote::edam::NoteAttributes & noteAttributes);

const QByteArray GetSerializedNoteAttributes(const evernote::edam::NoteAttributes & noteAttributes);
const evernote::edam::NoteAttributes GetDeserializedNoteAttributes(const QByteArray & data);

QDataStream & operator<<(QDataStream & out, const evernote::edam::ResourceAttributes & resourceAttributes);
QDataStream & operator>>(QDataStream & in, evernote::edam::ResourceAttributes & resourceAttributes);

const QByteArray GetSerializedResourceAttributes(const evernote::edam::ResourceAttributes & resourceAttributes);
const evernote::edam::ResourceAttributes GetDeserializedResourceAttributes(const QByteArray & data);

struct Note
{
    Note() : isDirty(true), isLocal(true), isDeleted(false), en_note() {}

    bool isDirty;
    bool isLocal;
    bool isDeleted;
    evernote::edam::Note en_note;

    bool CheckParameters(QString & errorDescription) const;
};

struct Notebook
{
    Notebook() : isDirty(true), isLocal(true), isLastUsed(false), en_notebook() {}

    bool isDirty;
    bool isLocal;
    bool isLastUsed;
    evernote::edam::Notebook en_notebook;

    bool CheckParameters(QString & errorDescription) const;
};

struct Resource
{
    Resource() : isDirty(true), isLocal(true), en_resource() {}

    bool isDirty;
    bool isLocal;
    evernote::edam::Resource en_resource;

    bool CheckParameters(QString & errorDescription, const bool isFreeAccount = true) const;
};

struct Tag
{
    Tag() : isDirty(true), isLocal(true), isDeleted(false), en_tag() {}

    bool isDirty;
    bool isLocal;
    bool isDeleted;
    evernote::edam::Tag en_tag;

    bool CheckParameters(QString & errorDescription) const;
};

struct User
{
    User() : isDirty(true), isLocal(true), en_user() {}

    bool isDirty;
    bool isLocal;
    evernote::edam::User en_user;
};

typedef evernote::edam::Timestamp Timestamp;
typedef evernote::edam::UserID UserID;
typedef evernote::edam::Guid Guid;

namespace en_wrappers_private {

bool CheckGuid(const Guid & guid);

bool CheckUpdateSequenceNumber(const int32_t updateSequenceNumber);

}

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H
