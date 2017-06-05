#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegExp>
#include <QDesktopServices>
#include <QTemporaryFile>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QStandardPaths>
#endif

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) || (defined(_MSC_VER) && (_MSC_VER <= 1600))
#define QNSIGNAL(className, methodName, ...) SIGNAL(methodName(__VA_ARGS__))
#define QNSLOT(className, methodName, ...) SLOT(methodName(__VA_ARGS__))
#else
#define QNSIGNAL(className, methodName, ...) &className::methodName
#define QNSLOT(className, methodName, ...) &className::methodName
#endif

#include <fstream>

MainWindow::MainWindow(const QString & symbolsFileLocation,
                       const QString & stackwalkBinaryLocation,
                       const QString & minidumpLocation,
                       QWidget * parent) :
    QMainWindow(parent),
    m_pUi(new Ui::MainWindow)
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Quentier crashed"));

    m_pUi->minidumpFilePathLineEdit->setText(minidumpLocation);

    QFileInfo symbolsFileInfo(QDir::fromNativeSeparators(symbolsFileLocation));
    if (Q_UNLIKELY(!symbolsFileInfo.exists())) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: symbols file doesn't exist") +
                                                     QString::fromUtf8(": ") + symbolsFileLocation);
        return;
    }
    else if (Q_UNLIKELY(!symbolsFileInfo.isFile())) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: the path to symbols file doesn't point to an actual file") +
                                                     QString::fromUtf8(": ") + symbolsFileLocation);
        return;
    }

    QFileInfo stackwalkBinaryInfo(QDir::fromNativeSeparators(stackwalkBinaryLocation));
    if (Q_UNLIKELY(!stackwalkBinaryInfo.exists())) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: minidump stackwalk utility file doesn't exist") +
                                                     QString::fromUtf8(": ") + stackwalkBinaryLocation);
        return;
    }
    else if (Q_UNLIKELY(!stackwalkBinaryInfo.isFile())) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: the path to minidump stackwalk utility doesn't point to an actual file") +
                                                     QString::fromUtf8(": ") + stackwalkBinaryLocation);
        return;
    }

    QFile compressedSymbolsFile(symbolsFileInfo.absoluteFilePath());
    if (!compressedSymbolsFile.open(QIODevice::ReadOnly)) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: can't open the compressed symbols file for reading") +
                                                     QString::fromUtf8(": ") + symbolsFileLocation);
        return;
    }

    QByteArray symbolsFileData = compressedSymbolsFile.readAll();
    compressedSymbolsFile.close();

    QTemporaryFile uncompressedSymbolsFile;
    if (!uncompressedSymbolsFile.open()) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: can't create the temporary file to store the uncompressed symbols") +
                                                     QString::fromUtf8(": ") + symbolsFileLocation);
        return;
    }

    uncompressedSymbolsFile.setAutoRemove(true);

    symbolsFileData = qUncompress(symbolsFileData);
    uncompressedSymbolsFile.write(symbolsFileData);
    QFileInfo uncompressedSymbolsFileInfo(uncompressedSymbolsFile);

    // NOTE: all attempts to read the first line from the symbols file using QFile's API result in crash for unknown reason;
    // maybe it doesn't like what it finds within the file, I have no idea.
    // Working around this crap by using C++ standard library's API.
    std::ifstream symbolsFileStream;
    symbolsFileStream.open(QDir::toNativeSeparators(uncompressedSymbolsFileInfo.absoluteFilePath()).toLocal8Bit().constData(), std::ifstream::in);
    if (Q_UNLIKELY(!symbolsFileStream.good())) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: can't open the temporary uncompressed symbols file for reading") +
                                                     QString::fromUtf8(": ") + symbolsFileLocation);
        return;
    }

    char buf[1024];
    symbolsFileStream.getline(buf, 1024);

    QByteArray symbolsFirstLineBytes(buf, 1024);
    QString symbolsFirstLine = QString::fromUtf8(symbolsFirstLineBytes);
    int appNameIndex = symbolsFirstLine.indexOf(QString::fromUtf8("quentier"));
    if (Q_UNLIKELY(appNameIndex < 0)) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: can't find the app name within the first 1024 bytes read from the symbols file") +
                                                     QString::fromUtf8(": ") + QString::fromLocal8Bit(buf, 1024));
        return;
    }

    symbolsFirstLine.truncate(appNameIndex + 8);

    QRegExp regex(QString::fromUtf8("\\s"));
    QStringList symbolsFirstLineTokens = symbolsFirstLine.split(regex);

    if (symbolsFirstLineTokens.size() != 5) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: unexpected number of tokens at the first line of the symbols file") +
                                                     QString::fromUtf8(": ") + symbolsFirstLineTokens.join(QString::fromUtf8(", ")));
        return;
    }

    const QString & id = symbolsFirstLineTokens.at(3);
    if (Q_UNLIKELY(id.isEmpty())) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: symbol id is empty, first line of the minidump file") +
                                                     QString::fromUtf8(": ") + symbolsFirstLineTokens.join(QString::fromUtf8(", ")));
        return;
    }

    const QString & appName = symbolsFirstLineTokens.at(4);
    if (Q_UNLIKELY(appName.isEmpty())) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: minidump's application name is empty, first line of the minidump file") +
                                                     QString::fromUtf8(": ") + symbolsFirstLineTokens.join(QString::fromUtf8(", ")));
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString tmpFolderPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#else
    QString tmpFolderPath = QDesktopServices::storageLocation(QDesktopServices::TempLocation);
