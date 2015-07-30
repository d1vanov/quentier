#include <qute_note/utility/EventLoopWithExitStatus.h>

namespace qute_note {

EventLoopWithExitStatus::EventLoopWithExitStatus(QObject * parent) :
    QEventLoop(parent)
{
}

void EventLoopWithExitStatus::exitAsSuccess()
{
    QEventLoop::exit(ExitStatus::Success);
}

void EventLoopWithExitStatus::exitAsFailure()
{
    QEventLoop::exit(ExitStatus::Failure);
}

void EventLoopWithExitStatus::exitAsTimeout()
{
    QEventLoop::exit(ExitStatus::Timeout);
}

} // namespace qute_note
