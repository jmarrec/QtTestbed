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
    auto * centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto * mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    centralWidget->setLayout(mainLayout);

    auto * combo = new QComboBox(this);
    combo->addItem("");
    combo->addItem("Item 1");
    combo->addItem("A very long item that is likely going to extend past the size of the combobox");

    mainLayout->addWidget(combo);
}

MainWindow::~MainWindow()
{
}

