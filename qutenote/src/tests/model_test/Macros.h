#ifndef __QUTE_NOTE__TESTS__MODEL_TEST__MACROS_H
#define __QUTE_NOTE__TESTS__MODEL_TEST__MACROS_H

#define FAIL(text) \
    QNWARNING(text); \
    emit failure(); \
    return

#endif // __QUTE_NOTE__TESTS__MODEL_TEST__MACROS_H
