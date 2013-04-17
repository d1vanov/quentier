#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <cmath>
#include <QtDebug>

MainWindow::MainWindow(QWidget * pParentWidget) :
    QMainWindow(pParentWidget),
    m_pUI(new Ui::MainWindow)
{
    m_pUI->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete m_pUI;
}

void MainWindow::SetDefaultLayoutSettings()
{
    int mainWindowHeight = height();
    int mainWindowWidth = width();

    // Let's make widgets on the left take a reasonable amount of window width
    int helperWidgetsWidth = std::floor(0.175 * mainWindowWidth + 0.5);

    // Calculating optimal height for each helper widget. Together they are
    // going to take approx. 85% of useable height. There should be some reasonable
    // proportion between these widgets heights. For example, labels should have
    // about 30% of sum height, notebooks - about 25%, search properties - about 20%,
    // saved requests - about 20%, trash - only about 5%.

    int helperWidgetsAvailableHeight = std::floor(0.9 * mainWindowHeight + 0.5);

    int dockNotebooksHeight = std::floor(0.3 * helperWidgetsAvailableHeight + 0.5);
    int dockLabelsHeight = std::floor(0.2 * helperWidgetsAvailableHeight + 0.5);
    int dockSearchPropertiesHeight = std::floor(0.2 * helperWidgetsAvailableHeight + 0.5);
    int dockSavedRequestsHeight = std::floor(0.2 * helperWidgetsAvailableHeight + 0.5);
    int dockTrashHeight = std::floor(0.1 * helperWidgetsAvailableHeight + 0.5);

    // Now resize helper dock widgets appropriately
    if (m_pUI == nullptr) {
        qDebug() << "Warning! UI pointer is NULL!";
        return;
    }
    Ui::MainWindow & ui = *m_pUI;

    // multipliers to set min and max height and width for helper widgets
    double minHeightMultiplier = 0.6;
    double minWidthMultiplier = 0.6;
    double maxHeightMultiplier = 1.5;
    double maxWidthMultiplier = 1.25;

    // 1) Dock notebooks
    ResizeHelperDockWidget(ui.dockNotebooks, dockNotebooksHeight, helperWidgetsWidth,
                           minHeightMultiplier, minWidthMultiplier, maxHeightMultiplier,
                           maxWidthMultiplier);
    // 2) Dock labels
    ResizeHelperDockWidget(ui.dockLabels, dockLabelsHeight, helperWidgetsWidth,
                           minHeightMultiplier, minWidthMultiplier, maxHeightMultiplier,
                           maxWidthMultiplier);
    // 3) Dock search properties
    ResizeHelperDockWidget(ui.dockSearchProperties, dockSearchPropertiesHeight,
                           helperWidgetsWidth, minHeightMultiplier, minWidthMultiplier,
                           maxHeightMultiplier, maxWidthMultiplier);
    // 4) Dock saved requests
    ResizeHelperDockWidget(ui.dockSavedRequests, dockSavedRequestsHeight,
                           helperWidgetsWidth, minHeightMultiplier, minWidthMultiplier,
                           maxHeightMultiplier, maxWidthMultiplier);
    // 5) Dock trash
    ResizeHelperDockWidget(ui.dockTrash, dockTrashHeight, helperWidgetsWidth,
                           minHeightMultiplier, minWidthMultiplier, maxHeightMultiplier,
                           maxWidthMultiplier);

    // TODO: rearrange the position of each helper widget

    // TODO: change vertical sizes of notes table and note editor window
}

void MainWindow::resizeEvent(QResizeEvent * pResizeEvent)
{
    Q_CHECK_PTR(pResizeEvent);

    SetDefaultLayoutSettings();
}

void MainWindow::ResizeHelperDockWidget(QDockWidget * pDock, const int dockHeight,
                                        const int dockWidth, const double minHeightMultiplier,
                                        const double minWidthMultiplier,
                                        const double maxHeightMultiplier,
                                        const double maxWidthMultiplier)
{
#ifndef QT_NO_DEBUG
    if (pDock == nullptr) {
        qDebug() << "MainWindow::ResizeHelperDockWidget: found NULL dock widget pointer!";
        return;
    }
#endif

    QDockWidget & rDock = *pDock;

    int dockOldHeight = pDock->height();
    int dockOldWidth = pDock->width();

    if ( (std::abs(dockOldWidth - dockWidth) < 1) &&
         (std::abs(dockOldHeight - dockHeight) < 1) )
    {
        rDock.resize(dockWidth, dockHeight);
    }

    rDock.setMaximumHeight(std::floor(dockHeight * maxHeightMultiplier + 0.5));
    rDock.setMaximumWidth(std::floor(dockWidth * maxWidthMultiplier + 0.5));
    rDock.setMinimumHeight(std::floor(dockHeight * minHeightMultiplier + 0.5));
    rDock.setMinimumWidth(std::floor(dockWidth * minWidthMultiplier + 0.5));
}
