#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__PAGE_MUTATION_HANDLER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__PAGE_MUTATION_HANDLER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>

namespace qute_note {

/**
 * @brief The PageMutationHandler class is used as object providing slot
 * to be called from JavaScript on the change of note editor page's content;
 * it is a workaround for QWebEnginePage's inability to signal of page content changes natively
 */
class PageMutationHandler: public QObject
{
    Q_OBJECT
public:
    explicit PageMutationHandler(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void contentsChanged();

public Q_SLOTS:
    void onPageMutation();
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__JAVASCRIPT_GLUE__PAGE_MUTATION_HANDLER_H
