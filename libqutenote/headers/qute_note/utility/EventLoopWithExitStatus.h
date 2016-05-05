#ifndef __LIB_QUTE_NOTE__UTILITY__EVENT_LOOP_WITH_EXIT_STATUS_H
#define __LIB_QUTE_NOTE__UTILITY__EVENT_LOOP_WITH_EXIT_STATUS_H

#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/utility/Linkage.h>
#include <QEventLoop>

namespace qute_note {

class QUTE_NOTE_EXPORT EventLoopWithExitStatus : public QEventLoop
{
    Q_OBJECT
public:
    explicit EventLoopWithExitStatus(QObject * parent = Q_NULLPTR);

    struct ExitStatus {
        enum type {
            Success,
            Failure,
            Timeout
        };
    };

public Q_SLOTS:
    void exitAsSuccess();
    void exitAsFailure();
    void exitAsFailureWithError(QString errorDescription);
    void exitAsTimeout();
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__EVENT_LOOP_WITH_EXIT_STATUS_H
