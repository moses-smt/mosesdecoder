#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
from PyQt4 import QtCore, QtGui

import DIMterface as my_gui



if __name__ == "__main__":
    app = QtGui.QApplication(sys.argv)
    wnd = my_gui.MainWindow()
    wnd.resize(640, 480)
    wnd.setWindowTitle("DIMwid")
    wnd.show()
    sys.exit(app.exec_())
