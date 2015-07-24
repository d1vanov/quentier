#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__ATTACHMENT_STORAGE_PATH_CONFIG_DIALOG_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__ATTACHMENT_STORAGE_PATH_CONFIG_DIALOG_H

#include <QDialog>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(AttachmentStoragePathConfigDialog)
}

namespace qute_note {

class AttachmentStoragePathConfigDialog: public QDialog
{
    Q_OBJECT
public:
    explicit AttachmentStoragePathConfigDialog(QWidget * parent = nullptr);
    virtual ~AttachmentStoragePathConfigDialog();

    const QString path() const;

private Q_SLOTS:
    void onChooseFolder();

private:
    Ui::AttachmentStoragePathConfigDialog * m_pUI;
};

const QString getAttachmentStoragePath(QWidget * parent);

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__ATTACHMENT_STORAGE_PATH_CONFIG_DIALOG_H