#endif

    QString unpackFolderSymbols = tmpFolderPath + QString::fromUtf8("/") + appName + QString::fromUtf8("_") + id + QString::fromUtf8("/symbols");

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QDir unpackFolderSymbolsDir(unpackFolderSymbols);
    if (unpackFolderSymbolsDir.exists())
    {
        bool res = unpackFolderSymbolsDir.removeRecursively();
        if (Q_UNLIKELY(!res)) {
            m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: failed to remove the already existing temporary directory for unpacking the symbols") +
                                                         QString::fromUtf8(":\n") + QDir::toNativeSeparators(unpackFolderSymbols));
            return;
        }
    }
#endif

    QString unpackFolder = unpackFolderSymbols + QString::fromUtf8("/") + appName + QString::fromUtf8("/") + id;
    QDir unpackFolderDir(unpackFolder);
    bool res = unpackFolderDir.mkpath(unpackFolder);
    if (Q_UNLIKELY(!res)) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: failed to create the temporary directory for unpacking the symbols") +
                                                     QString::fromUtf8(":\n") + QDir::toNativeSeparators(unpackFolder));
        return;
    }

    QString newSymbolsFilePath = unpackFolder + QString::fromUtf8("/") + symbolsFileInfo.baseName() + QString::fromUtf8(".sym");

    QFile newSymbolsFile(newSymbolsFilePath);
    res = newSymbolsFile.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!res)) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: failed to open the temporary file for unpacked symbols for writing") +
                                                     QString::fromUtf8(":\n") + QDir::toNativeSeparators(unpackFolder));
        return;
    }

    newSymbolsFile.write(symbolsFileData);
    newSymbolsFile.close();

    QProcess * pStackwalkProcess = new QProcess(this);
    QObject::connect(pStackwalkProcess, QNSIGNAL(QProcess,readyReadStandardOutput),
                     this, QNSLOT(MainWindow,onMinidumpStackwalkReadyReadStandardOutput));
    QObject::connect(pStackwalkProcess, QNSIGNAL(QProcess,readyReadStandardError),
                     this, QNSLOT(MainWindow,onMinidumpStackwalkReadyReadStandardError));
    QObject::connect(pStackwalkProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
                     this, SLOT(onMinidumpStackwalkProcessFinished(int,QProcess::ExitStatus)));

    QStringList stackwalkArgs;
    stackwalkArgs.reserve(2);
    stackwalkArgs << QDir::fromNativeSeparators(minidumpLocation);
    stackwalkArgs << unpackFolderSymbols;

    pStackwalkProcess->start(stackwalkBinaryInfo.absoluteFilePath(), stackwalkArgs, QIODevice::ReadOnly);
}

MainWindow::~MainWindow()
{
    delete m_pUi;
}

void MainWindow::onMinidumpStackwalkReadyReadStandardOutput()
{
    QProcess * pStackwalkProcess = qobject_cast<QProcess*>(sender());
    if (Q_UNLIKELY(!pStackwalkProcess)) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: can't cast the invoker of minidump stackwalk stdout update to QProcess"));
        return;
    }

    m_output += readData(*pStackwalkProcess, /* from stdout = */ true);
    m_pUi->stackTracePlainTextEdit->setPlainText(m_output);
}

void MainWindow::onMinidumpStackwalkReadyReadStandardError()
{
    QProcess * pStackwalkProcess = qobject_cast<QProcess*>(sender());
    if (Q_UNLIKELY(!pStackwalkProcess)) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: can't cast the invoker of minidump stackwalk stderr update to QProcess"));
        return;
    }

    m_error += readData(*pStackwalkProcess, /* from stdout = */ false);
}

void MainWindow::onMinidumpStackwalkProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    QString output = tr("Stacktrace extraction finished, exit code") + QString::fromUtf8(": ") + QString::number(exitCode) + QString::fromUtf8("\n");
    output += m_output;
    output += QString::fromUtf8("\n\n");
    output += m_error;

    m_pUi->stackTracePlainTextEdit->setPlainText(output);
}

QString MainWindow::readData(QProcess & process, const bool fromStdout)
{
    QByteArray output = (fromStdout
                         ? process.readAllStandardOutput()
                         : process.readAllStandardError());

#ifdef Q_OS_WIN
    return QString::fromUtf16(reinterpret_cast<const ushort*>(output.constData()), output.size());
#else
    return QString::fromUtf8(output);
#endif
}
