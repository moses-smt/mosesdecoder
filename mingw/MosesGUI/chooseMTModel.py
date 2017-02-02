# -*- coding: utf-8 -*-

"""
Module implementing ChooseMTModelDialog.
"""

import sys

from PyQt4.QtCore import (
    pyqtSignature,
    QObject,
    SIGNAL,
    )
from PyQt4.QtGui import QDialog
from PyQt4.QtSql import QSqlQueryModel

from Ui_chooseMTModel import Ui_Dialog
from util import doAlert


class ChooseMTModelDialog(QDialog, Ui_Dialog):
    """
    Class documentation goes here.
    """
    def __init__(self, parent=None, datamodel=None):
        """
        Constructor
        """
        QDialog.__init__(self, parent)
        self.setupUi(self)
        self.model = QSqlQueryModel()
        self.selTableView.setModel(self.model)
        self.database = datamodel.getQSqlDatabase()
        self.updateModel()
        self.selTableView.hideColumn(0)
        self.selTableView.hideColumn(5)
        self.selTableView.hideColumn(6)
        # Change status and keep the column.
        QObject.connect(
            datamodel,  SIGNAL("modelInstalled()"),
            self.on_datamodel_modelInstalled)

    def updateModel(self):
        self.model.setQuery(
            'SELECT ID, name, srclang, trglang, status, path, mosesini '
            'FROM models '
            'WHERE status = "READY" AND deleted != "True"',
            self.database)

    def on_datamodel_recordUpdated(self,  bRecord):
        """Deal with the selection changed problem."""
        try:
            if bRecord:
                current = self.selTableView.currentIndex()
                if current and current.row() != -1:
                    self.curSelection = current.row()
                else:
                    self.curSelection = None
            else:
                if self.curSelection is not None:
                    self.selTableView.selectRow(self.curSelection)
        except Exception as e:
            print >> sys.stderr, str(e)

    def on_datamodel_modelInstalled(self):
        self.updateModel()

    @pyqtSignature("")
    def on_buttonBox_accepted(self):
        """
        Slot documentation goes here.
        """
        current = self.selTableView.currentIndex()
        if not current:
            doAlert("Please choose a model to start.")
            return
        record = self.model.record(current.row())
        self.ID = record.value("ID").toString()
        self.modelName = record.value("name").toString()
        self.srcLang = record.value('srclang').toString()
        self.trgLang = record.value('trglang').toString()
        self.path = record.value("path").toString()
        self.mosesini = record.value("mosesini").toString()
        self.accept()
