#include "BreakpadIntegration.h"
#include <quentier/utility/Macros.h>
#include <quentier/logging/QuentierLogger.h>
#include <QString>
#include <QFileInfoList>
#include <QDir>

namespace quentier {

void setQtWebEngineFlags()
{
    const char * envVar = "QTWEBENGINE_CHROMIUM_FLAGS";
    const char * disableInProcessStackTraces = "--disable-in-process-stack-traces";

    QByteArray flags = qgetenv(envVar);
    if (!flags.contains(disableInProcessStackTraces)) {
        qputenv(envVar, flags + " " + disableInProcessStackTraces);
    }
}

void findCompressedSymbolsFiles(const QApplication & app,
                                QString & quentierCompressedSymbolsFilePath,
                                QString & libquentierCompressedSymbolsFilePath)
{
    QNDEBUG(QStringLiteral("findCompressedSymbolsFiles"));

    quentierCompressedSymbolsFilePath.resize(0);
    libquentierCompressedSymbolsFilePath.resize(0);

    QString appFilePath = app.applicationFilePath();
    QFileInfo appFileInfo(appFilePath);

    QDir appDir(appFileInfo.absoluteDir());

    QFileInfoList fileInfosNearApp = appDir.entryInfoList(QDir::Files);
    for(auto it = fileInfosNearApp.constBegin(), end = fileInfosNearApp.constEnd(); it != end; ++it)
    {
        const QFileInfo & fileInfo = *it;
        QString fileName = fileInfo.fileName();
        if (!fileName.endsWith(QStringLiteral(".syms.compressed"))) {
            continue;
        }

        if (fileName.startsWith(QStringLiteral("lib")))
        {
            if (fileName.contains(QStringLiteral("quentier"))) {
                libquentierCompressedSymbolsFilePath = fileInfo.absoluteFilePath();
                QNDEBUG(QStringLiteral("Found libquentier's compressed symbols file: ") << libquentierCompressedSymbolsFilePath);
            }

            continue;
        }

        if (fileName.startsWith(QStringLiteral("quentier"))) {
            quentierCompressedSymbolsFilePath = fileInfo.absoluteFilePath();
            QNDEBUG(QStringLiteral("Found quentier's compressed symbols file: ") << quentierCompressedSymbolsFilePath);
            continue;
        }
    }
}

} // namespace quentier
