/*
 * Copyright 2019-2024 Dmitry Ivanov
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

#include "FetchNotes.h"
#include "PrepareAvailableCommandLineOptions.h"
#include "PrepareLocalStorage.h"
#include "PrepareNotebooks.h"
#include "PrepareTags.h"
#include "ProcessNoteOptions.h"
#include "ProcessNotebookOptions.h"
#include "ProcessStartupAccount.h"
#include "ProcessTagOptions.h"

#include <lib/account/AccountManager.h>
#include <lib/initialization/Initialize.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/Initialize.h>
#include <quentier/utility/StandardPaths.h>

#include <QApplication>
#include <QThread>
#include <QTime>

#include <cstdlib>
#include <ctime>
#include <iostream>

using namespace quentier;

int main(int argc, char * argv[])
{
    QApplication app{argc, argv};
    app.setOrganizationName(QStringLiteral("quentier.org"));
    app.setApplicationName(QStringLiteral("wiki2account"));

    QHash<QString, CommandLineParser::OptionData> availableCmdOptions;
    prepareAvailableCommandLineOptions(availableCmdOptions);

    ParseCommandLineResult parseCmdResult;
    parseCommandLine(argc, argv, availableCmdOptions, parseCmdResult);
    if (!parseCmdResult.m_errorDescription.isEmpty()) {
        std::cerr << parseCmdResult.m_errorDescription.nonLocalizedString()
                         .toLocal8Bit()
                         .constData()
                  << std::endl;
        return 1;
    }

    const auto storageDirIt =
        parseCmdResult.m_cmdOptions.find(QStringLiteral("storageDir"));

    if (storageDirIt == parseCmdResult.m_cmdOptions.end()) {
        // Set storageDir to the location of Quentier app's persistence
        app.setApplicationName(QStringLiteral("quentier"));
        QString path = applicationPersistentStoragePath();
        parseCmdResult.m_cmdOptions[QStringLiteral("storageDir")] = path;
        app.setApplicationName(QStringLiteral("wiki2account"));
    }

    if (!processStorageDirCommandLineOption(parseCmdResult.m_cmdOptions)) {
        return 1;
    }

    // Initialize logging
    QUENTIER_INITIALIZE_LOGGING();
    QUENTIER_SET_MIN_LOG_LEVEL(Info);

    initializeLibquentier();

    const Account account = processStartupAccount(parseCmdResult.m_cmdOptions);
    if (account.isEmpty()) {
        return 1;
    }

    QString targetNotebookName;
    quint32 numNewNotebooks = 0;
    if (!processNotebookOptions(
        parseCmdResult.m_cmdOptions, targetNotebookName, numNewNotebooks)) {
        return 1;
    }

    quint32 minTagsPerNote = 0;
    quint32 maxTagsPerNote = 0;
    if (!processTagOptions(
        parseCmdResult.m_cmdOptions, minTagsPerNote, maxTagsPerNote)) {
        return 1;
    }

    quint32 numNotes = 0;
    if (!processNoteOptions(parseCmdResult.m_cmdOptions, numNotes)) {
        return 1;
    }

    const QString accountPersistencePath =
        accountPersistentStoragePath(account);

    const QDir localStorageDir{accountPersistencePath};

    ErrorString errorDescription;
    auto localStorage = prepareLocalStorage(
        account, localStorageDir, errorDescription);

    if (!localStorage) {
        std::cerr
            << errorDescription.nonLocalizedString().toLocal8Bit().constData()
            << std::endl;
        return 1;
    }

    std::cout << "Preparing notebooks..." << std::endl;

    errorDescription.clear();
    auto notebooks = prepareNotebooks(
        targetNotebookName, numNewNotebooks, localStorage, errorDescription);
    if (notebooks.isEmpty()) {
        return 1;
    }

    std::cout << "Done." << std::endl;
    std::cout << "Preparing tags..." << std::endl;

    errorDescription.clear();
    auto tags = prepareTags(
        minTagsPerNote, maxTagsPerNote, localStorage, errorDescription);
    if (tags.isEmpty() && !errorDescription.isEmpty()) {
        return 1;
    }

    std::cout << "Done." << std::endl;
    std::cout << "Fetching notes..." << std::endl;

    if (!fetchNotes(notebooks, tags, minTagsPerNote, numNotes, localStorage)) {
        return 1;
    }

    return 0;
}
