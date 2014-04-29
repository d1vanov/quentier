#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__SERIALIZATION_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__SERIALIZATION_H

#include <QtGlobal>

namespace qevercloud {
QT_FORWARD_DECLARE_STRUCT(BusinessUserInfo)
QT_FORWARD_DECLARE_STRUCT(PremiumInfo)
QT_FORWARD_DECLARE_STRUCT(Accounting)
QT_FORWARD_DECLARE_STRUCT(UserAttributes)
QT_FORWARD_DECLARE_STRUCT(NoteAttributes)
QT_FORWARD_DECLARE_STRUCT(ResourceAttributes)
}

namespace evernote {
namespace edam {
QT_FORWARD_DECLARE_CLASS(NoteAttributes)
QT_FORWARD_DECLARE_CLASS(ResourceAttributes)
}
}

QT_FORWARD_DECLARE_CLASS(QByteArray)

namespace qute_note {

const QByteArray GetSerializedBusinessUserInfo(const qevercloud::BusinessUserInfo & info);
const qevercloud::BusinessUserInfo GetDeserializedBusinessUserInfo(const QByteArray & data);

const QByteArray GetSerializedPremiumInfo(const qevercloud::PremiumInfo & info);
const qevercloud::PremiumInfo GetDeserializedPremiumInfo(const QByteArray & data);

const QByteArray GetSerializedAccounting(const qevercloud::Accounting & accounting);
const qevercloud::Accounting GetDeserializedAccounting(const QByteArray & data);

const QByteArray GetSerializedUserAttributes(const qevercloud::UserAttributes & userAttributes);
const qevercloud::UserAttributes GetDeserializedUserAttributes(const QByteArray & data);

const QByteArray GetSerializedNoteAttributes(const evernote::edam::NoteAttributes & noteAttributes);
const evernote::edam::NoteAttributes GetDeserializedNoteAttributes(const QByteArray & data);

const QByteArray GetSerializedResourceAttributes(const evernote::edam::ResourceAttributes & resourceAttributes);
const evernote::edam::ResourceAttributes GetDeserializedResourceAttributes(const QByteArray & data);

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__SERIALIZATION_H
