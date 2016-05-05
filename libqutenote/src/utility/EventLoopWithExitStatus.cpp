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

void EventLoopWithExitStatus::exitAsFailureWithError(QString errorDescription)
{
    Q_UNUSED(errorDescription)
    QEventLoop::exit(ExitStatus::Failure);
}

} // namespace qute_note
