#ifndef LIB_QUENTIER_NOTE_EDITOR_EDIT_HYPERLINK_DIALOG_H
#define LIB_QUENTIER_NOTE_EDITOR_EDIT_HYPERLINK_DIALOG_H

#include <quentier/utility/Qt4Helper.h>
#include <QDialog>
#include <QUrl>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(EditHyperlinkDialog)
}

namespace quentier {

class EditHyperlinkDialog: public QDialog
{
    Q_OBJECT
public:
    explicit EditHyperlinkDialog(QWidget * parent = Q_NULLPTR,
                                 const QString & startupText = QString(),
                                 const QString & startupUrl = QString(),
                                 const quint64 idNumber = 0);
    virtual ~EditHyperlinkDialog();

Q_SIGNALS:
    void accepted(QString text, QUrl url, quint64 idNumber, bool startupUrlWasEmpty);

private Q_SLOTS:
    virtual void accept() Q_DECL_OVERRIDE;

    void onUrlEdited(QString url);
    void onUrlEditingFinished();

private:
    bool validateAndGetUrl(QUrl & url);

private:
    Ui::EditHyperlinkDialog * m_pUI;
    const quint64       m_idNumber;
    const bool          m_startupUrlWasEmpty;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_EDIT_HYPERLINK_DIALOG_H
