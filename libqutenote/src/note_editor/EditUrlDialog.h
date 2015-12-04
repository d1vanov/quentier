#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__EDIT_URL_DIALOG_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__EDIT_URL_DIALOG_H

#include <qute_note/utility/Qt4Helper.h>
#include <QDialog>
#include <QUrl>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(EditUrlDialog)
}

namespace qute_note {

class EditUrlDialog: public QDialog
{
    Q_OBJECT
public:
    explicit EditUrlDialog(QWidget * parent = Q_NULLPTR,
                           const QString & startupText = QString(),
                           const QString & startupUrl = QString(),
                           const quint64 idNumber = 0);
    virtual ~EditUrlDialog();

Q_SIGNALS:
    void accepted(QString text, QUrl url, quint64 idNumber, bool startupUrlWasEmpty);

private Q_SLOTS:
    virtual void accept() Q_DECL_OVERRIDE;

    void onUrlEdited(QString url);
    void onUrlEditingFinished();

private:
    bool validateAndGetUrl(QUrl & url);

private:
    Ui::EditUrlDialog * m_pUI;
    const quint64       m_idNumber;
    const bool          m_startupUrlWasEmpty;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__EDIT_URL_DIALOG_H
