# -*- coding: utf-8 -*-

import os
import platform
import sys
from PyQt4.QtGui import QFileDialog

from util import (
    doAlert,
    doQuestion,
    )


class Moses():
    def __init__(self):
        pass

    def findRegistryPath(self):
        import _winreg
        key = None
        path = None
        try:
            reg = _winreg.ConnectRegistry(None, _winreg.HKEY_CURRENT_USER)
            key = _winreg.OpenKey(
                reg, r'Software\Moses Core Team\MosesDecoder')
            value, type = _winreg.QueryValueEx(key, 'Path')
            path = value
        except Exception, e:
            print >> sys.stderr, str(e)
            return None
        finally:
            if key:
                _winreg.CloseKey(key)
        return path

    def checkMosesInstall(self):
        for func in (self.getMosesCmd, self.getTokenizer, self.getDetokenizer, self.getTruecase, self.getDetruecase):
            if not os.path.exists(func()):
                doAlert(
                    "Missing executables in Moses installation path [%s], "
                    "exit." % self.mosesPath)
                return False
        return True

    def detect(self):
        pf = platform.system()
        if pf == 'Windows':
            self.mosesPath = self.findRegistryPath()
            if self.mosesPath:
                return self.checkMosesInstall()
            else:
                if not doQuestion(
                    'Cannot find Moses installation, click "Yes" to '
                    'manually set the Moses path, click "No" to exit.'):
                    return False
            # If not found, use a dialog.
            startdir = 'C:\\'
            if "ProgramFiles(x86)" in os.environ:
                startdir = os.environ["ProgramFiles(x86)"]
            elif "ProgramFiles" in os.environ:
                startdir = os.environ["ProgramFiles"]
            else:
                pass
            dialog = QFileDialog(None, directory=startdir)
            dialog.setFileMode(QFileDialog.Directory)
            dialog.setViewMode(QFileDialog.Detail)
            dialog.setOptions(QFileDialog.ShowDirsOnly)
            if dialog.exec_():
                self.mosesPath = str(dialog.selectedFiles()[0])
                return self.checkMosesInstall()
            else:
                doAlert("Failed to find Moses Installation path, exit.")
                return False
        else:
            doAlert("Platform %s not supported yet" % pf)
            return False

    def getMosesCmd(self):
        return os.path.join(self.mosesPath, 'moses-cmd.exe')

    def getTokenizer(self):
        return os.path.join(self.mosesPath, 'tokenizer.exe')

    def getDetokenizer(self):
        return os.path.join(self.mosesPath, 'detokenizer.exe')

    def getTruecase(self):
        return os.path.join(self.mosesPath, 'truecase.exe')

    def getDetruecase(self):
        return os.path.join(self.mosesPath, 'detruecase.exe')
