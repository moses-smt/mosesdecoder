#!/usr/bin/env python

import sys, os, subprocess

class Tokenizer:
    
    @staticmethod
    def batch_tokenise(lang, mosesdir, infilename, workdir):
        print "Tokenizing [%s] in working directory [%s]..." % (infilename, workdir)
        if not os.path.exists(workdir):
            os.makedirs(workdir)
        tok = Tokenizer(lang, mosesdir)
        basefilename = os.path.basename(infilename)
        outfilename = workdir + os.sep + basefilename + '.tok'
        tok.file_tokenise(infilename, outfilename)
        return outfilename
        
    def __init__(self, lang, mosesdir):
        self.arrows = None
        self.lang = lang
        #check the perl tokenizer is here
        #path = os.path.dirname(os.path.abspath(__file__))
        path = mosesdir + os.sep + 'scripts' + os.sep + 'tokenizer'
        self.perltok = path + os.sep + 'tokenizer.perl'
        if not os.path.exists(path):
            raise Exception, "Perl tokenizer does not exists"

    def file_tokenise(self, infilename, outfilename):
        cmd = '%s -q -l %s < %s > %s' % (self.perltok, self.lang, infilename, outfilename)
        pipe = subprocess.Popen(cmd, stdin = subprocess.PIPE, stdout = subprocess.PIPE, shell=True)
        pipe.wait()

if __name__ == '__main__':
    #do some test
    pass

