#ifndef __QUTE_NOTE__MAINWINDOW_H
#define __QUTE_NOTE__MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

QT_FORWARD_DECLARE_CLASS(EvernoteOAuthBrowser)
QT_FORWARD_DECLARE_CLASS(QUrl)

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget * pParentWidget = nullptr);
    virtual ~MainWindow() final;
    
public:
    void SetDefaultLayoutSettings();
    virtual void resizeEvent(QResizeEvent * pResizeEvent);

    EvernoteOAuthBrowser * OAuthBrowser();

private:
    void ResizeHelperDockWidget(QDockWidget * pDock, const int dockHeight,
                                const int dockWidth, const double minHeightMultiplier,
                                const double minWidthMultiplier,
                                const double maxHeightMultiplier,
                                const double maxWidthMultiplier);

    void ConnectActionsToEditorSlots();

public slots:
    void onSetStatusBarText(QString message, const int duration = 0);
    void onShowAuthWebPage(QUrl url);

private slots:
    void textBold();
    void textItalic();
    void textUnderline();
    void textStrikeThrough();

private:
    Ui::MainWindow * m_pUI;
    QWidget * m_currentStatusBarChildWidget;
    EvernoteOAuthBrowser * m_pBrowser;
};

#endif // __QUTE_NOTE__MAINWINDOW_H
