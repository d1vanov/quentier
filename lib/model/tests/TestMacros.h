/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#pragma once

#include <QTextStream>

#define FAIL(text)                                                             \
    QString str;                                                               \
    QTextStream strm{&str};                                                    \
    strm << __FILE__ << ":" << __LINE__ << " " << text;                        \
    Q_EMIT failure(ErrorString(str));                                          \
    return // FAIL

#define CATCH_EXCEPTION()                                                      \
    catch (const IQuentierException & exception) {                             \
        SysInfo sysInfo;                                                       \
        QString error = QStringLiteral("Caught Quentier exception: ") +        \
            exception.nonLocalizedErrorMessage() +                             \
            QStringLiteral(", what: ") + QString::fromUtf8(exception.what()) + \
            QStringLiteral("; stack trace: ") + sysInfo.stackTrace();          \
        errorDescription = ErrorString(error);                                 \
    }                                                                          \
    catch (const std::exception & exception) {                                 \
        SysInfo sysInfo;                                                       \
        QString error = QStringLiteral("Caught std::exception: ") +            \
            QString::fromUtf8(exception.what()) +                              \
            QStringLiteral("; stack trace: ") + sysInfo.stackTrace();          \
        errorDescription = ErrorString(error);                                 \
    }                                                                          \
    catch (...) {                                                              \
        SysInfo sysInfo;                                                       \
        QString error = QStringLiteral("Caught some unknown exception; ") +    \
            QStringLiteral("stack trace: ") + sysInfo.stackTrace();            \
        errorDescription = ErrorString(error);                                 \
    }                                                                          \
    // CATCH_EXCEPTION
