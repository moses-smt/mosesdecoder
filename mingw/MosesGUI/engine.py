# -*- coding: utf-8 -*-

from PyQt4.QtCore import *

import os, subprocess, sys, signal, ConfigParser

from moses import Moses
from util import *

class Engine(QObject):
    def __init__(self, model, moses):
        super(Engine, self).__init__()
        self.model = model
        self.moses = moses
        self.check(self.model)
        #determine how many steps by num of modules in the model directory (moses + tok/detok + ...) + 1
        modelini = model['path'] + os.sep + 'model' + os.sep + 'model.ini'
        if not os.path.exists(modelini):
            raise Exception, "Model ini file doesn't exist, please check model dir %s" % modelini       
        cp = ConfigParser.RawConfigParser()
        cp.readfp(open(modelini))
        #true model
        truemodel = None
        try:
            truemodel = cp.get("Preprocess", 'Truemodel')
            if not os.path.exists(model['path'] + os.sep + 'model' + os.sep + truemodel):
                doAlert("Truemodel doesn't exist, ignore %s" % truemodel)
                truemodel = None
        except: 
            #doAlert("Truemodel not configured")
            truemodel = None
        self.cmds = []
        #tok
        self.cmds.append('"%s" -q -l %s -noxml' % (self.moses.getTokenizer(), str(model['srclang']).lower()))
        if not truemodel is None:
            self.cmds.append('"%s" -model "%s"' % (self.moses.getTruecase(), truemodel) )
        self.cmds.append('"%s" -f moses.ini' % self.moses.getMosesCmd())
        self.cmds.append('"%s"' % self.moses.getDetruecase())
        self.cmds.append('"%s" -q -noxml -l %s' % (self.moses.getDetokenizer(), str(model['trglang']).lower()))
        self.process = []
        #doAlert('\n'.join(self.cmds))
        
    def check(self, model):
        if not os.path.exists(model['path']) or not os.path.exists(model['mosesini']):
            raise Exception, "Model file doesn't exist, please check model dir %s" % self.model['path']
        
    def countSteps(self):
        return len(self.cmds) + 1
        
    def start(self):
        #print >> sys.stderr, self.cmds
        try:
            self.emit(SIGNAL("stepFinished(int)"), 0)
            for i, cmd in enumerate(self.cmds):
                proc = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, cwd=os.path.join(str(self.model['path']), 'model'))
                self.process.append(proc)
                if not proc.poll() is None:
                    raise Exception, "Failed to start engine!"
                proc.stdin.write("dummy\n")
                proc.stdin.flush()
                if len(proc.stdout.readline().strip()) <= 0:
                    raise Exception, "Engine process exited: [%s] in folder [%s]" % (cmd, os.path.join(str(self.model['path']), 'model'))
                self.emit(SIGNAL("stepFinished(int)"), i+1)
            self.emit(SIGNAL("loaded(bool, QString)"), True, "Model Loaded")
        except Exception, e:
            self.emit(SIGNAL("loaded(bool, QString)"), False, "Failed to load Model: %s" % str(e))
            
    def stop(self):
        for process in self.process:
            #doAlert(str(process.pid))
            #print >> sys.stderr, str(process)
            process.terminate()
            process.wait()
        self.process = []
        
    def translate(self, input):
        lastInput = input
        try:
            for i, proc in enumerate(self.process):
                if not proc.poll() is None:
                    raise Exception, "Failed to start engine!"
                proc.stdin.write("%s\n" % lastInput)
                proc.stdin.flush()
                output = proc.stdout.readline().strip()
                lastInput = output
            return output
        except Exception, e:
            print >> sys.stderr, "Translate error: %s" % str(e)
            return lastInput
