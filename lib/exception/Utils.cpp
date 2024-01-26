/*
 * Copyright 2024 Dmitry Ivanov
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

#include "Utils.h"

#include <quentier/exception/IQuentierException.h>
#include <quentier/utility/Unreachable.h>

#include <exception>

namespace quentier {

ErrorString exceptionMessage(const QException & e)
{
    try {
        e.raise();
    }
    catch (const IQuentierException & exc) {
        return exc.errorMessage();
    }
    catch (const std::exception & exc) {
        return ErrorString{QString::fromUtf8(exc.what())};
    }
    catch (...) {
        return ErrorString{QT_TRANSLATE_NOOP(
            "exception::Utils",
            "Unknown exception")};
    }

    UNREACHABLE;
}

} // namespace quentier
