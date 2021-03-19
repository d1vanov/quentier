/*
 * Copyright 2017-2021 Dmitry Ivanov
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
#include "ParseStartupAccount.h"
#include "SetupApplicationIcon.h"
#include "SetupStartAtLogin.h"
#include "SetupTranslations.h"

#include <lib/account/AccountManager.h>
#include <lib/preferences/keys/Logging.h>
#include <lib/preferences/keys/SystemTray.h>
#include <lib/utility/HumanReadableVersionInfo.h>
#include <lib/utility/Log.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/Initialize.h>
#include <quentier/utility/MessageBox.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/utility/VersionInfo.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QtGlobal>

#include <memory>

#ifdef BUILDING_WITH_BREAKPAD
#include "breakpad/BreakpadIntegration.h"
#endif

#ifdef Q_OS_WIN
#include <Windows.h>
#include <rtcapi.h>
#endif

namespace quentier {

#ifdef Q_OS_WIN

namespace {

int null_exception_handler(LPEXCEPTION_POINTERS)
{
    exit(1);
}

int null_runtime_check_handler(
    int /* errorType */, const char * /* filename */, int /* linenumber */,
    const char * /* moduleName */, const char * /* format */, ...)
{
    exit(1);
}

} // namespace

#endif

void composeCommonAvailableCommandLineOptions(
    QHash<QString, CommandLineParser::OptionData> & availableCmdOptions)
{
    using OptionData = CommandLineParser::OptionData;
    using ArgumentType = CommandLineParser::ArgumentType;

    OptionData & storageDirData =
        availableCmdOptions[QStringLiteral("storageDir")];

    storageDirData.m_name =
        QCoreApplication::translate("CommandLineParser", "storage dir");

    storageDirData.m_description =
        QStringLiteral("set directory with the app's persistence");

    storageDirData.m_type = ArgumentType::String;

    OptionData & accountData = availableCmdOptions[QStringLiteral("account")];

    accountData.m_name =
        QCoreApplication::translate("CommandLineParser", "account spec");

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
    int argc, char * argv[], // NOLINT
    const QHash<QString, CommandLineParser::OptionData> & availableCmdOptions,
    ParseCommandLineResult & result)
{
    quentier::CommandLineParser cmdParser(argc, argv, availableCmdOptions);

    if (cmdParser.hasError()) {
        result.m_errorDescription = cmdParser.errorDescription();
    }
    else {
        result.m_cmdOptions = cmdParser.options();
    }
}

std::unique_ptr<LogLevel> processLogLevelCommandLineOption(
    const CommandLineParser::Options & options)
{
    const auto logLevelIt = options.find(QStringLiteral("logLevel"));
    if (logLevelIt == options.end()) {
        return {};
    }

    QString level = logLevelIt->toString().toLower();

    if (level == QStringLiteral("error")) {
        return std::make_unique<LogLevel>(LogLevel::Error);
    }

    if (level == QStringLiteral("warning")) {
        return std::make_unique<LogLevel>(LogLevel::Warning);
    }

    if (level == QStringLiteral("debug")) {
        return std::make_unique<LogLevel>(LogLevel::Debug);
    }

    if (level == QStringLiteral("trace")) {
        return std::make_unique<LogLevel>(LogLevel::Trace);
    }

    return {};
}

void initializeAppVersion()
{
    const QString appVersion = QStringLiteral("\n") + quentierVersion() +
        QStringLiteral(", build info: ") + quentierBuildInfo() +
        QStringLiteral("\nBuilt with Qt ") + QStringLiteral(QT_VERSION_STR) +
        QStringLiteral(", uses Qt ") + QString::fromUtf8(qVersion()) +
        QStringLiteral("\nBuilt with libquentier: ") +
        libquentierBuildTimeInfo() + QStringLiteral("\nUses libquentier: ") +
        libquentierRuntimeInfo() + QStringLiteral("\n");

    QCoreApplication::setApplicationVersion(appVersion);
}

bool initialize(const CommandLineParser::Options & cmdOptions)
{
    // NOTE: need to check for "storageDir" command line option first, before
    // doing any other part of initialization routine because this option
    // affects the path to Quentier's persistent storage folder
    if (!processStorageDirCommandLineOption(cmdOptions)) {
        return false;
    }

    const auto pLogLevel = processLogLevelCommandLineOption(cmdOptions);

    // Initialize logging
    QUENTIER_INITIALIZE_LOGGING();
    QuentierSetMinLogLevel(pLogLevel ? *pLogLevel : LogLevel::Info);
    QUENTIER_ADD_STDOUT_LOG_DESTINATION();

    auto logFilterByComponent = restoreLogFilterByComponent();
    QuentierSetLogComponentFilter(QRegularExpression(logFilterByComponent));

#ifdef BUILDING_WITH_BREAKPAD
    if (libquentierUsesQtWebEngine()) {
        setQtWebEngineFlags();
    }

    setupBreakpad();
#endif

    initializeLibquentier();
    setupApplicationIcon();
    setupTranslations();

    if (!pLogLevel) {
        // Log level was not specified on the command line, restore the last
        // active min log level
        ApplicationSettings appSettings;
        appSettings.beginGroup(preferences::keys::loggingGroup);
        if (appSettings.contains(preferences::keys::minLogLevel)) {
            bool conversionResult = false;

            int minLogLevel = appSettings.value(preferences::keys::minLogLevel)
                                  .toInt(&conversionResult);

            if (conversionResult && (0 <= minLogLevel) && (minLogLevel < 6)) {
                quentier::QuentierSetMinLogLevel(
                    static_cast<quentier::LogLevel>(minLogLevel));
            }
        }
    }

    setupStartQuentierAtLogin();

    std::unique_ptr<Account> pStartupAccount;
    if (!processAccountCommandLineOption(cmdOptions, pStartupAccount)) {
        return false;
    }

    return processOverrideSystemTrayAvailabilityCommandLineOption(cmdOptions);
}

