# -*- coding: utf_8 -*-
"""This program is used to prepare TMX files from corpora composed of 2 files for each language pair,
where the position of a segment in the first language file is exactly the same as in the second
language file.

The program requires that Pythoncard and wxPython (as well as Python) be previously installed.

Copyright 2009, 2010 João Luís A. C. Rosas

Distributed under GNU GPL v3 licence (see http://www.gnu.org/licenses/)

E-mail: joao.luis.rosas@gmail.com """

__version__ = "$Revision: 1.032$"
__date__ = "$Date: 2010/02/25$"
__author__="$João Luís A. C. Rosas$"

from PythonCard import clipboard, dialog, graphic, model
from PythonCard.components import button, combobox,statictext,checkbox,staticbox
import wx
import os, re
import string
import sys
from time import strftime
import codecs
import sys

class Moses2TMX(model.Background):

    def on_initialize(self, event):
        self.inputdir=''
        #Get directory where program file is and ...
        currdir=os.getcwd()
        #... load the file ("LanguageCodes.txt") with the list of languages that the program can process
        try:
            self.languages=open(currdir+r'\LanguageCodes.txt','r+').readlines()
        except:
            # If the languages file doesn't exist in the program directory, alert user that it is essential for the good working of the program and exit
            result = dialog.alertDialog(self, 'The file "LanguageCodes.txt" is missing. The program will now close.', 'Essential file missing')
            sys.exit()
        #remove end of line marker from each line in "LanguageCodes.txt"
        for lang in range(len(self.languages)):
            self.languages[lang]=self.languages[lang].rstrip()
        self.lang1code=''
        self.lang2code=''
        #Insert list of language names in appropriate program window's combo boxes
        self.components.cbStartingLanguage.items=self.languages
        self.components.cbDestinationLanguage.items=self.languages

    def CreateTMX(self, name):
        print 'Started at '+strftime('%H-%M-%S')
        #get the startinglanguage name (e.g.: "EN-GB") from the program window
        self.lang1code=self.components.cbStartingLanguage.text
        #get the destinationlanguage name from the program window
        self.lang2code=self.components.cbDestinationLanguage.text
        print name+'.'+self.lang2code[:2].lower()
        e=codecs.open(name,'r',"utf-8","strict")
        f=codecs.open(name+'.'+self.lang2code[:2].lower()+'.moses','r',"utf-8","strict")
        a=codecs.open(name+'.tmp','w',"utf-8","strict")
        b=codecs.open(name+'.'+self.lang2code[:2].lower()+'.moses.tmp','w',"utf-8","strict")
        for line in e:
            if line.strip():
                a.write(line)
        for line in f:
            if line.strip():
                b.write(line)
        a=codecs.open(name+'.tmp','r',"utf-8","strict")
        b=codecs.open(name+'.'+self.lang2code[:2].lower()+'.moses.tmp','r',"utf-8","strict")
        g=codecs.open(name+'.tmx','w','utf-16','strict')
        g.write('<?xml version="1.0" ?>\r\n<!DOCTYPE tmx SYSTEM "tmx14.dtd">\r\n<tmx version="version 1.4">\r\n\r\n<header\r\ncreationtool="moses2tmx"\r\ncreationtoolversion="1.032"\r\nsegtype="sentence"\r\ndatatype="PlainText"\r\nadminlang="EN-US"\r\nsrclang="'+self.lang1code+'"\r\n>\r\n</header>\r\n\r\n<body>\r\n')
        parar=0
        while True:
            self.ling1segm=a.readline().strip()
            self.ling2segm=b.readline().strip()
            if not self.ling1segm:
                break
            elif not self.ling2segm:
                break
            else:
                try:
                    g.write('<tu creationid="MT!">\r\n<prop type="Txt::Translator">Moses</prop>\r\n<tuv xml:lang="'+self.lang1code+'">\r\n<seg>'+self.ling1segm+'</seg>\r\n</tuv>\r\n<tuv xml:lang="'+self.lang2code+ \
                    '">\r\n<seg>'+self.ling2segm+'</seg>\r\n</tuv>\r\n</tu>\r\n\r\n')
                except:
                    pass
        a.close()
        b.close()
        e.close()
        f.close()
        g.write('</body>\r\n</tmx>\r\n')
        g.close()
        #os.remove(name)
        #os.remove(name+'.'+self.lang2code[:2].lower()+'.moses')
        os.remove(name+'.tmp')
        os.remove(name+'.'+self.lang2code[:2].lower()+'.moses.tmp')

    def createTMXs(self):
        try:
            # Get a list of all TMX files that need to be processed
            fileslist=self.locate('*.moses',self.inputdir)
        except:
            # if any error up to now, add the name of the TMX file to the output file @errors
            self.errortypes=self.errortypes+'   - Get All Segments: creation of output files error\n'
        if fileslist:
            # For each relevant TMX file ...
            for self.presentfile in fileslist:
                filename=self.presentfile[:-9]
                #print filename
                self.CreateTMX(filename)
        print 'Finished at '+strftime('%H-%M-%S')
        result = dialog.alertDialog(self, 'Processing done.', 'Processing Done')

    def on_btnCreateTMX_mouseClick(self, event):
        self.createTMXs()

    def on_menuFileCreateTMXFiles_select(self, event):
        self.createTMXs()

    def on_btnSelectLang1File_mouseClick(self, event):
        self.input1=self.GetInputFileName()
        
    def on_btnSelectLang2File_mouseClick(self, event):
        self.input2=self.GetInputFileName()
        
    def locate(self,pattern, basedir):
        """Locate all files matching supplied filename pattern in and below
        supplied root directory.
        
        @pattern: something like '*.tmx'
        @basedir:whole directory to be treated
        """
        import fnmatch
        for path, dirs, files in os.walk(os.path.abspath(basedir)):
            for filename in fnmatch.filter(files, pattern):
                yield os.path.join(path, filename)

    def SelectDirectory(self):
        """Select the directory where the files to be processed are

        @result: object returned by the dialog window with attributes accepted (true if user clicked OK button, false otherwise) and
            path (list of strings containing the full pathnames to all files selected by the user)
        @self.inputdir: directory where files to be processed are (and where output files will be written)
        @self.statusBar.text: text displayed in the program window status bar"""
        
        result= dialog.directoryDialog(self, 'Choose a directory', 'a')
        if result.accepted:
            self.inputdir=result.path
            self.statusBar.text=self.inputdir+' selected.'

    def on_menuFileSelectDirectory_select(self, event):
        self.SelectDirectory()

    def on_btnSelectDirectory_mouseClick(self, event):
        self.SelectDirectory()

    def on_menuHelpShowHelp_select(self, event):
        f = open('_READ_ME_FIRST.txt', "r")
        msg = f.read()
        result = dialog.scrolledMessageDialog(self, msg, '_READ_ME_FIRST.txt')
        
    def on_menuFileExit_select(self, event):
        sys.exit()
        

if __name__ == '__main__':
    app = model.Application(Moses2TMX)
    app.MainLoop()
