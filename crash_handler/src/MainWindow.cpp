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

    QString realMinidumpLocation = minidumpLocation;
    realMinidumpLocation.replace(QString::fromUtf8("\\\\"), QString::fromUtf8("\\"));

    m_pUi->minidumpFilePathLineEdit->setText(realMinidumpLocation);

    QString quentierId, quentierSymbolsSourceName, errorDescription;
    QByteArray quentierSymbolsData;
    bool res = qualifySymbolsFile(symbolsFileLocation, QString::fromUtf8("quentier"),
                                  quentierSymbolsSourceName, quentierId, quentierSymbolsData, errorDescription);
    if (!res) {
        m_pUi->stackTracePlainTextEdit->setPlainText(errorDescription);
        return;
    }

    QString realStackwalkBinaryLocation = convertFromNativePath(stackwalkBinaryLocation);
    QFileInfo stackwalkBinaryInfo(realStackwalkBinaryLocation);
    if (Q_UNLIKELY(!stackwalkBinaryInfo.exists())) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: minidump stackwalk utility file doesn't exist") +
                                                     QString::fromUtf8(": ") + QDir::toNativeSeparators(realStackwalkBinaryLocation));
        return;
    }
    else if (Q_UNLIKELY(!stackwalkBinaryInfo.isFile())) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: the path to minidump stackwalk utility doesn't point to an actual file") +
                                                     QString::fromUtf8(": ") + QDir::toNativeSeparators(realStackwalkBinaryLocation));
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString tmpFolderPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
#else
    QString tmpFolderPath = QDesktopServices::storageLocation(QDesktopServices::TempLocation);
#endif

    QString unpackFolderSymbols = tmpFolderPath + QString::fromUtf8("/") + quentierSymbolsSourceName +
                                  QString::fromUtf8("_") + quentierId + QString::fromUtf8("/symbols");

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

    QString unpackFolder = unpackFolderSymbols + QString::fromUtf8("/") + quentierSymbolsSourceName + QString::fromUtf8("/") + quentierId;

    QDir unpackFolderDir(unpackFolder);
    res = unpackFolderDir.mkpath(unpackFolder);
    if (Q_UNLIKELY(!res)) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: failed to create the temporary directory for unpacking the symbols") +
                                                     QString::fromUtf8(":\n") + QDir::toNativeSeparators(unpackFolder));
        return;
    }

    QString newSymbolsFilePath = unpackFolder + QString::fromUtf8("/quentier.sym");

    QFile newSymbolsFile(newSymbolsFilePath);
    res = newSymbolsFile.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!res)) {
        m_pUi->stackTracePlainTextEdit->setPlainText(tr("Error: failed to open the temporary file for unpacked symbols for writing") +
                                                     QString::fromUtf8(":\n") + QDir::toNativeSeparators(unpackFolder));
        return;
    }

    newSymbolsFile.write(quentierSymbolsData);
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
    stackwalkArgs << QDir::fromNativeSeparators(realMinidumpLocation);
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

QString MainWindow::convertFromNativePath(const QString & path) const
{
    QString result = path;

#ifdef Q_OS_WIN
    result.replace(QString::fromUtf8("\\\\"), QString::fromUtf8("\\"));
    result = QDir::fromNativeSeparators(result);
#endif

    return result;
}

