/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#include "Initialize.h"
#include "SetupApplicationIcon.h"
#include "SetupTranslations.h"
#include "SetupStartAtLogin.h"
#include "ParseStartupAccount.h"

#include <lib/account/AccountManager.h>
#include <lib/preferences/SettingsNames.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/Utility.h>
#include <quentier/utility/QuentierApplication.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/VersionInfo.h>

#include <QFileInfo>

#ifdef BUILDING_WITH_BREAKPAD
#include "breakpad/BreakpadIntegration.h"
#endif

namespace quentier {

void composeCommonAvailableCommandLineOptions(
    QHash<QString,CommandLineParser::CommandLineOptionData> & availableCmdOptions)
{
    typedef CommandLineParser::CommandLineOptionData OptionData;
    typedef CommandLineParser::CommandLineArgumentType ArgumentType;

    OptionData & helpData = availableCmdOptions[QStringLiteral("help")];
    helpData.m_singleLetterKey = QChar::fromLatin1('h');
    helpData.m_description = QStringLiteral("show help message");
    helpData.m_type = ArgumentType::None;

    OptionData & versionData = availableCmdOptions[QStringLiteral("version")];
    versionData.m_singleLetterKey = QChar::fromLatin1('v');
    versionData.m_description = QStringLiteral("show version info");
    versionData.m_type = ArgumentType::None;

    OptionData & storageDirData = availableCmdOptions[QStringLiteral("storageDir")];
    storageDirData.m_description =
        QStringLiteral("set directory with the app's persistence");
    storageDirData.m_type = ArgumentType::String;

    OptionData & accountData = availableCmdOptions[QStringLiteral("account")];
    accountData.m_description = QStringLiteral(
        "set the account to use by default:\n"
        "local_<Name>\n"
        "evernote_<id>_<Name>\n"
        "evernotesandbox_<id>_<Name>\n"
        "yinxiangbiji_<id>_<Name>\n"
        "where <id> is user ID and <Name> is the account name");
    accountData.m_type = ArgumentType::String;
}

void parseCommandLine(
    int argc, char *argv[],
    const QHash<QString,CommandLineParser::CommandLineOptionData> & availableCmdOptions,
    ParseCommandLineResult & result)
{
    quentier::CommandLineParser cmdParser(argc, argv, availableCmdOptions);

    result.m_shouldQuit = cmdParser.shouldQuit();

    if (cmdParser.hasError()) {
        result.m_errorDescription = cmdParser.errorDescription();
    }
    else {
        result.m_responseMessage = cmdParser.responseMessage();
        result.m_cmdOptions = cmdParser.options();
    }
}

LogLevel::type processLogLevelCommandLineOption(
    const CommandLineParser::CommandLineOptions & options)
{
    auto logLevelIt = options.find(QStringLiteral("logLevel"));
    if (logLevelIt == options.end()) {
        return LogLevel::InfoLevel;
    }

    QString level = logLevelIt->toString().toLower();

    if (level == QStringLiteral("error")) {
        return LogLevel::ErrorLevel;
    }

    if (level == QStringLiteral("warning")) {
        return LogLevel::WarnLevel;
    }

    if (level == QStringLiteral("debug")) {
        return LogLevel::DebugLevel;
    }

    if (level == QStringLiteral("trace")) {
        return LogLevel::TraceLevel;
    }

    return LogLevel::InfoLevel;
}

bool initialize(
    QuentierApplication & app,
    const CommandLineParser::CommandLineOptions & cmdOptions)
{
    // NOTE: need to check for "storageDir" command line option first, before
    // doing any other part of initialization routine because this option affects
    // the path to Quentier's persistent storage folder
    if (!processStorageDirCommandLineOption(cmdOptions)) {
        return false;
    }

    LogLevel::type logLevel = processLogLevelCommandLineOption(cmdOptions);

    // Initialize logging
    QUENTIER_INITIALIZE_LOGGING();
    QuentierSetMinLogLevel(logLevel);
    QUENTIER_ADD_STDOUT_LOG_DESTINATION();

#ifdef BUILDING_WITH_BREAKPAD
    if (libquentierUsesQtWebEngine()) {
        setQtWebEngineFlags();
    }

    setupBreakpad(app);
#endif

    initializeLibquentier();
    setupApplicationIcon(app);
    setupTranslations(app);

    // Restore the last active min log level
    ApplicationSettings appSettings;
    appSettings.beginGroup(LOGGING_SETTINGS_GROUP);
    if (appSettings.contains(CURRENT_MIN_LOG_LEVEL))
    {
        bool conversionResult = false;
        int minLogLevel = appSettings.value(CURRENT_MIN_LOG_LEVEL)
                          .toInt(&conversionResult);
        if (conversionResult && (0 <= minLogLevel) && (minLogLevel < 6))
        {
            quentier::QuentierSetMinLogLevel(
                static_cast<quentier::LogLevel::type>(minLogLevel));
        }
    }

    setupStartQuentierAtLogin();

    QScopedPointer<Account> pStartupAccount;
    if (!processAccountCommandLineOption(cmdOptions, pStartupAccount)) {
        return false;
    }

    return processOverrideSystemTrayAvailabilityCommandLineOption(cmdOptions);
}

bool processStorageDirCommandLineOption(
    const CommandLineParser::CommandLineOptions & options)
{
    auto storageDirIt = options.find(QStringLiteral("storageDir"));
    if (storageDirIt == options.constEnd()) {
        return true;
    }

    QString storageDir = storageDirIt.value().toString();
    QFileInfo storageDirInfo(storageDir);
    if (!storageDirInfo.exists())
    {
        QDir dir(storageDir);
        if (!dir.mkpath(storageDir))
        {
            criticalMessageBox(Q_NULLPTR,
                               QCoreApplication::applicationName() +
                               QStringLiteral(" ") + QObject::tr("cannot start"),
                               QObject::tr("Cannot create directory for persistent "
                                           "storage pointed to by \"storageDir\" "
                                           "command line option"),
                               QDir::toNativeSeparators(storageDir));
            return false;
        }
    }
    else if (Q_UNLIKELY(!storageDirInfo.isDir()))
    {
        criticalMessageBox(Q_NULLPTR,
                           QCoreApplication::applicationName() +
                           QStringLiteral(" ") + QObject::tr("cannot start"),
                           QObject::tr("\"storageDir\" command line option "
                                       "doesn't point to a directory"),
                           QDir::toNativeSeparators(storageDir));
        return false;
    }
    else if (Q_UNLIKELY(!storageDirInfo.isReadable()))
    {
        criticalMessageBox(Q_NULLPTR,
                           QCoreApplication::applicationName() +
                           QStringLiteral(" ") + QObject::tr("cannot start"),
                           QObject::tr("The directory for persistent storage "
                                       "pointed to by \"storageDir\" command "
                                       "line option is not readable"),
                           QDir::toNativeSeparators(storageDir));
        return false;
    }
    else if (Q_UNLIKELY(!storageDirInfo.isWritable()))
    {
        criticalMessageBox(Q_NULLPTR,
                           QCoreApplication::applicationName() +
                           QStringLiteral(" ") + QObject::tr("cannot start"),
                           QObject::tr("The directory for persistent storage "
                                       "pointed to by \"storageDir\" command "
                                       "line option is not writable"),
                           QDir::toNativeSeparators(storageDir));
        return false;
    }

    qputenv(LIBQUENTIER_PERSISTENCE_STORAGE_PATH, storageDir.toLocal8Bit());
    return true;
}

bool processAccountCommandLineOption(
    const CommandLineParser::CommandLineOptions & options,
    QScopedPointer<Account> & pStartupAccount)
{
    auto accountIt = options.find(QStringLiteral("account"));
    if (accountIt == options.constEnd()) {
        return true;
    }

    QString accountStr = accountIt.value().toString();

    bool isLocal = false;
    qevercloud::UserID userId = -1;
    QString evernoteHost;
    QString accountName;

    ErrorString errorDescription;
    bool res = parseStartupAccount(accountStr, isLocal, userId, evernoteHost,
                                   accountName, errorDescription);
    if (!res) {
        criticalMessageBox(Q_NULLPTR,
                           QCoreApplication::applicationName() +
                           QStringLiteral(" ") + QObject::tr("cannot start"),
                           QObject::tr("Unable to parse the startup account"),
                           errorDescription.localizedString());
        return false;
    }

    bool foundAccount = false;
    Account::EvernoteAccountType::type evernoteAccountType =
        Account::EvernoteAccountType::Free;

    AccountManager accountManager;
    const QVector<Account> & availableAccounts =
        accountManager.availableAccounts();
    for(int i = 0, numAvailableAccounts = availableAccounts.size();
        i < numAvailableAccounts; ++i)
    {
        const Account & availableAccount = availableAccounts.at(i);
        if (isLocal != (availableAccount.type() == Account::Type::Local)) {
            continue;
        }

        if (availableAccount.name() != accountName) {
            continue;
        }

        if (!isLocal && (availableAccount.evernoteHost() != evernoteHost)) {
            continue;
        }

        if (!isLocal && (availableAccount.id() != userId)) {
            continue;
        }

        pStartupAccount.reset(new Account(availableAccount));
        foundAccount = true;
        if (!isLocal) {
            evernoteAccountType = availableAccount.evernoteAccountType();
        }
        break;
    }

    if (!foundAccount)
    {
        criticalMessageBox(Q_NULLPTR,
                           QCoreApplication::applicationName() +
                           QStringLiteral(" ") + QObject::tr("cannot start"),
                           QObject::tr("Wrong startup account"),
                           QObject::tr("The startup account specified on "
                                       "the command line does not correspond "
                                       "to any already existing account"));
        return false;
    }

    qputenv(ACCOUNT_NAME_ENV_VAR, accountName.toLocal8Bit());
    qputenv(ACCOUNT_TYPE_ENV_VAR, (isLocal
                                   ? QByteArray("1")
                                   : QByteArray("0")));
    qputenv(ACCOUNT_ID_ENV_VAR, QByteArray::number(userId));
    qputenv(ACCOUNT_EVERNOTE_ACCOUNT_TYPE_ENV_VAR,
            QByteArray::number(evernoteAccountType));
    qputenv(ACCOUNT_EVERNOTE_HOST_ENV_VAR, evernoteHost.toLocal8Bit());

    return true;
}

bool processOverrideSystemTrayAvailabilityCommandLineOption(
    const CommandLineParser::CommandLineOptions & options)
{
    auto it = options.find(QStringLiteral("overrideSystemTrayAvailability"));
    if (it != options.constEnd())
    {
        bool value = it.value().toBool();
        qputenv(OVERRIDE_SYSTEM_TRAY_AVAILABILITY_ENV_VAR,
                (value ? QByteArray("1") : QByteArray("0")));
    }

    return true;
}

} // namespace quentier
