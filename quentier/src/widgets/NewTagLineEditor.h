#ifndef QUENTIER_WIDGETS_NEW_TAG_LINE_EDITOR_H
#define QUENTIER_WIDGETS_NEW_TAG_LINE_EDITOR_H

#include <quentier/utility/Qt4Helper.h>
#include <QLineEdit>
#include <QPointer>

namespace Ui {
class NewTagLineEditor;
}

QT_FORWARD_DECLARE_CLASS(QCompleter)
QT_FORWARD_DECLARE_CLASS(QModelIndex)

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
    void onTagModelDataChanged(const QModelIndex & from,
                               const QModelIndex & to);
    void onTagModelRowsChanged(const QModelIndex & index, int start, int end);
    void onTagModelChanged();

protected:
    virtual void keyPressEvent(QKeyEvent * pEvent) Q_DECL_OVERRIDE;

private:
    void createConnections();
    void setupCompleter();

private:
    Ui::NewTagLineEditor *  m_pUi;
    QPointer<TagModel>      m_pTagModel;
    QCompleter *            m_pCompleter;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NEW_TAG_LINE_EDITOR_H
