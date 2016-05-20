#ifndef LIB_QUTE_NOTE_UTILITY_QUTE_NOTE_APPLICATION_H
#define LIB_QUTE_NOTE_UTILITY_QUTE_NOTE_APPLICATION_H

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

#endif // LIB_QUTE_NOTE_UTILITY_QUTE_NOTE_APPLICATION_H