bool processStorageDirCommandLineOption(
    const CommandLineParser::Options & options)
{
    const auto storageDirIt = options.find(QStringLiteral("storageDir"));
    if (storageDirIt == options.constEnd()) {
        return true;
    }

    const QString storageDirPath = storageDirIt.value().toString();
    const QFileInfo storageDirInfo{storageDirPath};
    if (!storageDirInfo.exists()) {
        QDir dir{storageDirPath};
        if (!dir.mkpath(storageDirPath)) {
            Q_UNUSED(criticalMessageBox(
                nullptr,
                QCoreApplication::applicationName() + QStringLiteral(" ") +
                    QObject::tr("cannot start"),
                QObject::tr("Cannot create directory for persistent "
                            "storage pointed to by \"storageDir\" "
                            "command line option"),
                QDir::toNativeSeparators(storageDirPath)))

            return false;
        }
    }
    else if (Q_UNLIKELY(!storageDirInfo.isDir())) {
        Q_UNUSED(criticalMessageBox(
            nullptr,
            QCoreApplication::applicationName() + QStringLiteral(" ") +
                QObject::tr("cannot start"),
            QObject::tr("\"storageDir\" command line option "
                        "doesn't point to a directory"),
            QDir::toNativeSeparators(storageDirPath)));

        return false;
    }
    else if (Q_UNLIKELY(!storageDirInfo.isReadable())) {
        Q_UNUSED(criticalMessageBox(
            nullptr,
            QCoreApplication::applicationName() + QStringLiteral(" ") +
                QObject::tr("cannot start"),
            QObject::tr("The directory for persistent storage "
                        "pointed to by \"storageDir\" command "
                        "line option is not readable"),
            QDir::toNativeSeparators(storageDirPath)));

        return false;
    }
    else if (Q_UNLIKELY(!storageDirInfo.isWritable())) {
        Q_UNUSED(criticalMessageBox(
            nullptr,
            QCoreApplication::applicationName() + QStringLiteral(" ") +
                QObject::tr("cannot start"),
            QObject::tr("The directory for persistent storage "
                        "pointed to by \"storageDir\" command "
                        "line option is not writable"),
            QDir::toNativeSeparators(storageDirPath)));

        return false;
    }

    qputenv(LIBQUENTIER_PERSISTENCE_STORAGE_PATH, storageDirPath.toLocal8Bit());
    return true;
}

bool processAccountCommandLineOption(
    const CommandLineParser::Options & options,
    std::unique_ptr<Account> & pStartupAccount)
{
    const auto accountIt = options.find(QStringLiteral("account"));
    if (accountIt == options.constEnd()) {
        return true;
    }

    const QString accountStr = accountIt.value().toString();

    bool isLocal = false;
    qevercloud::UserID userId = -1;
    QString evernoteHost;
    QString accountName;
    ErrorString errorDescription;

    const bool res = parseStartupAccount(
        accountStr, isLocal, userId, evernoteHost, accountName,
        errorDescription);

    if (!res) {
        Q_UNUSED(criticalMessageBox(
            nullptr,
            QCoreApplication::applicationName() + QStringLiteral(" ") +
                QObject::tr("cannot start"),
            QObject::tr("Unable to parse the startup account"),
            errorDescription.localizedString()));

        return false;
    }

    bool foundAccount = false;

    AccountManager accountManager;
    const auto & availableAccounts = accountManager.availableAccounts();

    for (const auto & availableAccount: qAsConst(availableAccounts)) {
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

        pStartupAccount = std::make_unique<Account>(availableAccount);
        foundAccount = true;
        break;
    }

    if (!foundAccount) {
        Q_UNUSED(criticalMessageBox(
            nullptr,
            QCoreApplication::applicationName() + QStringLiteral(" ") +
                QObject::tr("cannot start"),
            QObject::tr("Wrong startup account"),
            QObject::tr("The startup account specified on "
                        "the command line does not correspond "
                        "to any already existing account")));

        return false;
    }

    accountManager.setStartupAccount(*pStartupAccount);
    return true;
}

bool processOverrideSystemTrayAvailabilityCommandLineOption(
    const CommandLineParser::Options & options)
{
    const auto it = options.find(QStringLiteral("overrideSystemTrayAvailability"));
    if (it != options.constEnd()) {
        bool value = it.value().toBool();

        qputenv(
            preferences::keys::overrideSystemTrayAvailabilityEnvVar,
            (value ? QByteArray("1") : QByteArray("0")));
    }

    return true;
}

void finalize()
{
#ifdef BUILDING_WITH_BREAKPAD
    detachBreakpad();
#endif

#ifdef Q_OS_WIN
    // WinAPI magic to disable Windows error reporting dialog in case of
    // crashes: https://stackoverflow.com/a/15660790/1217285
    DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);

    SetUnhandledExceptionFilter(
        (LPTOP_LEVEL_EXCEPTION_FILTER)&null_exception_handler);

    _RTC_SetErrorFunc(&null_runtime_check_handler);
#endif
}

} // namespace quentier
