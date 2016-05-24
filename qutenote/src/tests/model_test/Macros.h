#ifndef QUTE_NOTE_TESTS_MODEL_TEST_MACROS_H
#define QUTE_NOTE_TESTS_MODEL_TEST_MACROS_H

#define FAIL(text) \
    QNWARNING(text); \
    emit failure(); \
    return

#endif // QUTE_NOTE_TESTS_MODEL_TEST_MACROS_H
