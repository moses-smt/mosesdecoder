# -*- coding: utf-8 -*-

"""
Module implementing Dialog.
"""

from PyQt4.QtGui import (
    QDialog,
    QFileDialog,
    )
from PyQt4.QtCore import pyqtSignature

import datetime
import os

from Ui_addMTModel import Ui_Dialog
from util import doAlert


class AddMTModelDialog(QDialog, Ui_Dialog):
    """
    Class documentation goes here.
    """
    def __init__(self, parent=None, workdir=None):
        """
        Constructor
        """
        QDialog.__init__(self, parent)
        self.setupUi(self)
        self.timestr = None
        self.workdir = workdir
        todir, timestr = self.findEmptyDirWithTime(self.workdir)
        self.editPath.setText(todir)
        self.editName.setText("SampleModel_" + timestr)

    def findEmptyDirWithTime(self, workdir):
        if not self.timestr:
            self.timestr = datetime.datetime.now().strftime('%Y-%m-%d_%H%M%S')
        while True:
            todir = os.path.join(workdir,  "Model_%s" % self.timestr)
            if not os.path.exists(todir):
                break
            self.timestr = datetime.datetime.now().strftime('%Y-%m-%d_%H%M%S')
        return todir, self.timestr

    @pyqtSignature("")
    def on_btnLocal_clicked(self):
        """
        Slot documentation goes here.
        """
        dialog = QFileDialog(self)
        dialog.setFileMode(QFileDialog.ExistingFile)
        dialog.setNameFilter("Zipped model files (*.zip)")
        dialog.setViewMode(QFileDialog.Detail)
        if dialog.exec_():
            self.editLocal.setText(dialog.selectedFiles()[0])

    @pyqtSignature("")
    def on_btnPath_clicked(self):
        """
        Slot documentation goes here.
        """
        dialog = QFileDialog(self, directory=self.workdir)
        dialog.setFileMode(QFileDialog.Directory)
        dialog.setViewMode(QFileDialog.Detail)
        dialog.setOptions(QFileDialog.ShowDirsOnly)
        if dialog.exec_():
            root = str(dialog.selectedFiles()[0])
            todir, _ = self.findEmptyDirWithTime(root)
            self.editPath.setText(todir)

    @pyqtSignature("bool")
    def on_grpBoxInternet_toggled(self, p0):
        """
        Slot documentation goes here.
        """
        self.grpBoxLocal.setChecked(not p0)

    @pyqtSignature("bool")
    def on_grpBoxLocal_toggled(self, p0):
        """
        Slot documentation goes here.
        """
        self.grpBoxInternet.setChecked(not p0)

    @pyqtSignature("")
    def on_buttonBox_accepted(self):
        """
        Slot documentation goes here.
        """
        def checkEmpty(mystr):
            return len(str(mystr).strip()) <= 0

        # Check everything.
        self.modelName = self.editName.text()
        if checkEmpty(self.modelName):
            doAlert("Please provide non-empty Model Name")
            return
        if self.grpBoxInternet.isChecked():
            self.source = self.editInternet.text()
            self.sourceMode = "Internet"
        elif self.grpBoxLocal.isChecked():
            self.source = self.editLocal.text()
            self.sourceMode = "Local"
            if not os.path.exists(str(self.source)):
                doAlert("Please provide valid local file as source")
                return
        else:
            doAlert("Please provide Install Source")
            return
        if checkEmpty(self.source):
            doAlert("Please provide non-empty Install Source")
            return
        self.dest = self.editPath.text()
        if checkEmpty(self.dest):
            doAlert("Please provide non-empty Install Destination Folder")
            return
        self.accept()
