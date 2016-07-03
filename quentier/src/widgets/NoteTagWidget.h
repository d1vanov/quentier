#ifndef QUENTIER_WIDGETS_NOTE_TAG_WIDGET_H
#define QUENTIER_WIDGETS_NOTE_TAG_WIDGET_H

#include <quentier/utility/Qt4Helper.h>
#include <QWidget>

namespace Ui {
class NoteTagWidget;
}

/**
 * @brief The NoteTagWidget class represents a single tag within the list of currently selected note's tags
 *
 * It is a very simple class combining tag name label and the button intended to signal
 * the desire to remove the tag from the note
 */
class NoteTagWidget: public QWidget
{
    Q_OBJECT
public:
    explicit NoteTagWidget(QWidget * parent = Q_NULLPTR);
    ~NoteTagWidget();

    QString tagName() const;
    void setTagName(const QString & name);

Q_SIGNALS:
    void removeTagFromNote(const QString & name);

private Q_SLOTS:
    void onRemoveTagButtonPressed();

private:
    Ui::NoteTagWidget * m_pUi;
};

#endif // QUENTIER_WIDGETS_NOTE_TAG_WIDGET_H
