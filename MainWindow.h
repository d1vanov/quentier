#ifndef __QUTE_NOTE__MAINWINDOW_H
#define __QUTE_NOTE__MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget * pParentWidget = nullptr);
    ~MainWindow();
    
public:
    void SetDefaultLayoutSettings();
    virtual void resizeEvent(QResizeEvent * pResizeEvent);

private:
    void ResizeHelperDockWidget(QDockWidget * pDock, const int dockHeight,
                                const int dockWidth, const double minHeightMultiplier,
                                const double minWidthMultiplier,
                                const double maxHeightMultiplier,
                                const double maxWidthMultiplier);

    void ConnectActionsToEditorSlots();

private slots:
    void textBold();
    void textItalic();
    void textUnderline();
    void textStrikeThrough();

private:
    Ui::MainWindow * m_pUI;
};

#endif // __QUTE_NOTE__MAINWINDOW_H
