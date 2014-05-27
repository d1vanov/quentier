#include "SavedSearchLocalStorageManagerAsyncTester.h"
#include <client/local_storage/LocalStorageManagerThread.h>

namespace qute_note {
namespace test {

SavedSearchLocalStorageManagerAsyncTester::SavedSearchLocalStorageManagerAsyncTester(QObject * parent) :
    QObject(parent),
    m_state(STATE_UNINITIALIZED),
    m_localStorageManagerThread(nullptr)
{}

SavedSearchLocalStorageManagerAsyncTester::~SavedSearchLocalStorageManagerAsyncTester()
{}

void SavedSearchLocalStorageManagerAsyncTester::onInitTestCase()
{
    QString username = "SavedSearchLocalStorageManagerAsyncTester";
    qint32 userId = 0;
    bool startFromScratch = true;

    if (m_localStorageManagerThread != nullptr) {
        delete m_localStorageManagerThread;
        m_localStorageManagerThread = nullptr;
    }

    m_localStorageManagerThread = new LocalStorageManagerThread(username, userId, startFromScratch, this);
    // TODO: create connections
    // TODO: send initial add request
}



}
}
