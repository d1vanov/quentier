#ifndef __LIB_QUTE_NOTE__UTILITY__QUTE_NOTE_APPLICATION_H
#define __LIB_QUTE_NOTE__UTILITY__QUTE_NOTE_APPLICATION_H

#include <qute_note/utility/Linkage.h>
#include <QApplication>

namespace qute_note {

class QUTE_NOTE_EXPORT QuteNoteApplication: public QApplication
{
public:
    QuteNoteApplication(int & argc, char * argv[]);
    virtual ~QuteNoteApplication();

    virtual bool notify(QObject * object, QEvent * event);
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__QUTE_NOTE_APPLICATION_H
