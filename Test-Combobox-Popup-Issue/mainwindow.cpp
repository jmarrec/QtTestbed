#include "mainwindow.h"

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QScrollArea>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setObjectName("MainWindow");
    auto * centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    setCentralWidget(centralWidget);

    auto * mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setObjectName("mainLayout");
    mainLayout->setSpacing(0);
    // Already done in ctor
    // centralWidget->setLayout(mainLayout);

    auto * combo = new QComboBox(this);
    combo->setObjectName("combo");
    combo->addItem("");
    combo->addItem("Item 1");
    combo->addItem("A very long item that is likely going to extend past the size of the combobox");

    mainLayout->addWidget(combo);
}

MainWindow::~MainWindow()
{
}

