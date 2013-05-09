#ifndef TEXT_EDIT_WITH_CHECKBOXES_H
#define TEXT_EDIT_WITH_CHECKBOXES_H

#include <QTextEdit>

class TextEditWithCheckboxes: public QTextEdit
{
    Q_OBJECT
public:
    explicit TextEditWithCheckboxes(QWidget * parent = nullptr);
    virtual ~TextEditWithCheckboxes() override {}

protected:
    virtual void mousePressEvent(QMouseEvent * pEvent) override;
    virtual void mouseMoveEvent(QMouseEvent * pEvent) override;
};

#endif // TEXT_EDIT_WITH_CHECKBOXES_H
