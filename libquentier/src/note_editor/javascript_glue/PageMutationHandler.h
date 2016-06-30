#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_PAGE_MUTATION_HANDLER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_PAGE_MUTATION_HANDLER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>

namespace quentier {

/**
 * @brief The PageMutationHandler class represents an object providing slot
 * to be called from JavaScript on the change of note editor page's content;
 * it would then pass the signal on to NoteEditor.
 *
 * The necessity for this class comes from two facts:
 * 1. As of Qt 5.5.x QWebEnginePage's doesn't have just any signal which could provide
 * such information
 * 2. QtWebKit's contentsChanged signal from QWebPage seems to be not smart enough for
 * NoteEditor's purposes: it would notify of any changes while only those caused by
 * manual editing are really wanted and needed.
 *
 * Hence, JavaScript-side filtering of page mutations + this class for a dialog between JS and C++
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

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_PAGE_MUTATION_HANDLER_H
