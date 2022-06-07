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
    centralWidget->setFixedWidth(165);

    auto * mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    centralWidget->setLayout(mainLayout);

    auto * m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameStyle(QFrame::NoFrame);
    mainLayout->addWidget(m_scroll);
    centralWidget->setContentsMargins(0, 0, 0, 0);


    auto* layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* hlayout = new QHBoxLayout();
    hlayout->setSpacing(0);
    hlayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(hlayout);
    hlayout->addLayout(layout);


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

    layout->addWidget(frame);

}

MainWindow::~MainWindow()
{
}

