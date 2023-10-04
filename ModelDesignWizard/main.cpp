#include "MainWindow.hpp"
#include "ModelDesignWizardDialog.hpp"

#include <QApplication>
#include <QFile>
#include <QDebug>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  QFile data(":/app.qss");
  QString style;
  if (data.open(QFile::ReadOnly)) {
    QTextStream styleIn(&data);
    style = styleIn.readAll();
    data.close();
    a.setStyleSheet(style);
  } else {
    qCritical() << "Failed to open openstudiolib.qss";
  }

  MainWindow w;
  w.show();
  openstudio::ModelDesignWizardDialog dlg;
  dlg.exec();
  return a.exec();
}
