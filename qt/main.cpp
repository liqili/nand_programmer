/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "main_window.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    if (QSysInfo::productType() == "osx") {
        // Force non-native menu bar on macOS
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
    }
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
