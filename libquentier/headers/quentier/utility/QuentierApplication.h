#ifndef LIB_QUENTIER_UTILITY_QUENTIER_APPLICATION_H
#define LIB_QUENTIER_UTILITY_QUENTIER_APPLICATION_H

#include <quentier/utility/Linkage.h>
#include <QApplication>

namespace quentier {

class QUENTIER_EXPORT QuentierApplication: public QApplication
{
public:
    QuentierApplication(int & argc, char * argv[]);
    virtual ~QuentierApplication();

    virtual bool notify(QObject * object, QEvent * event);
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_QUENTIER_APPLICATION_H
