# -*- coding: utf-8 -*-

"""
Module implementing MainWindow.
"""

from PyQt4.QtCore import (
    pyqtSignature,
    QObject,
    Qt,
    SIGNAL,
    )
from PyQt4.QtGui import (
    QMainWindow,
    QMessageBox,
    QProgressDialog,
    )

import sys
import threading

from Ui_mainWindow import Ui_MainWindow
from addMTModel import AddMTModelDialog
from chooseMTModel import ChooseMTModelDialog
from engine import Engine
from credits import DlgCredits
from util import doAlert


class MainWindow(QMainWindow, Ui_MainWindow):
    """
    Class documentation goes here.
    """
    def setupUi(self, mainWindow):
        super(MainWindow, self).setupUi(mainWindow)
        self.tableView.setModel(self.datamodel)
        self.tableView.hideColumn(0)
        # Change status and keep the column.
        QObject.connect(
            self.datamodel, SIGNAL("recordUpdated(bool)"),
            self.on_datamodel_recordUpdated)
        QObject.connect(
            self.datamodel, SIGNAL("messageBox(QString)"),
            self.on_datamodel_messageBox)
        # The response to change model.
        for obj in (self.editModelName, self.editSrcLang, self.editTrgLang):
            obj.installEventFilter(self)

    def __init__(self, parent=None,  dm=None, moses=None, workdir=None):
        """
        Constructor
        """
        QMainWindow.__init__(self, parent)
        self.moses = moses
        self.datamodel = dm
        self.engine = None
        self.progress = None
        self.workdir = workdir

    @pyqtSignature("")
    def on_delModelBtn_clicked(self):
        """
        Slot documentation goes here.
        """
        current = self.tableView.currentIndex()
        if not current or current.row() < 0:
            return
        model_in_use = (
            self.engine and
            self.datamodel.getRowID(current.row()) == self.engine.model['ID']
            )
        if model_in_use:
            text = (
                "The model is still in use, do you want to "
                "stop and delete it?\n"
                "It might take a while..."
                )
            reply = QMessageBox.question(
                None, 'Message', text, QMessageBox.Yes, QMessageBox.No)
            if reply == QMessageBox.No:
                return
            t = self.stopEngine(self.engine)
            t.join()
            self.engine = None
            self.clearPanel()
        self.datamodel.delModel(current.row())

    @pyqtSignature("")
    def on_newModelBtn_clicked(self):
        """
        Slot documentation goes here.
        """
        dialog = AddMTModelDialog(self, self.workdir)
        if dialog.exec_():
            installParam = {
                'modelName': dialog.modelName,
                'source': dialog.source,
                'sourceMode': dialog.sourceMode,
                'dest': dialog.dest,
            }
            self.datamodel.installModel(installParam)
            self.tableView.selectRow(self.tableView.model().rowCount() - 1)
        # self.datamodel.newEntry()

    def on_datamodel_recordUpdated(self,  bRecord):
        # Deal with the selection changed problem.
        try:
            if bRecord:
                current = self.tableView.currentIndex()
                if current and current.row() != -1:
                    self.curSelection = current.row()
                else:
                    self.curSelection = None
            else:
                if self.curSelection is not None:
                    self.tableView.selectRow(self.curSelection)
        except Exception, e:
            print >> sys.stderr, str(e)

    def on_datamodel_messageBox(self, str):
        doAlert(str)

    def closeEvent(self, event):
        # Clear up.
        if self.datamodel.destroy():
            event.accept()
        else:
            event.reject()

    def eventFilter(self, obj, event):
        for obj in (self.editModelName, self.editSrcLang, self.editTrgLang):
            if self.gridLayout.indexOf(obj) != -1:
                if event.type() == event.MouseButtonPress:
                    dialog = ChooseMTModelDialog(self, self.datamodel)
                    if dialog.exec_():
                        # Get the model.
                        model = {
                            'ID': dialog.ID,
                            'name': dialog.modelName,
                            'srclang': dialog.srcLang,
                            'trglang': dialog.trgLang,
                            'path': dialog.path,
                            'mosesini': dialog.mosesini,
                        }
                        self.startEngine(model)
                    return True  # We handle it here.
        return super(MainWindow, self).eventFilter(obj, event)

    def stopEngine(self, engine):
        # Stop the engine with another thread.
        def stopEngineThread():
            engine.stop()
        t = threading.Thread(target=stopEngineThread)
        t.start()
        return t

    def startEngine(self, model):
        self.editModelName.setText(model['name'])
        self.editSrcLang.setText(model['srclang'])
        self.editTrgLang.setText(model['trglang'])
        self.editSrc.setText("")
        self.editTrg.setText("")
        try:
            if self.engine:
                self.stopEngine(self.engine)
                self.engine = None
            # Create engine.
            self.engine = Engine(model, self.moses)

            # Create progress bar dialog.
            if self.progress:
                self.progress.close()
                self.progress = None
            self.progress = QProgressDialog(
                "Model: %s" % model['name'], "Cancel", 0,
                self.engine.countSteps(), self)
            self.progress.setAutoReset(True)
            self.progress.setAutoClose(True)
            self.progress.setWindowModality(Qt.WindowModal)
            self.progress.setWindowTitle('Loading Model...')
            QObject.connect(
                self.progress, SIGNAL("canceled()"), self.progressCancelled)
            self.progress.show()

            # Connect engine signal.
            QObject.connect(
                self.engine, SIGNAL("stepFinished(int)"),
                self.engineStepFinished)
            QObject.connect(
                self.engine, SIGNAL("loaded(bool, QString)"),
                self.engineLoaded)

            def startEngineThread():
                self.engine.start()
            t = threading.Thread(target=startEngineThread)
            t.start()
        except Exception, e:
            if self.engine:
                self.stopEngine(self.engine)
                self.engine = None
            self.clearPanel()
            doAlert("Error start MT engine: " + str(e))

    def clearPanel(self):
        if self.engine:
            self.stopEngine(self.engine)
            self.engine = None
        self.editModelName.setText("")
        self.editSrcLang.setText("")
        self.editTrgLang.setText("")
        self.editSrc.setText("")
        self.editTrg.setText("")

    def progressCancelled(self):
        self.clearPanel()
        if self.engine:
            self.stopEngine(self.engine)
            self.engine = None
        if self.progress:
            self.progress = None

    def engineStepFinished(self, nStep):
        if self.progress:
            self.progress.setValue(nStep)

    def engineLoaded(self, success, message):
        if not success:
            self.clearPanel()
            if message:
                doAlert(message)
        else:
            if self.progress:
                self.progress.setValue(self.progress.maximum())
                self.progress = None

    @pyqtSignature("")
    def on_btnTranslate_clicked(self):
        """
        Slot documentation goes here.
        """
        if self.engine is None:
            doAlert("Please load MT model first.")
            return
        self.btnTranslate.setEnabled(False)
        self.editTrg.setText("")
        try:
            texts = str(self.editSrc.toPlainText().toUtf8()).split('\n')
            trans = []
            for text in texts:
                if text.strip() == "":
                    trans.append(text)
                else:
                    trans.append(
                        self.engine.translate(
                            text.replace('\r', ' ').strip()).decode('utf8'))
            self.editTrg.setText('\n'.join(trans))
        except Exception, e:
            print >> sys.stderr, str(e)
            doAlert("Translation failed!")
        self.btnTranslate.setEnabled(True)
        self.btnTranslate.setFocus()

    @pyqtSignature("QString")
    def on_labelInfo_linkActivated(self, link):
        """
        Slot documentation goes here.
        """
        dialog = DlgCredits(self)
        dialog.exec_()
