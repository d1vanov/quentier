#include "LocalStorageManagerThread.h"

namespace qute_note {

class LocalStorageManagerThreadWorker
{};

LocalStorageManagerThread::LocalStorageManagerThread(QObject * parent) :
    QThread(parent)
{}

LocalStorageManagerThread::~LocalStorageManagerThread()
{}

}
