#include "mainwindow.h"

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto * centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    centralWidget->setFixedWidth(165);

    auto * mainLayout = new QVBoxLayout(this);
    centralWidget->setLayout(mainLayout);

    auto * frame = new QFrame(this);
    auto * vbox = new QVBoxLayout();
    frame->setLayout(vbox);
    auto* label = new QLabel("My Label", parent);
    label->setWordWrap(true);

    auto * combo = new QComboBox(this);
    combo->addItem("");
    combo->addItem("Item 1");
    combo->addItem("A very long item that is likely going to extend past the size of the combobox");
    vbox->addWidget(label);
    vbox->addWidget(combo);

    mainLayout->addWidget(frame);

}

MainWindow::~MainWindow()
{
}

