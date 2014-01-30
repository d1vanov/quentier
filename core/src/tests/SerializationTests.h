#ifndef __QUTE_NOTE__CORE__TESTS__SERIALIZATION_TESTS_H
#define __QUTE_NOTE__CORE__TESTS__SERIALIZATION_TESTS_H

#include <QtGlobal>
QT_FORWARD_DECLARE_CLASS(QString)

namespace qute_note {
namespace test {

bool TestBusinessUserInfoSerialization(QString & errorDescription);

bool TestPremiumInfoSerialization(QString & errorDescription);

bool TestAccountingSerialization(QString & errorDescription);

}
}

#endif // __QUTE_NOTE__CORE__TESTS__SERIALIZATION_TESTS_H
