# -*- coding: utf-8 -*-

"""
Module implementing DlgCredits.
"""

from PyQt4.QtGui import QDialog, QDesktopServices
from PyQt4.QtCore import pyqtSignature, QUrl

from Ui_credits import Ui_Dialog


class DlgCredits(QDialog, Ui_Dialog):
    """
    Class documentation goes here.
    """
    def __init__(self, parent=None):
        """
        Constructor
        """
        QDialog.__init__(self, parent)
        self.setupUi(self)

    @pyqtSignature("QString")
    def on_label_linkActivated(self, link):
        """
        Slot documentation goes here.
        """
        QDesktopServices().openUrl(QUrl(link))
