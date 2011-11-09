#! /usr/bin/env python
# -*- coding: utf_8 -*-
"""This program is used to prepare corpora extracted from TMX files.
It is particularly useful for translators not very familiar
with machine translation systems that want to use Moses with a highly customised
corpus.

It extracts  from a directory containing TMX files (and from all of its subdirectories)
all the segments of one or more language pairs (except empty segments and segments that are equal in both languages)
and removes all other information. It then creates 2 separate monolingual files per language pair,
both of which have strictly parallel (aligned) segments. This kind of corpus can easily be transformed
in other formats, if need be.

The program requires that Pythoncard and wxPython (as well as Python) be previously installed.

Copyright 2009, João L. A. C. Rosas

Distributed under GNU GPL v3 licence (see http://www.gnu.org/licenses/)

E-mail: extracttmxcorpus@gmail.com """

__version__ = "$Revision: 1.043$"
__date__ = "$Date: 2011/08/13$"
__author__="$João L. A. C. Rosas$"
#Special thanks to Gary Daine for a helpful suggestion about a regex expression
#Updated to run on Linux by Tom Hoar

from PythonCard import clipboard, dialog, graphic, model
from PythonCard.components import button, combobox,statictext,checkbox,staticbox
import wx
import os, re
import string
import sys
from time import strftime
import codecs


