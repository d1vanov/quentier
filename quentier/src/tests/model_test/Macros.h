#ifndef QUENTIER_TESTS_MODEL_TEST_MACROS_H
#define QUENTIER_TESTS_MODEL_TEST_MACROS_H

#define FAIL(text) \
    QNWARNING(text); \
    emit failure(); \
    return

#define CATCH_EXCEPTION() \
    catch(const IQuentierException & exception) { \
        SysInfo & sysInfo = SysInfo::GetSingleton(); \
        QNWARNING("Caught Quentier exception: " + exception.errorMessage() + \
                  ", what: " + QString(exception.what()) + "; stack trace: " + sysInfo.GetStackTrace()); \
    } \
    catch(const std::exception & exception) { \
        SysInfo & sysInfo = SysInfo::GetSingleton(); \
        QNWARNING("Caught std::exception: " + QString(exception.what()) + "; stack trace: " + sysInfo.GetStackTrace()); \
    } \
    catch(...) { \
        SysInfo & sysInfo = SysInfo::GetSingleton(); \
        QNWARNING("Caught some unknown exception; stack trace: " + sysInfo.GetStackTrace()); \
    }

#endif // QUENTIER_TESTS_MODEL_TEST_MACROS_H
