/*
 * Copyright 2017 Dmitry Ivanov
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

#include "BreakpadIntegration.h"
#include <client/linux/handler/exception_handler.h>

static bool dumpCallback(const google_breakpad::MinidumpDescriptor & descriptor,
                         void * context, bool succeeded)
{
    printf("Dump path: %s\n", descriptor.path());
    return succeeded;
}

namespace quentier {

static google_breakpad::MinidumpDescriptor * pBreakpadDescriptor = NULL;
static google_breakpad::ExceptionHandler * pBreakpadHandler = NULL;

void setupBreakpad()
{
    pBreakpadDescriptor = new google_breakpad::MinidumpDescriptor("/tmp");
    pBreakpadHandler = new google_breakpad::ExceptionHandler(*pBreakpadDescriptor, NULL, dumpCallback, NULL, true, -1);
}

} // namespace quentier
