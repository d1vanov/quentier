#ifndef QUTE_NOTE_TESTS_MANUAL_TESTING_HELPER_H
#define QUTE_NOTE_TESTS_MANUAL_TESTING_HELPER_H

#include <QString>

namespace qute_note {
namespace test {

// Collection of various stuff useful for manual testing
class ManualTestingHelper
{
public:
    static QString noteContentWithEncryption();
    static QString noteContentWithResources();
};

} // namespace test
} // namespace qute_note

#endif // QUTE_NOTE_TESTS_MANUAL_TESTING_HELPER_H

