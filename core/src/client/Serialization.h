#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__SERIALIZATION_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__SERIALIZATION_H

#include <QtGlobal>

namespace qevercloud {
QT_FORWARD_DECLARE_STRUCT(PremiumInfo)
QT_FORWARD_DECLARE_STRUCT(Accounting)
QT_FORWARD_DECLARE_STRUCT(UserAttributes)
QT_FORWARD_DECLARE_STRUCT(NoteAttributes)
QT_FORWARD_DECLARE_STRUCT(ResourceAttributes)
}

QT_FORWARD_DECLARE_CLASS(QByteArray)

namespace qute_note {

const QByteArray GetSerializedPremiumInfo(const qevercloud::PremiumInfo & info);
const qevercloud::PremiumInfo GetDeserializedPremiumInfo(const QByteArray & data);

const QByteArray GetSerializedAccounting(const qevercloud::Accounting & accounting);
const qevercloud::Accounting GetDeserializedAccounting(const QByteArray & data);

const QByteArray GetSerializedUserAttributes(const qevercloud::UserAttributes & userAttributes);
const qevercloud::UserAttributes GetDeserializedUserAttributes(const QByteArray & data);

const QByteArray GetSerializedNoteAttributes(const qevercloud::NoteAttributes & noteAttributes);
const qevercloud::NoteAttributes GetDeserializedNoteAttributes(const QByteArray & data);

const QByteArray GetSerializedResourceAttributes(const qevercloud::ResourceAttributes & resourceAttributes);
const qevercloud::ResourceAttributes GetDeserializedResourceAttributes(const QByteArray & data);

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__SERIALIZATION_H
