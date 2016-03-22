#include <qute_note/utility/UidGenerator.h>

namespace qute_note {

QString UidGenerator::Generate()
{
    QUuid uid = QUuid::createUuid();
    return UidToString(uid);
}

QString UidGenerator::UidToString(const QUuid & uid)
{
    if (uid.isNull()) {
        return QString();
    }

    QString result = uid.toString();
    result.remove(result.size()-1, 1);
    result.remove(0, 1);
    return result;
}

} // namespace qute_note
