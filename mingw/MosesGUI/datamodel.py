# -*- coding: utf-8 -*-

from PyQt4.QtCore import (
    QDateTime,
    SIGNAL,
    )
from PyQt4.QtGui import (
    QMessageBox,
    )
from PyQt4.QtSql import (
    QSqlDatabase,
    QSqlQuery,
    QSqlTableModel,
    QVariant,
    )

import ConfigParser
import os
import shutil
import sys
import threading
import urllib2
import zipfile

from util import (
    doAlert,
    doQuestion,
    )


class DataModel(QSqlTableModel):
    defaultDbFile = os.path.join(
        os.path.split(os.path.realpath(__file__))[0],  "models.sqlite")

    def __init__(self, parent=None,  filename=None):
        self.installThreads = {}
        self.processes = set()
        if filename is None:
            filename = DataModel.defaultDbFile
        self.db = QSqlDatabase.addDatabase('QSQLITE')
        print >> sys.stderr, "Open database at %s" % filename
        self.db.setDatabaseName(filename)
        self.db.open()
        query = QSqlQuery(
            'SELECT COUNT(*) '
            'FROM sqlite_master '
            'WHERE type="table" AND tbl_name="models"',
            self.db)
        if not query.next() or query.value(0).toInt()[0] < 1:
            # Create new table.
            print >> sys.stderr,  "Table not find, create the table"
            query = QSqlQuery(
                'CREATE TABLE models ('
                'ID INTEGER PRIMARY KEY AUTOINCREMENT, '
                'name TEXT, '
                'status TEXT, '
                'srclang TEXT, '
                'trglang TEXT, '
                'date DATE, '
                'path TEXT, '
                'mosesini TEXT, '
                'origin TEXT, '
                'originMode TEXT, '
                'deleted TEXT)',
                self.db)
            if query.next():
                print >> sys.stderr,  query.value(0).toString()
        # TODO: shoudn't design the deletion checking like this
        # Change all deleted models into not deleted in case it failed last
        # time.
        query = QSqlQuery(
            'UPDATE models SET deleted="False" WHERE deleted="True"', self.db)
        query = QSqlQuery(
            'UPDATE models SET status="READY" WHERE status="ON"', self.db)
        super(DataModel, self).__init__(parent,  self.db)
        self.setTable("models")
        self.select()
        self.setEditStrategy(QSqlTableModel.OnFieldChange)

    def destroy(self):
        bExit = False
        for i in self.installThreads:
            t,  flag = self.installThreads[i]
            if t.isAlive() and flag:
                if not bExit:
                    if not doQuestion(
                        "Installing process is running in the background, "
                        "do you want to terminate them and exit?"):
                        return False
                    else:
                        bExit = True
                self.installThreads[i][1] = False
                t.join()
        if self.db:
            self.db.close()
            self.db = None
        return True

    def getQSqlDatabase(self):
        return self.db

    def getRowID(self, row):
        record = self.record(row)
        return record.value('ID')

    def delModel(self, row):
        record = self.record(row)
        if str(record.value('deleted').toString()) == 'True':
            self.emit(
                SIGNAL("messageBox(QString)"),
                "The model is deleting, please be patient!")
            return
        # Hint to decide what to delete.
        text = '''You are going to delete the selected model entry.
Do you also want to delete all the model files on the disk?
Click "Yes" to delete model entry and model files.
Click "No" to delete model entry but keep model files.
Click "Cancel" to do nothing.'''
        reply = QMessageBox.question(
            None, 'Message', text, QMessageBox.Yes, QMessageBox.No,
            QMessageBox.Cancel)

        if reply == QMessageBox.Cancel:
            return
        else:
            record.setValue('deleted', 'True')
            self.changeRecord(row, record)

            def delModelThread():
                irowid, _ = record.value("ID").toInt()
                if irowid in self.installThreads:
                    t,  flag = self.installThreads[irowid]
                    if t.isAlive() and flag:
                        self.installThreads[irowid][1] = False
                        t.join()
                if reply == QMessageBox.Yes:
                    destDir = str(record.value("path").toString())
                    try:
                        shutil.rmtree(destDir)
                    except Exception as e:
                        self.emit(
                            SIGNAL("messageBox(QString)"),
                            "Failed to remove dir: " + destDir)
                        print >> sys.stderr, str(e)
                self.removeRow(row)
                # End of Model deleting thread.

            t = threading.Thread(target=delModelThread)
            t.start()

    def newEntry(self):
        import random
        rec = self.record()
        for i in xrange(1, 10):
            rec.setValue(i,  QVariant(str(random.random())))
        self.insertRecord(-1,  rec)
        doAlert(self.query().lastInsertId().toString())

    def changeRecord(self, curRow, record):
        # self.emit(SIGNAL("recordUpdated(bool)"),  True) #record selection
        self.setRecord(curRow,  record)
        # self.emit(SIGNAL("recordUpdated(bool)"),  False) #restore selection

    def installModel(self,  installParam):
        dest = installParam['dest']
        # Make dir.
        if not os.path.exists(dest):
            try:
                os.makedirs(str(dest))
            except:
                doAlert("Failed to create install directory: %s" % dest)
                return
        # Create entry in db.
        rec = self.record()
        rec.setValue('name',  installParam['modelName'])
        rec.setValue('status',  'Fetching Source...')
        rec.setValue('path',  dest)
        rec.setValue('origin',  installParam['source'])
        rec.setValue('originMode',  installParam['sourceMode'])
        rec.setValue('date',  QDateTime.currentDateTime())
        rec.setValue('deleted', 'False')
        self.insertRecord(-1,  rec)
        rowid = self.query().lastInsertId()

        # Start thread.
        def installThread(irowid):

            # Find the current row in model.
            def updateRecord(keyvalues):
                curRow = None
                # TODO: use binary search instead of linear
                for i in xrange(0,  self.rowCount()):
                    if self.record(i).value("ID") == rowid:
                        curRow = i
                        break
                if curRow is not None:
                    record = self.record(curRow)
                    for key in keyvalues:
                        record.setValue(key,  keyvalues[key])
                    self.changeRecord(curRow, record)
                return curRow

            def checkExit():
                # Check thread is ok to run.
                if irowid not in self.installThreads or not self.installThreads[irowid][1]:
                    return True
                else:
                    return False

            def markExit():
                if irowid in self.installThreads:  # Set thread to dead.
                    self.installThreads[irowid][1] = False

            def statusMessageLogMarkExit(status=None, message=None,
                                         exception=None):
                if status is not None:
                    updateRecord({'status': status})
                if message is not None:
                    self.emit(SIGNAL("messageBox(QString)"), message)
                    print >> sys.stderr, message
                if exception is not None:
                    print >> sys.stderr, str(exception)
                markExit()

            # 1. Download or copy from local.
            # Where the downloaded/copied zip file is:
            destFile = os.path.join(str(dest),  "model.zip")
            # Where the unzipped contents are:
            destDir = os.path.join(str(dest), "model")

            if installParam['sourceMode'] == 'Local':
                fin = fout = None
                try:
                    inFile = str(installParam['source'])
                    total_size = os.path.getsize(inFile)
                    fin = open(inFile, 'rb')
                    chunk_size = 52428800  # 50MB as chunk size
                    fout = open(destFile, 'wb')
                    content = fin.read(chunk_size)
                    download_size = content_size = len(content)
                    lastMsg = ""
                    while content_size > 0:
                        # Check if thread is notified as exit.
                        if checkExit():
                            return statusMessageLogMarkExit()
                        fout.write(content)
                        if total_size > 0:
                            msg = 'COPY %.0f%%' % (
                                download_size * 100.0 / total_size)
                        else:
                            msg = 'COPY %d MB' % (download_size / 1048576)
                        if msg != lastMsg:
                            updateRecord({'status': msg})
                            lastMsg = msg
                        content = fin.read(chunk_size)
                        content_size = len(content)
                        download_size += content_size
                except Exception as e:
                    return statusMessageLogMarkExit(
                        status=(
                            'Failed copying from: %s'
                            % installParam['source']),
                        message=(
                            "Failed copy model: %s"
                            % installParam['modelName']),
                        exception=e)
                finally:
                    if fin:
                        fin.close()
                    if fout:
                        fout.close()

            elif installParam['sourceMode'] == 'Internet':
                conn = fout = None
                try:
                    conn = urllib2.urlopen(str(installParam['source']))
                    total_size = int(conn.headers['Content-Length'])
                    chunk_size = 1048576  # 1MB as chunk size
                    fout = open(destFile, 'wb')
                    content = conn.read(chunk_size)
                    download_size = content_size = len(content)
                    lastMsg = ""
                    while content_size > 0:
                        # Check if thread is notified as exit.
                        if checkExit():
                            return statusMessageLogMarkExit()
                        fout.write(content)
                        if total_size > 0:
                            msg = 'DOWNLOAD %.0f%%' % (
                                download_size * 100.0 / total_size)
                        else:
                            msg = 'DOWNLOAD %d MB' % (download_size / 1048576)
                        if msg != lastMsg:
                            updateRecord({'status': msg})
                            lastMsg = msg
                        content = conn.read(chunk_size)
                        content_size = len(content)
                        download_size += content_size
                except Exception as e:
                    return statusMessageLogMarkExit(
                        status=(
                            'Failed downloading from: %s'
                            % installParam['source']),
                        message=(
                            "Failed download model: %s"
                            % installParam['modelName']),
                        exception=e)
                finally:
                    if conn:
                        conn.close()
                    if fout:
                        fout.close()
            else:
                return statusMessageLogMarkExit(
                    status='Unsupported source mode: %s'
                    % installParam['sourceMode'])

            # 2. Unzip.
            zfile = fout = None
            try:
                zfile = zipfile.ZipFile(destFile)
                # Check property files.
                if "model.ini" not in zfile.namelist():
                    return statusMessageLogMarkExit(
                        status=(
                            'Missing model.ini in model file: %s'
                            % installParam['sourceMode']),
                        message=(
                            "Invalid modle file format because model.ini "
                            "is missing in the zipped model file, exit "
                            "installation for model %s"
                            % installParam['modelName']))
                chunk_size = 52428800  # 50MB as chunk size
                # Get file size uncompressed.
                total_size = 0
                for name in zfile.namelist():
                    total_size += zfile.getinfo(name).file_size
                download_size = 0
                lastMsg = ""
                for i, name in enumerate(zfile.namelist()):
                    (dirname, filename) = os.path.split(name)
                    outDir = os.path.join(destDir, dirname)
                    if not os.path.exists(outDir):
                        os.makedirs(outDir)
                    if filename:
                        fin = zfile.open(name, 'r')
                        outFile = os.path.join(destDir,  name)
                        fout = open(outFile, 'wb')
                        content = fin.read(chunk_size)
                        content_size = len(content)
                        download_size += content_size
                        while content_size > 0:
                            # Check if thread is notified as exit.
                            if checkExit():
                                return statusMessageLogMarkExit()
                            fout.write(content)
                            if total_size > 0:
                                msg = 'UNZIP %.0f%%' % (
                                    download_size * 100.0 / total_size)
                            else:
                                msg = 'UNZIP %d MB' % (
                                    download_size / 1048576)
                            if msg != lastMsg:
                                updateRecord({'status': msg})
                                lastMsg = msg
                            content = fin.read(chunk_size)
                            content_size = len(content)
                            download_size += content_size
                        fin.close()
                        fout.close()
            except Exception as e:
                return statusMessageLogMarkExit(
                    status=(
                        'Failed unzipping from: %s' % installParam['source']),
                    message=(
                        "Failed unzip model: %s" % installParam['modelName']),
                    exception=e)
            finally:
                if zfile:
                    zfile.close()
                if fin:
                    fin.close()
                if fout:
                    fout.close()

            # 3 Post process and check data validity.
            try:
                modelini = os.path.join(destDir, "model.ini")
                cp = ConfigParser.RawConfigParser()
                cp.read(modelini)
                mosesini = os.path.join(destDir, 'moses.ini')
                if not os.path.exists(mosesini):
                    raise Exception("Moses ini doesn't exist")
                updateRecord({
                    'srclang': cp.get("Language", 'Source Language').upper(),
                    'trglang': cp.get("Language", 'Target Language').upper(),
                    'mosesini': mosesini},
                    )
            except Exception as e:
                return statusMessageLogMarkExit(
                    status='ERROR model format %s' % installParam['source'],
                    message=(
                        "Unspported model format: %s"
                        % installParam['modelName']),
                    exception=e)

            statusMessageLogMarkExit(
                status='READY',
                message="Model %s Installed!" % installParam['modelName'])
            # Send new model signal.
            self.emit(SIGNAL("modelInstalled()"))  # Record selection.
            return
            # End of thread func.

        # Start the thread.
        irowid, _ = rowid.toInt()
        t = threading.Thread(target=installThread,  args=(irowid, ))
        if irowid in self.installThreads:  # If old thread is there.
            print >> sys.stderr, (
                "table rowid %d already has a thread running, stop it"
                % irowid)
            self.installThreads[irowid][1] = False
        self.installThreads[irowid] = [t,  True]
        t.start()
