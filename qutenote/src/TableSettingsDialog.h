#ifndef __QUTE_NOTE__GUI__TABLE_SETTINGS_DIALOG_H
#define __QUTE_NOTE__GUI__TABLE_SETTINGS_DIALOG_H

#include <QDialog>
#include <qute_note/utility/Qt4Helper.h>

namespace Ui {
class TableSettingsDialog;
}

class TableSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TableSettingsDialog(QWidget * parent = 0);
    ~TableSettingsDialog();

    int numRows() const;
    int numColumns() const;
    double tableWidth() const;
    bool relativeWidth() const;

public Q_SLOTS:
    void onOkButtonPressed();
    void onCancelButtonPressed();

private:
    bool verifySettings(QString & error) const;
    bool checkRelativeWidth() const;

private:
    Ui::TableSettingsDialog *ui;

    int     m_numRows;
    int     m_numColumns;
    double  m_tableWidth;
    bool    m_relativeWidth;
};

#endif // __QUTE_NOTE__GUI__TABLE_SETTINGS_DIALOG_H
