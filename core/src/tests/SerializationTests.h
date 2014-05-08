#ifndef __QUTE_NOTE__CORE__TESTS__SERIALIZATION_TESTS_H
#define __QUTE_NOTE__CORE__TESTS__SERIALIZATION_TESTS_H

#include <QtGlobal>
QT_FORWARD_DECLARE_CLASS(QString)

namespace qute_note {
namespace test {

bool TestAccountingSerialization(QString & errorDescription);

// NOTE: this test is intentionally incomplete because checks for all possible
// combinations of set/unset parameters would run too long. Some parameters are
// are considered set/unset only if some other parameters are set/unset too.
bool TestUserAttributesSerialization(QString & errorDescription);

bool TestNoteAttributesSerialization(QString & errorDescription);

bool TestResourceAttributesSerialization(QString & errorDescription);

}
}

#endif // __QUTE_NOTE__CORE__TESTS__SERIALIZATION_TESTS_H