bool MainWindow::qualifySymbolsFile(const QString & symbolsFileLocation,
                                    const QString & symbolsSourceNameHint,
                                    QString & symbolsSourceName, QString & id,
                                    QByteArray & symbolsUncompressedData,
                                    QString & errorDescription)
{
    QString symbolsFilePath = convertFromNativePath(symbolsFileLocation);

    QFileInfo symbolsFileInfo(symbolsFilePath);
    if (Q_UNLIKELY(!symbolsFileInfo.exists())) {
        errorDescription = tr("Error: symbols file doesn't exist") + QString::fromUtf8(": ") +
                           QDir::toNativeSeparators(symbolsFilePath);
        return false;
    }
    else if (Q_UNLIKELY(!symbolsFileInfo.isFile())) {
        errorDescription = tr("Error: the path to symbols file doesn't really point to a file") +
                           QString::fromUtf8(": ") + QDir::toNativeSeparators(symbolsFilePath);
        return false;
    }

    QFile compressedSymbolsFile(symbolsFileInfo.absoluteFilePath());
    if (!compressedSymbolsFile.open(QIODevice::ReadOnly)) {
        errorDescription = tr("Error: can't open the compressed symbols file for reading") +
                           QString::fromUtf8(": ") + QDir::toNativeSeparators(symbolsFilePath);
        return false;
    }

    symbolsUncompressedData = compressedSymbolsFile.readAll();
    compressedSymbolsFile.close();

    QTemporaryFile uncompressedSymbolsFile;
    if (!uncompressedSymbolsFile.open()) {
        errorDescription = tr("Error: can't create the temporary file to store the uncompressed symbols") +
                           QString::fromUtf8(": ") + QDir::toNativeSeparators(symbolsFilePath);
        return false;
    }

    uncompressedSymbolsFile.setAutoRemove(true);

    symbolsUncompressedData = qUncompress(symbolsUncompressedData);
    uncompressedSymbolsFile.write(symbolsUncompressedData);
    QFileInfo uncompressedSymbolsFileInfo(uncompressedSymbolsFile);

    // NOTE: all attempts to read the first line from the symbols file using QFile's API result in crash for unknown reason;
    // maybe it doesn't like what it finds within the file, I have no idea.
    // Working around this crap by using C++ standard library's API.
    std::ifstream symbolsFileStream;
    symbolsFileStream.open(QDir::toNativeSeparators(uncompressedSymbolsFileInfo.absoluteFilePath()).toLocal8Bit().constData(), std::ifstream::in);
    if (Q_UNLIKELY(!symbolsFileStream.good())) {
        errorDescription = tr("Error: can't open the temporary uncompressed symbols file for reading") +
                           QString::fromUtf8(": ") + QDir::toNativeSeparators(symbolsFilePath);
        return false;
    }

    char buf[1024];
    symbolsFileStream.getline(buf, 1024);

    QByteArray symbolsFirstLineBytes(buf, 1024);
    QString symbolsFirstLine = QString::fromUtf8(symbolsFirstLineBytes);
    int symbolsSourceNameIndex = symbolsFirstLine.indexOf(symbolsSourceNameHint);
    if (Q_UNLIKELY(symbolsSourceNameIndex < 0)) {
        errorDescription = tr("Error: can't find the symbols source name within the first 1024 bytes read from the symbols file") +
                           QString::fromUtf8(": ") + QString::fromLocal8Bit(buf, 1024);
        return false;
    }

    symbolsFirstLine.truncate(symbolsSourceNameIndex + symbolsSourceNameHint.size());

    QRegExp regex(QString::fromUtf8("\\s"));
    QStringList symbolsFirstLineTokens = symbolsFirstLine.split(regex);

    if (symbolsFirstLineTokens.size() != 5) {
        errorDescription = tr("Error: unexpected number of tokens at the first line of the symbols file") +
                           QString::fromUtf8(": ") + symbolsFirstLineTokens.join(QString::fromUtf8(", "));
        return false;
    }

    id = symbolsFirstLineTokens.at(3);
    if (Q_UNLIKELY(id.isEmpty())) {
        errorDescription = tr("Error: symbol id is empty, first line of the minidump file") +
                           QString::fromUtf8(": ") + symbolsFirstLineTokens.join(QString::fromUtf8(", "));
        return false;
    }

    symbolsSourceName = symbolsFirstLineTokens.at(4);
    if (Q_UNLIKELY(symbolsSourceName.isEmpty())) {
        errorDescription = tr("Error: minidump's application name is empty, first line of the minidump file") +
                           QString::fromUtf8(": ") + symbolsFirstLineTokens.join(QString::fromUtf8(", "));
        return false;
    }

#ifdef _MSC_VER
    symbolsSourceName += QString::fromUtf8(".pdb");
#endif

    return true;
}

QString MainWindow::readData(QProcess & process, const bool fromStdout)
{
    QByteArray output = (fromStdout
                         ? process.readAllStandardOutput()
                         : process.readAllStandardError());
    return QString::fromUtf8(output);
}
