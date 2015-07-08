#ifndef __QUTE_NOTE__CORE__TOOLS__QUTE_NOTE_APPLICATION_H
#define __QUTE_NOTE__CORE__TOOLS__QUTE_NOTE_APPLICATION_H

#include <tools/Linkage.h>
#include <QApplication>

namespace qute_note {

class QUTE_NOTE_EXPORT QuteNoteApplication: public QApplication
{
public:
    QuteNoteApplication(int & argc, char * argv[]);
    virtual ~QuteNoteApplication();

    virtual bool notify(QObject * object, QEvent * event);

    bool hasGui() const;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__TOOLS__QUTE_NOTE_APPLICATION_H
