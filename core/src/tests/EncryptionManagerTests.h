#ifndef __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_TESTS_H
#define __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_TESTS_H

#include <QString>

namespace qute_note {
namespace test {

bool encryptDecryptTest(QString & error);
bool decryptAesTest(QString & error);
bool decryptRc2Test(QString & error);

} // namespace test
} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__ENCRYPTION_MANAGER_TESTS_H
