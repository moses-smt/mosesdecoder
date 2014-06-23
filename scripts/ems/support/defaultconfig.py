#!/usr/bin/env python2

#
# Version of ConfigParser which accepts default values
#


import ConfigParser


class Config:
  def __init__(self,filename):
    self.config = ConfigParser.SafeConfigParser()
    cfh = open(filename)
    self.config.readfp(cfh)
    cfh.close()

  def get(self,section,name,default=None):
    if default == None or self.config.has_option(section,name):
      return self.config.get(section,name)
    else:
      return default

  def getint(self,section,name,default=None):
    if default == None or self.config.has_option(section,name):
      return self.config.getint(section,name)
    else:
      return default


  def getboolean(self,section,name,default=None):
    if default == None or self.config.has_option(section,name):
      return self.config.getboolean(section,name)
    else:
      return default


  def getfloat(self,section,name,default=None):
    if default == None or self.config.has_option(section,name):
      return self.config.getfloat(section,name)
    else:
      return default


  def __str__(self):
    ret = ""
    for section in self.config.sections():
      for option in self.config.options(section):
        ret = ret + "%s:%s = %s\n" % (section,option,self.config.get(section,option))
    return ret



