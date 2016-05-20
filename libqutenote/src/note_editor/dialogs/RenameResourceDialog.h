#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_DIALOGS_RENAME_RESOURCE_DIALOG_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_DIALOGS_RENAME_RESOURCE_DIALOG_H

#include <qute_note/utility/Qt4Helper.h>
#include <QDialog>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(RenameResourceDialog)
}

namespace qute_note {

class RenameResourceDialog: public QDialog
{
    Q_OBJECT
public:
    explicit RenameResourceDialog(const QString & initialResourceName,
                                  QWidget * parent = Q_NULLPTR);
    virtual ~RenameResourceDialog();

Q_SIGNALS:
    void accepted(QString newResourceName);

private Q_SLOTS:
    virtual void accept() Q_DECL_OVERRIDE;

private:
    Ui::RenameResourceDialog * m_pUI;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_DIALOGS_RENAME_RESOURCE_DIALOG_H
