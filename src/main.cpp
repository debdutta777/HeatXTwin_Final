#include "app/ui/MainWindow.hpp"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  
  app.setApplicationName("HeatXTwin Pro");
  app.setApplicationVersion("2.0");
  app.setOrganizationName("Digital Twin Lab");
  
  MainWindow window;
  window.show();
  
  return app.exec();
}
