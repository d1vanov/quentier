#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__SERIALIZATION_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__SERIALIZATION_H

#include <QtGlobal>

namespace evernote {
namespace edam {
QT_FORWARD_DECLARE_CLASS(BusinessUserInfo)
QT_FORWARD_DECLARE_CLASS(PremiumInfo)
QT_FORWARD_DECLARE_CLASS(Accounting)
QT_FORWARD_DECLARE_CLASS(UserAttributes)
QT_FORWARD_DECLARE_CLASS(NoteAttributes)
QT_FORWARD_DECLARE_CLASS(ResourceAttributes)
}
}

QT_FORWARD_DECLARE_CLASS(QByteArray)

namespace qute_note {

const QByteArray GetSerializedBusinessUserInfo(const evernote::edam::BusinessUserInfo & info);
const evernote::edam::BusinessUserInfo GetDeserializedBusinessUserInfo(const QByteArray & data);

const QByteArray GetSerializedPremiumInfo(const evernote::edam::PremiumInfo & info);
const evernote::edam::PremiumInfo GetDeserializedPremiumInfo(const QByteArray & data);

const QByteArray GetSerializedAccounting(const evernote::edam::Accounting & accounting);
const evernote::edam::Accounting GetDeserializedAccounting(const QByteArray & data);

const QByteArray GetSerializedUserAttributes(const evernote::edam::UserAttributes & userAttributes);
const evernote::edam::UserAttributes GetDeserializedUserAttributes(const QByteArray & data);

const QByteArray GetSerializedNoteAttributes(const evernote::edam::NoteAttributes & noteAttributes);
const evernote::edam::NoteAttributes GetDeserializedNoteAttributes(const QByteArray & data);

const QByteArray GetSerializedResourceAttributes(const evernote::edam::ResourceAttributes & resourceAttributes);
const evernote::edam::ResourceAttributes GetDeserializedResourceAttributes(const QByteArray & data);

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__SERIALIZATION_H
