#include <QApplication>
#include "mainwindow.h"

auto main(int argc, char** argv)->int {
    auto app = QApplication{ argc, argv };
    auto w = MainWindow{};
    w.show();
    return app.exec();
}