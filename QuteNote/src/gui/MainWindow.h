#ifndef __QUTE_NOTE__MAINWINDOW_H
#define __QUTE_NOTE__MAINWINDOW_H

#include <QMainWindow>

template <class T>
class Singleton;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    static MainWindow & Instance();
    friend class Singleton<MainWindow>;

private:
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

public slots:
    void setStatusBarText(QString message, const int duration = 0);

private slots:
    void textBold();
    void textItalic();
    void textUnderline();
    void textStrikeThrough();

private:
    Ui::MainWindow * m_pUI;
    QWidget * m_currentStatusBarChildWidget;
};

#endif // __QUTE_NOTE__MAINWINDOW_H
