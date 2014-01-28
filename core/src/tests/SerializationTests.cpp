#include "SerializationTests.h"
#include "../client/EnWrappers.h"
#include "../tools/Printable.h"
#include <Types_types.h>
#include <bitset>

namespace qute_note {
namespace test {

bool TestBusinessUserInfoSerialization(QString & errorDescription)
{
    evernote::edam::BusinessUserInfo info;
    auto & isSet = info.__isset;

    // there are 4 optional data components of BusinessUserInfo
    for(int mask = 0; mask != (1 << 4); ++mask)
    {
        std::bitset<4> bits(mask);

        isSet.businessId   = bits[0];
        isSet.businessName = bits[1];
        isSet.email = bits[2];
        isSet.role  = bits[3];

        if (isSet.businessId) {
            info.businessId = mask;
        }

        if (isSet.businessName) {
            info.businessName = "Charlie";
        }

        if (isSet.email) {
            info.email = "nevertrustaliens@frustration.com";
        }

        if (isSet.role) {
            info.role = evernote::edam::BusinessUserRole::NORMAL;
        }

        QByteArray serializedInfo = GetSerializedBusinessUserInfo(info);
        evernote::edam::BusinessUserInfo deserializedInfo = GetDeserializedBusinessUserInfo(serializedInfo);

        if (info != deserializedInfo)
        {
            errorDescription = "Serialization test for BusinessUserInfo FAILED! ";
            errorDescription.append("Initial BusinessUserInfo: ");
            errorDescription.append(ToQString<evernote::edam::BusinessUserInfo>(info));
            errorDescription.append("Deserialized BusinessUserInfo: ");
            errorDescription.append(ToQString<evernote::edam::BusinessUserInfo>(deserializedInfo));

            return false;
        }
    }

    return true;
}

}
}
