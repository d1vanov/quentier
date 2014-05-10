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

const QByteArray GetSerializedResourceAttributes(const qevercloud::ResourceAttributes & resourceAttributes);
const qevercloud::ResourceAttributes GetDeserializedResourceAttributes(const QByteArray & data);

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__SERIALIZATION_H
