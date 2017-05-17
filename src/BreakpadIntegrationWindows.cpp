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
#include <client/windows/handler/exception_handler.h>
#include <windows.h>

namespace breakpad_settings {

bool ShowDumpResults(const wchar_t* dump_path,
                     const wchar_t* minidump_id,
                     void* context,
                     EXCEPTION_POINTERS* exinfo,
                     MDRawAssertionInfo* assertion,
                     bool succeeded)
{
    printf("Dump path: %s\n", dump_path);
    return succeeded;
}

} // namespace breakpad_settings

namespace quentier {

static google_breakpad::ExceptionHandler * pBreakpadHandler = NULL;

void setupBreakpad()
{
    using namespace breakpad_settings;

    pBreakpadHandler = new google_breakpad::ExceptionHandler(L"C:\\dumps\\",
                                                             NULL,
                                                             ShowDumpResults,
                                                             NULL,
                                                             google_breakpad::ExceptionHandler::HANDLER_ALL,
                                                             google_breakpad::MiniDumpNormal,
                                                             NULL,
                                                             NULL);
}

} // namespace quentier
