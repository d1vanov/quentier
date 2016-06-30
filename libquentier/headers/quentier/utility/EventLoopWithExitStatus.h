#ifndef LIB_QUENTIER_UTILITY_EVENT_LOOP_WITH_EXIT_STATUS_H
#define LIB_QUENTIER_UTILITY_EVENT_LOOP_WITH_EXIT_STATUS_H

#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/Linkage.h>
#include <QEventLoop>

namespace quentier {

class QUENTIER_EXPORT EventLoopWithExitStatus : public QEventLoop
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

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_EVENT_LOOP_WITH_EXIT_STATUS_H
