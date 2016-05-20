#ifndef LIB_QUTE_NOTE_TESTS_ENCRYPTION_MANAGER_TESTS_H
#define LIB_QUTE_NOTE_TESTS_ENCRYPTION_MANAGER_TESTS_H

#include <QString>

namespace qute_note {
namespace test {

bool encryptDecryptTest(QString & error);
bool decryptAesTest(QString & error);
bool decryptRc2Test(QString & error);

} // namespace test
} // namespace qute_note

#endif // LIB_QUTE_NOTE_TESTS_ENCRYPTION_MANAGER_TESTS_H
