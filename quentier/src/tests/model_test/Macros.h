/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUENTIER_TESTS_MODEL_TEST_MACROS_H
#define QUENTIER_TESTS_MODEL_TEST_MACROS_H

#include <quentier/utility/Qt4Helper.h>

#define FAIL(text) \
    QNWARNING(text); \
    emit failure(); \
    return

#define CATCH_EXCEPTION() \
    catch(const IQuentierException & exception) { \
        SysInfo sysInfo; \
        QNWARNING(QStringLiteral("Caught Quentier exception: ") + exception.nonLocalizedErrorMessage() + \
                  QStringLiteral(", what: ") + QString(exception.what()) + QStringLiteral("; stack trace: ") + sysInfo.stackTrace()); \
    } \
    catch(const std::exception & exception) { \
        SysInfo sysInfo; \
        QNWARNING(QStringLiteral("Caught std::exception: ") + QString(exception.what()) + QStringLiteral("; stack trace: ") + sysInfo.stackTrace()); \
    } \
    catch(...) { \
        SysInfo sysInfo; \
        QNWARNING(QStringLiteral("Caught some unknown exception; stack trace: ") + sysInfo.stackTrace()); \
    }

#endif // QUENTIER_TESTS_MODEL_TEST_MACROS_H
