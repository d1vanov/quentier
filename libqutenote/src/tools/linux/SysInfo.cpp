#include "../SysInfo.h"
#include <unistd.h>
#include <sys/sysinfo.h>
#include <StackTrace.h>
#include <QDir>

namespace qute_note {

SysInfo & SysInfo::GetSingleton()
{
    static SysInfo sysInfo;
    return sysInfo;
}

qint64 SysInfo::GetPageSize()
{
    return static_cast<qint64>(sysconf(_SC_PAGESIZE));
}

qint64 SysInfo::GetTotalMemoryBytes()
{
    struct sysinfo si;
    int rc = sysinfo(&si);
    if (rc) {
        return -1;
    }

    return static_cast<qint64>(si.totalram);
}

qint64 SysInfo::GetFreeMemoryBytes()
{
    struct sysinfo si;
    int rc = sysinfo(&si);
    if (rc) {
        return -1;
    }

    return static_cast<qint64>(si.freeram);
}

QString SysInfo::GetStackTrace()
{
    fpos_t pos;

    QString tmpFile = QDir::tempPath();
    tmpFile += "/QuteNoteStackTrace.txt";

    // flush existing stderr and reopen it as file
    fflush(stderr);
    fgetpos(stderr, &pos);
    int fd = dup(fileno(stderr));
    freopen(tmpFile.toLocal8Bit().data(), "w", stderr);

    stacktrace::displayCurrentStackTrace();

    // revert stderr
    fflush(stderr);
    dup2(fd, fileno(stderr));
    close(fd);
    clearerr(stderr);
    fsetpos(stderr, &pos);

    QFile file(tmpFile);
    bool res = file.open(QIODevice::ReadOnly);
    if (!res) {
        return "<cannot open temporary file with stacktrace>";
    }

    QByteArray rawOutput = file.readAll();
    QString output(rawOutput);
    return output;
}

SysInfo::SysInfo() {}

SysInfo::~SysInfo() {}

} // namespace qute_note
