#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Python utilities for moses
# 
# This package mostly wraps standard Moses utilities into pipes.
#
# Written by Ulrich Germann
# 
# This package borrows from scripts written by Christian Buck
# 
# The package assumes that there is a complete moses installation
# (including scripts) under one root directory,
# e.g., via 
#    bjam --with-xmlrpc-c=... [...] --install-scripts --prefix=${HOME}/moses
# By default, this root directory is "${HOME}/moses".

import xmlrpclib,datetime,argparse,time,os,sys
from subprocess import *
from unicodedata import normalize
 
moses_root = os.environ.get('MOSES_ROOT',os.environ.get('HOME')+"/moses")

class ProcessWrapper:

  def __init__(self,cmd=[]):
    self.process = None
    self.cmd = cmd
    return

  def start(self, stdin=PIPE, stdout=PIPE):
    if self.process:
      raise Exception("Process is already running")
    self.process = Popen(self.cmd, stdin = stdin, stdout = stdout)
    return

  def __del__(self):
    if self.process:
      self.process.terminate()
      pass
    return
  pass

class LineProcessor(ProcessWrapper):

  def __call__(self,input):
    if not self.process: self.start()
    self.process.stdin.write("%s\n"%input.strip())
    self.process.stdin.flush()
    return self.process.stdout.readline().strip()
  pass  

class SentenceSplitter(ProcessWrapper):
  """
  Wrapper for standard Moses sentence splitter
  """
  def __init__(self,lang):
    ssplit_cmd = moses_root+"/scripts/ems/support/split-sentences.perl"
    self.cmd = [ssplit_cmd, "-b", "-q", "-l",lang]
    self.process = None
    return

  def __call__(self,input):
    if not self.process:
      self.start()
      pass
    self.process.stdin.write(input.strip() + "\n<P>\n")
    self.process.stdin.flush()
    x = self.process.stdout.readline().strip()
    ret = []
    while x != '<P>' and x != '':
      ret.append(x)
      x = self.process.stdout.readline().strip()
      pass
    return ret

class Pretokenizer(LineProcessor):
  """
  Pretokenizer wrapper; the pretokenizer fixes known issues with the input.
  """
  def __init__(self,lang):
    pretok_cmd = moses_root+"/scripts/tokenizer/pre-tokenizer.perl"
    self.cmd = [pretok_cmd,"-b", "-q", "-l",lang]
    self.process = None
    return
  pass

class Tokenizer(LineProcessor):
  """
  Tokenizer wrapper; the pretokenizer fixes known issues with the input.
  """
  def __init__(self,lang,args=["-a","-no-escape"]):
    tok_cmd = moses_root+"/scripts/tokenizer/tokenizer.perl"
    self.cmd = [tok_cmd,"-b", "-q", "-l", lang] + args
    self.process = None
    return
   
class Truecaser(LineProcessor):
  """
  Truecaser wrapper.
  """
  def __init__(self,model):
    truecase_cmd = moses_root+"/scripts/recaser/truecase.perl"
    self.cmd = [truecase_cmd,"-b", "--model",model]
    self.process = None
    return
  pass

class LineProcessorPipeline:
  """
  Line processor: one line in, one line out
  """
  def __init__(self,parts=[]):
    self.chain = [LineProcessor(p.cmd) for p in parts]
    return 
  
  def start(self):
    if len(self.chain) == 0:
      return
    if self.chain[0].process:
      return
    self.chain[0].start()
    for i in xrange(1,len(self.chain)):
      self.chain[i].start(stdin = self.chain[i-1].process.stdout)
      pass
    return

  def __call__(self,input):
    if len(self.chain) == 0:
      return input
    self.start()
    self.chain[0].process.stdin.write("%s\n"%input.strip())
    self.chain[0].process.stdin.flush()
    return self.chain[0].process.stdout.readline().strip()

  pass

def find_free_port(p):
  """
  Find a free port, starting at /p/. 
  Return the free port, or False if none found.
  """
  ret = p
  while ret - p < 20:
    devnull = open(os.devnull,"w")
    n = Popen(["netstat","-tnp"],stdout=PIPE,stderr=devnull)
    if n.communicate()[0].find(":%d "%ret) < 0:
      return p
    ret += 1
    pass
  return False

class MosesServer(ProcessWrapper):

  def __init__(self,args=[]):
    self.process = None
    mserver_cmd  = moses_root+"/bin/mosesserver"
    self.cmd = [mserver_cmd] + args 
    self.url = None
    self.proxy = None
    return
  
  def start(self,config=None,args=[],port=7447,debug=False):
    self.cmd.extend(args)
    if config:
      if "-f" in args:
        raise Exception("Config file specified twice")
      else:
        self.cmd.extend(["-f",config])
        pass
      pass
    self.port = port # find_free_port(port)
    if not self.port:
      raise Excpetion("Cannot find free port for moses server!")
    self.cmd.extend(["--server-port", "%d"%self.port])
    if debug:
      print >>sys.stderr,self.cmd
      # self.stderr = open("mserver.%d.stderr"%self.port,'w')
      # self.stdout = open("mserver.%d.stdout"%self.port,'w')
      # self.process = Popen(self.cmd,stderr = self.stderr,stdout = self.stdout)
      self.process = Popen(self.cmd)
    else:
      devnull = open(os.devnull,"w")
      self.process = Popen(self.cmd, stderr=devnull, stdout=devnull)
      pass

    if self.process.poll():
      raise Exception("FATAL ERROR: Could not launch moses server!")
    if debug:
      print >>sys.stderr,"MOSES port is %d."%self.port 
      print >>sys.stderr,"Moses poll status is", self.process.poll()
      pass

    self.url = "http://localhost:%d/RPC2"%self.port
    self.connect(self.url)

    return True

  def connect(self,url):
    if url[:4]  != "http":  url = "http://%s"%url
    if url[-5:] != "/RPC2": url += "/RPC2"
    self.url = url
    self.proxy = xmlrpclib.ServerProxy(self.url)
    return

  def translate(self,input):
    attempts = 0
    while attempts < 100:
      try:
        if type(input) is unicode:
          # if the server does not expect unicode, provide a 
          # properly encoded string!
          param = {'text': input.strip().encode('utf8')}
          return self.proxy.translate(param)['text'].decode('utf8')

        elif type(input) is str:
          param = {'text': input.strip()}
          return self.proxy.translate(param)['text']

        elif type(input) is list:
          return [self.translate(x) for x in input]

        elif type(input) is dict:
          return self.proxy.translate(input)

        else:
          raise Exception("Can't handle input of this type!")

      except:
        attempts += 1
        print >>sys.stderr, "WAITING", attempts
        time.sleep(1)
        pass
      pass
    raise Exception("Translation request failed")
  pass

