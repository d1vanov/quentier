#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_ATTACHMENT_STORAGE_PATH_CONFIG_DIALOG_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_ATTACHMENT_STORAGE_PATH_CONFIG_DIALOG_H

#include <qute_note/utility/Qt4Helper.h>
#include <QDialog>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(AttachmentStoragePathConfigDialog)
}

namespace qute_note {

class AttachmentStoragePathConfigDialog: public QDialog
{
    Q_OBJECT
public:
    explicit AttachmentStoragePathConfigDialog(QWidget * parent = Q_NULLPTR);
    virtual ~AttachmentStoragePathConfigDialog();

    const QString path() const;

private Q_SLOTS:
    void onChooseFolder();

private:
    Ui::AttachmentStoragePathConfigDialog * m_pUI;
};

const QString getAttachmentStoragePath(QWidget * parent);

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_ATTACHMENT_STORAGE_PATH_CONFIG_DIALOG_H
