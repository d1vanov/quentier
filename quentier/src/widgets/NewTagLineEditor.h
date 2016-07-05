#ifndef QUENTIER_WIDGETS_NEW_TAG_LINE_EDITOR_H
#define QUENTIER_WIDGETS_NEW_TAG_LINE_EDITOR_H

#include <quentier/utility/Qt4Helper.h>
#include <QLineEdit>

namespace Ui {
class NewTagLineEditor;
}

QT_FORWARD_DECLARE_CLASS(QCompleter)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)

class NewTagLineEditor: public QLineEdit
{
    Q_OBJECT
public:
    explicit NewTagLineEditor(TagModel * pTagModel, QWidget * parent = Q_NULLPTR);
    virtual ~NewTagLineEditor();

Q_SIGNALS:
    void newTagAdded(QString name);

private Q_SLOTS:
    void onTagModelSortingChanged();

protected:
    virtual void keyPressEvent(QKeyEvent * pEvent) Q_DECL_OVERRIDE;

private:
    void setupCompleter();

private:
    Ui::NewTagLineEditor *  m_pUi;
    TagModel *              m_pTagModel;
    QCompleter *            m_pCompleter;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NEW_TAG_LINE_EDITOR_H
