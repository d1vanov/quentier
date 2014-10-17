#ifndef __QUTE_NOTE__CORE__TOOLS__QUTE_NOTE_APPLICATION_H
#define __QUTE_NOTE__CORE__TOOLS__QUTE_NOTE_APPLICATION_H

#include <QApplication>

namespace qute_note {

class QuteNoteApplication: public QApplication
{
public:
    QuteNoteApplication(int & argc, char * argv[]);
    virtual ~QuteNoteApplication();

    virtual bool notify(QObject * object, QEvent * event);
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__QUTE_NOTE_APPLICATION_H
