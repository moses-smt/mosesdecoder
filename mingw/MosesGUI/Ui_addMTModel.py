# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'C:\work\eric4workspace\MosesGUI\addMTModel.ui'
#
# Created: Thu Jul 11 13:38:49 2013
#      by: PyQt4 UI code generator 4.10.2
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui


_fromUtf8 = getattr(QtCore.QString, 'fromUtf8', lambda s: s)


def _translate(context, text, disambig):
    return QtGui.QApplication.translate(
        context, text, disambig,
        getattr(
            QtGui.QApplication, 'UnicodeUTF8',
            QtCore.QCoreApplication.Encoding))


class Ui_Dialog(object):
    def setupUi(self, Dialog):
        Dialog.setObjectName(_fromUtf8("Dialog"))
        Dialog.resize(494, 342)
        Dialog.setLocale(QtCore.QLocale(QtCore.QLocale.English, QtCore.QLocale.UnitedKingdom))
        Dialog.setWindowFilePath(_fromUtf8(""))
        self.verticalLayout = QtGui.QVBoxLayout(Dialog)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.groupBox_3 = QtGui.QGroupBox(Dialog)
        self.groupBox_3.setObjectName(_fromUtf8("groupBox_3"))
        self.gridLayout = QtGui.QGridLayout(self.groupBox_3)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.label = QtGui.QLabel(self.groupBox_3)
        self.label.setObjectName(_fromUtf8("label"))
        self.gridLayout.addWidget(self.label, 0, 0, 1, 1)
        self.editName = QtGui.QLineEdit(self.groupBox_3)
        self.editName.setObjectName(_fromUtf8("editName"))
        self.gridLayout.addWidget(self.editName, 0, 1, 1, 3)
        self.verticalLayout.addWidget(self.groupBox_3)
        self.groupBox = QtGui.QGroupBox(Dialog)
        self.groupBox.setLocale(QtCore.QLocale(QtCore.QLocale.English, QtCore.QLocale.UnitedKingdom))
        self.groupBox.setObjectName(_fromUtf8("groupBox"))
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.groupBox)
        self.verticalLayout_2.setObjectName(_fromUtf8("verticalLayout_2"))
        self.grpBoxInternet = QtGui.QGroupBox(self.groupBox)
        self.grpBoxInternet.setCheckable(True)
        self.grpBoxInternet.setObjectName(_fromUtf8("grpBoxInternet"))
        self.verticalLayout_3 = QtGui.QVBoxLayout(self.grpBoxInternet)
        self.verticalLayout_3.setObjectName(_fromUtf8("verticalLayout_3"))
        self.editInternet = QtGui.QLineEdit(self.grpBoxInternet)
        self.editInternet.setObjectName(_fromUtf8("editInternet"))
        self.verticalLayout_3.addWidget(self.editInternet)
        self.verticalLayout_2.addWidget(self.grpBoxInternet)
        self.grpBoxLocal = QtGui.QGroupBox(self.groupBox)
        self.grpBoxLocal.setFlat(False)
        self.grpBoxLocal.setCheckable(True)
        self.grpBoxLocal.setChecked(False)
        self.grpBoxLocal.setObjectName(_fromUtf8("grpBoxLocal"))
        self.horizontalLayout = QtGui.QHBoxLayout(self.grpBoxLocal)
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        self.editLocal = QtGui.QLineEdit(self.grpBoxLocal)
        self.editLocal.setText(_fromUtf8(""))
        self.editLocal.setObjectName(_fromUtf8("editLocal"))
        self.horizontalLayout.addWidget(self.editLocal)
        self.btnLocal = QtGui.QPushButton(self.grpBoxLocal)
        self.btnLocal.setObjectName(_fromUtf8("btnLocal"))
        self.horizontalLayout.addWidget(self.btnLocal)
        self.horizontalLayout.setStretch(0, 9)
        self.horizontalLayout.setStretch(1, 1)
        self.verticalLayout_2.addWidget(self.grpBoxLocal)
        self.verticalLayout.addWidget(self.groupBox)
        self.groupBox_2 = QtGui.QGroupBox(Dialog)
        self.groupBox_2.setLocale(QtCore.QLocale(QtCore.QLocale.English, QtCore.QLocale.UnitedKingdom))
        self.groupBox_2.setObjectName(_fromUtf8("groupBox_2"))
        self.horizontalLayout_2 = QtGui.QHBoxLayout(self.groupBox_2)
        self.horizontalLayout_2.setObjectName(_fromUtf8("horizontalLayout_2"))
        self.editPath = QtGui.QLineEdit(self.groupBox_2)
        self.editPath.setObjectName(_fromUtf8("editPath"))
        self.horizontalLayout_2.addWidget(self.editPath)
        self.btnPath = QtGui.QPushButton(self.groupBox_2)
        self.btnPath.setObjectName(_fromUtf8("btnPath"))
        self.horizontalLayout_2.addWidget(self.btnPath)
        self.horizontalLayout_2.setStretch(0, 9)
        self.horizontalLayout_2.setStretch(1, 1)
        self.verticalLayout.addWidget(self.groupBox_2)
        self.buttonBox = QtGui.QDialogButtonBox(Dialog)
        self.buttonBox.setStandardButtons(QtGui.QDialogButtonBox.Cancel | QtGui.QDialogButtonBox.Ok)
        self.buttonBox.setObjectName(_fromUtf8("buttonBox"))
        self.verticalLayout.addWidget(self.buttonBox)
        self.verticalLayout.setStretch(1, 2)
        self.verticalLayout.setStretch(2, 1)
        self.verticalLayout.setStretch(3, 1)

        self.retranslateUi(Dialog)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL(_fromUtf8("rejected()")), Dialog.reject)
        QtCore.QMetaObject.connectSlotsByName(Dialog)
        Dialog.setTabOrder(self.editName, self.grpBoxInternet)
        Dialog.setTabOrder(self.grpBoxInternet, self.editInternet)
        Dialog.setTabOrder(self.editInternet, self.grpBoxLocal)
        Dialog.setTabOrder(self.grpBoxLocal, self.editLocal)
        Dialog.setTabOrder(self.editLocal, self.btnLocal)
        Dialog.setTabOrder(self.btnLocal, self.editPath)
        Dialog.setTabOrder(self.editPath, self.btnPath)
        Dialog.setTabOrder(self.btnPath, self.buttonBox)

    def retranslateUi(self, Dialog):
        Dialog.setWindowTitle(_translate("Dialog", "Install MT Model", None))
        self.groupBox_3.setTitle(_translate("Dialog", "MT Model Details", None))
        self.label.setText(_translate("Dialog", "Model Name:", None))
        self.editName.setText(_translate("Dialog", "SampleModel", None))
        self.groupBox.setTitle(_translate("Dialog", "Install From ...", None))
        self.grpBoxInternet.setTitle(_translate("Dialog", "From Internet ...", None))
        self.editInternet.setText(_translate("Dialog", "http://www.statmt.org/~jie/models/EnFr4MosesGUI.zip", None))
        self.grpBoxLocal.setTitle(_translate("Dialog", "From Local File", None))
        self.btnLocal.setText(_translate("Dialog", "...", None))
        self.groupBox_2.setTitle(_translate("Dialog", "Install To ...", None))
        self.btnPath.setText(_translate("Dialog", "...", None))


if __name__ == "__main__":
    import sys
    app = QtGui.QApplication(sys.argv)
    Dialog = QtGui.QDialog()
    ui = Ui_Dialog()
    ui.setupUi(Dialog)
    Dialog.show()
    sys.exit(app.exec_())