class Extract_TMX_Corpus(model.Background):

    def on_initialize(self, event):
        """Initialize values

        
        @self.inputdir: directory whose files will be treated
        @self.outputfile: base name of the resulting corpora files
        @self.outputpath: root directory of the resulting corpora files
        @currdir: program's current working directory
        @self.languages: list of languages whose segments can be processed
        @self.startinglanguage: something like 'EN-GB'
        @self.destinationlanguage: something like 'FR-FR'
        @self.components.cbStartingLanguage.items: list of values of the Starting Language combobox of the program's window
        @self.components.cbDestinationLanguage.items: list of values of the Destination Language combobox of the program's window
        @self.numtus: number of translation units extracted so far
        @self.presentfile: TMX file being currently processed
        @self.errortypes: variable that stocks the types of errors detected in the TMX file that is being processed
        @self.wroteactions: variable that indicates whether the actions files has already been written to
        """
        
        self.inputdir=''
        self.outputfile=''
        self.outputpath=''
        #Get directory where program file is and ...
        currdir=os.path.abspath(os.path.dirname(os.path.realpath(sys.argv[0])))
        #... load the file ("LanguageCodes.txt") with the list of languages that the program can process
        try:
            self.languages=open(currdir+os.sep+r'LanguageCodes.txt','r+').readlines()
        except:
            # If the languages file doesn't exist in the program directory, alert user that it is essential for the good working of the program and exit
            result = dialog.alertDialog(self, 'The file "LanguageCodes.txt" is missing. The program will now close.', 'Essential file missing')
            sys.exit()
        #remove end of line marker from each line in "LanguageCodes.txt"
        for lang in range(len(self.languages)):
            self.languages[lang]=self.languages[lang].rstrip()
        self.startinglanguage=''
        self.destinationlanguage=''
        #Insert list of language names in appropriate program window's combo boxes
        self.components.cbStartingLanguage.items=self.languages
        self.components.cbDestinationLanguage.items=self.languages
        self.tottus=0
        self.numtus=0
        self.numequaltus=0
        self.presentfile=''
        self.errortypes=''
        self.wroteactions=False
        self.errors=''

    def extract_language_segments_tmx(self,text):
        """Extracts TMX language segments from TMX files
        
        @text: the text of the TMX file
        @pattern: compiled regular expression object, which can be used for matching
        @tus: list that collects the translation units of the text
        @segs: list that collects the segment units of the relevant pair of languages
        @numtus: number of translation units extracted
        @present_tu: variable that stocks the translation unit relevant segments (of the chosen language pair) that are being processed
        @self.errortypes: variable that stocks the types of errors detected in the TMX file that is being processed
        """
        #print 'extract_language_segments: start at '+strftime('%H-%M-%S')
        result=('','')
        try:
            if text:
                # Convert character entities to "normal"  characters
                pattern=re.compile('&gt;',re.U)
                text=re.sub(pattern,'>',text)
                pattern=re.compile('&lt;',re.U)
                text=re.sub(pattern,'<',text)
                pattern=re.compile('&amp;',re.U)
                text=re.sub(pattern,'&',text)
                pattern=re.compile('&quot;',re.U)
                text=re.sub(pattern,'"',text)
                pattern=re.compile('&apos;',re.U)
                text=re.sub(pattern,"'",text)
                # Extract translation units
                pattern=re.compile('(?s)<tu.*?>(.*?)</tu>')
                tus=re.findall(pattern,text)
                ling1=''
                ling2=''
                #Extract relevant segments and store them in the @text variable
                if tus:
                    for tu in tus:
                        pattern=re.compile('(?s)<tuv.*?lang="'+self.startinglanguage+'">.*?<seg>(.*?)</seg>.*?<tuv.*?lang="'+self.destinationlanguage+'">.*?<seg>(.*?)</seg>')
                        present_tu=re.findall(pattern,tu)
                        self.tottus+=1
                        #reject empty segments
                        if present_tu: # and not present_tu[0][0].startswith("<")
                            present_tu1=present_tu[0][0].strip()
                            present_tu2=present_tu[0][1].strip()
                            present_tu1 = re.sub('<bpt.*</bpt>', '', present_tu1)
                            present_tu2 = re.sub('<bpt.*</bpt>', '', present_tu2)
                            present_tu1 = re.sub(r'<ept.*</ept>', '', present_tu1)
                            present_tu2 = re.sub(r'<ept.*</ept>', '', present_tu2)
                            present_tu1 = re.sub(r'<ut.*</ut>', '', present_tu1)
                            present_tu2 = re.sub(r'<ut.*</ut>', '', present_tu2)
                            present_tu1 = re.sub(r'<ph.*</ph>', '', present_tu1)
                            present_tu2 = re.sub(r'<ph.*</ph>', '', present_tu2)
                            #Thanks to Gary Daine
                            present_tu1 = re.sub('^[0-9\.() \t\-_]*$', '', present_tu1)
                            #Thanks to Gary Daine
                            present_tu2 = re.sub('^[0-9\.() \t\-_]*$', '', present_tu2)
                            if present_tu1 != present_tu2:
                                x=len(present_tu1)
                                y=len(present_tu2)
                                if (x <= y*3) and (y <= x*3):
                                    ling1=ling1+present_tu1+'\n'
                                    ling2=ling2+present_tu2+'\n'
                                    self.numtus+=1
                            else:
                                self.numequaltus+=1
                        pattern=re.compile('(?s)<tuv.*?lang="'+self.destinationlanguage+'">.*?<seg>(.*?)</seg>.*?<tuv.*?lang="'+self.startinglanguage+'">.*?<seg>(.*?)</seg>')
                        present_tu=re.findall(pattern,tu)
                        #print present_tu
                        if present_tu:
                            present_tu1=present_tu[0][1].strip()
                            present_tu2=present_tu[0][0].strip()
                            present_tu1 = re.sub('<bpt.*</bpt>', '', present_tu1)
                            present_tu2 = re.sub('<bpt.*</bpt>', '', present_tu2)
                            present_tu1 = re.sub(r'<ept.*</ept>', '', present_tu1)
                            present_tu2 = re.sub(r'<ept.*</ept>', '', present_tu2)
                            present_tu1 = re.sub(r'<ut.*</ut>', '', present_tu1)
                            present_tu2 = re.sub(r'<ut.*</ut>', '', present_tu2)
                            present_tu1 = re.sub(r'<ph.*</ph>', '', present_tu1)
                            present_tu2 = re.sub(r'<ph.*</ph>', '', present_tu2)
                            #Thanks to Gary Daine
                            present_tu1 = re.sub('^[0-9\.() \t\-_]*$', '', present_tu1)
                            #Thanks to Gary Daine
                            present_tu2 = re.sub('^[0-9\.() \t\-_]*$', '', present_tu2)
                            if present_tu1 != present_tu2:
                                x=len(present_tu1)
                                y=len(present_tu2)
                                if (x <= y*3) and (y <= x*3):
                                    ling1=ling1+present_tu1+'\n'
                                    ling2=ling2+present_tu2+'\n'
                                    self.numtus+=1
                            else:
                                self.numequaltus+=1
                    result=(ling1,ling2)
        except:
            self.errortypes=self.errortypes+'   - Extract Language Segments error\n'
        return result

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
                
    def getallsegments(self):
        """Get all language segments from the TMX files in the specified 
        directory
        
        @self.startinglanguage: something like 'EN-GB'
        @self.destinationlanguage: something like 'FR-FR'
        @fileslist: list of files that should be processed
        @self.inputdir: directory whose files will be treated
        @startfile:output file containing all segments in the @startinglanguage; file
            will be created in @self.inputdir
        @destfile:output file containing all segments in the @destinationlanguage; file
            will be created in @self.inputdir
        @actions:output file indicating the names of all files that were processed without errors; file
            will be created in @self.inputdir
        @self.errortypes: variable that stocks the types of errors detected in the TMX file that is being processed
        @self.presentfile: TMX file being currently processed
        @preptext: parsed XML text with all tags extracted and in string format
        @tus: list that receives the extracted TMX language translation units just with segments of the relevant language pair
        @num: loop control variable between 0 and length of @tus - 1
        @self.numtus: number of translation units extracted so far
        """
        self.statusBar.text='Processing '+ self.inputdir
        try:
            # Get a list of all TMX files that need to be processed
            fileslist=self.locate('*.tmx',self.inputdir)
            # Open output files for writing
            startfile=open(self.outputpath+os.sep+self.startinglanguage+  ' ('+self.destinationlanguage+')_' +self.outputfile,'w+b')
            destfile=open(self.outputpath+os.sep+self.destinationlanguage+' ('+self.startinglanguage+')_'+self.outputfile,'w+b')
            actions=open(self.outputpath+os.sep+'_processing_info'+os.sep+self.startinglanguage+ '-'+self.destinationlanguage+'_'+'actions_'+self.outputfile+'.txt','w+')
        except:
            # if any error up to now, add the name of the TMX file to the output file @errors
            self.errortypes=self.errortypes+'   - Get All Segments: creation of output files error\n'
        if fileslist:
            # For each relevant TMX file ...
            for self.presentfile in fileslist:
                self.errortypes=''
                try:
                    print self.presentfile
                    fileObj = codecs.open(self.presentfile, "rb", "utf-16","replace",0 )
                    pos=0
                    while True:
                        # read a new chunk of text...
                        preptext = fileObj.read(692141)
                        if not preptext:
                            break
                        last5=''
                        y=''
                        #... and make it end at the end of a translation unit
                        while True:
                            y=fileObj.read(1)
                            if not y:
                                break
                            last5=last5+y
                            if '</tu>' in last5:
                                break
                        preptext=preptext+last5
                        # ... and extract its relevant segments ...
                        if not self.errortypes:
                            segs1,segs2=self.extract_language_segments_tmx(preptext)
                            preptext=''
                            #... and write those segments to the output files
                            if segs1 and segs2:
                                try:
                                    startfile.write('%s' % (segs1.encode('utf-8','strict')))
                                    destfile.write('%s' % (segs2.encode('utf-8','strict')))
                                except:
                                    self.errortypes=self.errortypes+'   - Get All Segments: writing of output files error\n'
                                    print 'erro'
                    #if no errors up to now, insert the name of the TMX file in the @actions output file
                    #encoding is necessary because @actions may be in a directory whose name has special diacritic characters
                    if self.errortypes=='':
                        try:
                            actions.write(self.presentfile.encode('utf_8','replace')+'\n')
                            self.wroteactions=True
                        except:
                            self.errortypes=self.errortypes+'   - Get All Segments: writing of actions file error\n'
                    fileObj.close()
                except:
                    self.errortypes=self.errortypes+'   - Error reading input file\n'
            try:
                if self.wroteactions:
                    actions.write('\n*************************************************\n\n')
                    actions.write('Total number of translation units: '+str(self.tottus)+'\n')
                    actions.write('Number of extracted translation units (source segment not equal to destination segment): '+str(self.numtus)+'\n')
                    actions.write('Number of removed translation units (source segment equal to destination segment): '+str(self.numequaltus)+'\n')
                    actions.write('Number of empty translation units (source segment and/or destination segment not present): '+str(self.tottus-self.numequaltus-self.numtus))
                   
            except:
                self.errortypes=self.errortypes+'   - Get All Segments: writing of actions file error\n'
            # Close output files
            actions.close()
            destfile.close()
            startfile.close()
                    
    def SelectDirectory(self):
        """Select the directory where the TMX files to be processed are

        @result: object returned by the dialog window with attributes accepted (true if user clicked OK button, false otherwise) and
            path (list of strings containing the full pathnames to all files selected by the user)
        @self.inputdir: directory where TMX files to be processed are (and where output files will be written)
        @self.statusBar.text: text displayed in the program window status bar"""
        
        result= dialog.directoryDialog(self, 'Choose a directory', 'a')
        if result.accepted:
            self.inputdir=result.path
            self.statusBar.text=self.inputdir+' selected.'

    def on_menuFileSelectDirectory_select(self, event):
        self.SelectDirectory()

    def on_btnSelectDirectory_mouseClick(self, event):
        self.SelectDirectory()
        
    def GetOutputFileBaseName(self):
        """Get base name of the corpus files

        @expr: variable containing the base name of the output files
        @wildcard: list of wildcards used in the dialog window to filter types of files
        @result: object returned by the Open File dialog window with attributes accepted (true if user clicked OK button, false otherwise) and
            path (list of strings containing the full pathnames to all files selected by the user)
        @self.inputdir: directory where TMX files to be processed are (and where output files will be written)
        @location: variable containing the full path to the base name output file
        @self.outputpath: base directory of output files
        @self.outputfile: base name of the output files
        """
        
        # Default base name of the corpora files that will be produced. If you choose as base name "Corpus.txt", as starting language "EN-GB" and as destination
        # language "FR-FR" the corpora files will be named "Corpus_EN-GB.txt" and "Corpus_FR-FR.txt"
        expr='Corpus'
        #open a dialog that lets you choose the base name of the corpora files that will be produced. 
        wildcard = "Text files (*.txt;*.TXT)|*.txt;*.TXT"
        result = dialog.openFileDialog(None, "Name of corpus file", self.inputdir,expr,wildcard=wildcard)
        if result.accepted:
            location=os.path.split(result.paths[0])
            self.outputpath=location[0]
            self.outputfile = location[1]
            if not os.path.exists(self.outputpath+os.sep+'_processing_info'):
                try:
                    os.mkdir(self.outputpath+os.sep+'_processing_info')
                except:
                    result1 = dialog.alertDialog(self, "The program can't create the directory " + self.outputpath+os.sep+r'_processing_info, which is necessary for ' + \
                        'the creation of the output files. The program will now close.','Error')
                    sys.exit()

    def on_menuGetOutputFileBaseName_select(self, event):
        self.GetOutputFileBaseName()

    def on_btnGetOutputFileBaseName_mouseClick(self, event):
        self.GetOutputFileBaseName()
        
    def ExtractCorpus(self):
        """Get the directory where TMX files to be processed are, get the choice of the pair of languages that will be treated and launch the extraction
        of the corpus

        @self.errortypes: variable that stocks the types of errors detected in the TMX file that is being processed
        @self.presentfile: TMX file being currently processed
        @self.numtus: number of translation units extracted so far
        @self.startinglanguage: something like 'EN-GB'
        @self.destinationlanguage: something like 'FR-FR'
        @self.inputdir: directory whose files will be treated
        @self.components.cbStartingLanguage.items: list of values of the Starting Language combobox of the program's window
        @self.components.cbDestinationLanguage.items: list of values of the Destination Language combobox of the program's window
        @self.outputfile: base name of the resulting corpora files
        @self.errors:output file indicating the types of error that occurred in each processed TMX file
        @self.numtus: number of translation units extracted so far
        """

        print 'Extract corpus: started at '+strftime('%H-%M-%S')
        self.errortypes=''
        self.presentfile=''
        self.numtus=0
        #get the startinglanguage name (e.g.: "EN-GB") from the program window
        self.startinglanguage=self.components.cbStartingLanguage.text
        #get the destinationlanguage name from the program window
        self.destinationlanguage=self.components.cbDestinationLanguage.text
        #if the directory where TMX files (@inputdir) or the pair of languages were not previously chosen, open a dialog box explaining
        #the conditions that have to be met so that the extraction can be made and do nothing...
        if (self.inputdir=='') or (self.components.cbStartingLanguage.text=='') or (self.components.cbDestinationLanguage.text=='') or (self.outputfile=='') \
           or (self.components.cbStartingLanguage.text==self.components.cbDestinationLanguage.text):
            result = dialog.alertDialog(self, 'In order to extract a corpus, you need to:\n\n 1) indicate the directory where the TMX files are,\n 2)' \
                        +' the starting language,\n 3) the destination language (the 2 languages must be different), and\n 4) the base name of the output files.', 'Error')
        
        #...else, go ahead
        else:
            try:
                self.errors=open(self.outputpath+os.sep+'_processing_info'+os.sep+self.startinglanguage+ '-'+self.destinationlanguage+'_'+'errors_'+self.outputfile+'.txt','w+')
            except:
                pass
            self.statusBar.text='Please wait. This can be a long process ...'
            #Launch the segment extraction
            self.numtus=0
            self.getallsegments()
            # if any error up to now, add the name of the TMX file to the output file @errors
            if self.errortypes:
                try:
                    self.errors.write(self.presentfile.encode('utf_8','replace')+':\n'+self.errortypes)
                except:
                    pass
            try:
                self.errors.close()
            except:
                pass
            self.statusBar.text='Processing finished.'
            #Open dialog box telling that processing is finished and where can the resulting files be found
            self.inputdir=''
            self.outputfile=''
            self.outputpath=''
            print 'Extract corpus: finished at '+strftime('%H-%M-%S')
            result = dialog.alertDialog(self, 'Processing done. Results found in:\n\n1) '+ \
                self.outputpath+os.sep+self.startinglanguage+  ' ('+self.destinationlanguage+')_' +self.outputfile+ ' (starting language corpus)\n2) '+ \
                self.outputpath+os.sep+self.destinationlanguage+' ('+self.startinglanguage+')_'+self.outputfile+ \
                ' (destination language corpus)\n3) '+self.outputpath+os.sep+'_processing_info'+os.sep+self.startinglanguage+ '-'+self.destinationlanguage+'_'+ \
                'errors_'+self.outputfile+'.txt'+ ' (list of files that caused errors)\n4) '+self.outputpath+os.sep+'_processing_info'+os.sep+self.startinglanguage+ \
                '-'+self.destinationlanguage+'_'+'actions_'+self.outputfile+'.txt'+ ' (list of files where processing was successful)', 'Processing Done')

    def on_menuFileExtractCorpus_select(self, event):
        self.ExtractCorpus()
    def on_btnExtractCorpus_mouseClick(self, event):
        self.ExtractCorpus()

    def ExtractAllCorpora(self):
        """Extracts all the LanguagePairs that can be composed with the languages indicated in the file "LanguageCodes.txt"

        @self.presentfile: TMX file being currently processed
        @self.numtus: number of translation units extracted so far
        @numcorpora: number of language pair being processed
        @self.inputdir: directory whose files will be treated
        @self.outputfile: base name of the resulting corpora files
        @self.errors:output file indicating the types of error that occurred in each processed TMX file
        @self.startinglanguage: something like 'EN-GB'
        @self.destinationlanguage: something like 'FR-FR'
        @lang1: code of the starting language
        @lang2: code of the destination language
        @self.errortypes: variable that stocks the types of errors detected in the TMX file that is being processed
        @self.wroteactions: variable that indicates whether the actions files has already been written to
        """
        
        print 'Extract All Corpora: started at '+strftime('%H-%M-%S')
        self.presentfile=''
        self.numtus=0
        numcorpora=0
        #if the directory where TMX files (@inputdir) or the base name of the output files were not previously chosen, open a dialog box explaining
        #the conditions that have to be met so that the extraction can be made and do nothing...
        if (self.inputdir=='') or (self.outputfile==''):
            result = dialog.alertDialog(self, 'In order to extract all corpora, you need to:\n\n 1) indicate the directory where the TMX files are, and\n ' \
                                        + '2) the base name of the output files.', 'Error')
        #...else, go ahead
        else:
            try:
                for lang1 in self.languages:
                    for lang2 in self.languages:
                        if lang2 > lang1:
                            print lang1+'/'+lang2+' corpus being created...'
                            numcorpora=numcorpora+1
                            self.errortypes=''
                            self.numtus=0
                            self.wroteactions=False
                            #get the startinglanguage name (e.g.: "EN-GB") from the program window
                            self.startinglanguage=lang1
                            #get the destinationlanguage name from the program window
                            self.destinationlanguage=lang2
                            try:
                                self.errors=open(self.outputpath+os.sep+'_processing_info'+os.sep+self.startinglanguage+ '-'+self.destinationlanguage+'_'+'errors.txt','w+')
                            except:
                                pass
                            self.statusBar.text='Language pair '+str(numcorpora)+' being processed. Please wait.'
                            #Launch the segment extraction
                            self.getallsegments()
                            # if any error up to now, add the name of the TMX file to the output file @errors
                            if self.errortypes:
                                try:
                                    self.errors.write(self.presentfile.encode('utf_8','replace')+':\n'+self.errortypes.encode('utf_8','replace'))
                                except:
                                    pass
                            try:
                                self.errors.close()
                            except:
                                pass
                self.statusBar.text='Processing finished.'
            except:
                self.errortypes=self.errortypes+'   - Extract All Corpora error\n'
                self.errors.write(self.presentfile.encode('utf_8','replace')+':\n'+self.errortypes.encode('utf_8','replace'))
                self.errors.close()
            #Open dialog box telling that processing is finished and where can the resulting files be found
            self.inputdir=''
            self.outputfile=''
            self.outputpath=''
            print 'Extract All Corpora: finished at '+strftime('%H-%M-%S')
            result = dialog.alertDialog(self, 'Results found in: '+ self.outputpath+'.', 'Processing done')
                    

    def on_menuFileExtractAllCorpora_select(self, event):
        self.ExtractAllCorpora()
    def on_btnExtractAllCorpora_mouseClick(self, event):
        self.ExtractAllCorpora()

    def ExtractSomeCorpora(self):
        """Extracts the segments of the LanguagePairs indicated in the file "LanguagePairs.txt" located in the program's root directory

        @self.presentfile: TMX file being currently processed
        @self.numtus: number of translation units extracted so far
        @currdir: current working directory of the program
        @pairsoflanguages: list of the pairs of language that are going to be processed
        @self.languages: list of languages whose segments can be processed
        @numcorpora: number of language pair being processed
        @self.inputdir: directory whose files will be treated
        @self.outputfile: base name of the resulting corpora files
        @self.errors:output file indicating the types of error that occurred in each processed TMX file
        @self.startinglanguage: something like 'EN-GB'
        @self.destinationlanguage: something like 'FR-FR'
        @lang1: code of the starting language
        @lang2: code of the destination language
        @self.errortypes: variable that stocks the types of errors detected in the TMX file that is being processed
        @self.wroteactions: variable that indicates whether the actions files has already been written to
        """
        
        print 'Extract Some Corpora: started at '+strftime('%H-%M-%S')
        self.presentfile=''
        self.numtus=0
        currdir=os.path.abspath(os.path.dirname(os.path.realpath(sys.argv[0])))
        #... load the file ("LanguageCodes.txt") with the list of languages that the program can process
        try:
            pairsoflanguages=open(currdir+os.sep+r'LanguagePairs.txt','r+').readlines()
        except:
            # If the languages file doesn't exist in the program directory, alert user that it is essential for the good working of the program and exit
            result = dialog.alertDialog(self, 'The file "LanguagePairs.txt" is missing. The program will now close.', 'Essential file missing')
            sys.exit()
        #remove end of line marker from each line in "LanguageCodes.txt"
        if pairsoflanguages:
            for item in range(len(pairsoflanguages)):
                pairsoflanguages[item]=pairsoflanguages[item].strip()
                pos=pairsoflanguages[item].find("/")
                pairsoflanguages[item]=(pairsoflanguages[item][:pos],pairsoflanguages[item][pos+1:])
        else:
            # If the languages file is empty, alert user that it is essential for the good working of the program and exit
            result = dialog.alertDialog(self, 'The file "LanguagePairs.txt" is an essential file and is empty. The program will now close.', 'Empty file')
            sys.exit()
            
        #if the directory where TMX files (@inputdir) or the base name of the output files were not previously chosen, open a dialog box explaining
        #the conditions that have to be met so that the extraction can be made and do nothing...
        if (self.inputdir=='') or (self.outputfile==''):
            result = dialog.alertDialog(self, 'In order to extract all corpora, you need to:\n\n 1) indicate the directory where the TMX files are, and\n ' \
                                        + '2) the base name of the output files.', 'Error')
        #...else, go ahead
        else:
            numcorpora=0
            for (lang1,lang2) in pairsoflanguages:
                if lang1<>lang2:
                    print lang1+'/'+lang2+' corpus being created...'
                    self.errortypes=''
                    numcorpora=numcorpora+1
                    #get the startinglanguage code (e.g.: "EN-GB") 
                    self.startinglanguage=lang1
                    #get the destinationlanguage code 
                    self.destinationlanguage=lang2
                    try:
                        self.errors=open(self.outputpath+os.sep+'_processing_info'+os.sep+self.startinglanguage+ '-'+self.destinationlanguage+'_'+'errors.txt','w+')
                    except:
                        pass
                    self.statusBar.text='Language pair '+str(numcorpora)+' being processed. Please wait.'
                    #Launch the segment extraction
                    self.numtus=0
                    self.wroteactions=False
                    self.getallsegments()
                    # if any error up to now, add the name of the TMX file to the output file @errors
                    if self.errortypes:
                        try:
                            self.errors.write(self.presentfile.encode('utf_8','replace')+':\n'+self.errortypes.encode('utf_8','replace'))
                        except:
                            pass
                    try:
                        self.errors.close()
                    except:
                        pass
                else:
                    result = dialog.alertDialog(self, 'A bilingual corpus involves two different languages. The pair "'+lang1+'/'+lang2 + \
                            '" will not be processed.', 'Alert')
                self.statusBar.text='Processing finished.'
            #Open dialog box telling that processing is finished and where can the resulting files be found
            self.inputdir=''
            self.outputfile=''
            self.outputpath=''
            print 'Extract Some Corpora: finished at '+strftime('%H-%M-%S')
            result = dialog.alertDialog(self, 'Results found in: '+ self.outputpath+'.', 'Processing done')

    def on_menuFileExtractSomeCorpora_select(self, event):
        self.ExtractSomeCorpora()
    def on_btnExtractSomeCorpora_mouseClick(self, event):
        self.ExtractSomeCorpora()

    def on_menuHelpHelp_select(self, event):
        try:
            f = open('_READ_ME_FIRST.txt', "r")
            msg = f.read()
            result = dialog.scrolledMessageDialog(self, msg, 'readme.txt')
        except:
            result = dialog.alertDialog(self, 'Help file missing', 'Problem with the Help file')
        
            
if __name__ == '__main__':
    app = model.Application(Extract_TMX_Corpus)
    app.MainLoop()
