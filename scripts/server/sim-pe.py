#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Written by Ulrich Germann on the basis of contrib/server/client.py.
# This script simulates post-editing of MT output and incrementally
# updates the dynamic phrase tables in the moses server.

import xmlrpclib,datetime,argparse,sys,os,time
import moses
from moses import MosesServer
from subprocess import *
mserver = moses.MosesServer()

# We must perform some custom argument processing, as moses parameter
# specifications do not comply with the standards used in standard
# argument parsing packages; an isolated double dash separates script
# arguments from moses arguments
def split_args(all_args):
    """
    Split argument list all_args into arguments specific to this script and
    arguments relating to the moses server. An isolated double dash acts as 
    the separator between the two types of arguments. 
    """
    my_args = []
    mo_args = []
    try:
        i = all_args.index("--")
        my_args = all_args[:i]
        mo_args = all_args[i+1:]
    except:
        my_args = []
        mo_args = all_args[:]
        pass

    # IMPORTANT: the code below must be coordinated with 
    # - the evolution of moses command line arguments
    # - mert-moses.pl 
    i = 0
    while i < len(mo_args):
        if mo_args[i] == "-i" or mo_args[i] == "-input-file":
            my_args.extend(["-i",mo_args[i+1]])
            mo_args[i:i+2] = []
            
        elif mo_args[i] == "-inputtype":
            if mo_args[i+1] != "0":
                # not yet supported! Therefore:
                errmsg  = "FATAL ERROR: %s "%sys.argv[0]
                errmsg += "only supports plain text input at this point."
                raise Exception(errsmg)
            my_args.extend(["--input-type",mo_args[i+1]])
            mo_args[i:i+2] = []
            
        elif mo_args[i] == "-lattice-samples":
            # my_args.extend(["--lattice-sample",mo_args[i+2]])
            # my_args.extend(["--lattice-sample-file",mo_args[i+1]])
            # mo_args[i:i+3] = []
            # This is not yet supported! Therefore:
            errmsg  = "FATAL ERROR: %s "%sys.argv[0]
            errmsg += "does not yet support lattice sampling."
            raise Exception(errsmg)
        
        elif mo_args[i] == "-n-best-list":
            my_args.extend(["--nbest",mo_args[i+2]])
            my_args.extend(["--nbest-file",mo_args[i+1]])
            mo_args[i:i+3] = []

        elif mo_args[i] == "-n-best-distinct":
            my_args.extend(["-U"])
            mo_args[i:i+1] = []

        else:
            i += 1
            pass
        pass
    return my_args,mo_args
    
def interpret_args(my_args):
    """
    Parse script-specific argument list.
    """
    aparser = argparse.ArgumentParser()

    aparser.add_argument("-s","--server-cmd",default="mosesserver",
                         dest="servercmd", help="path to moses server command")
    aparser.add_argument("--url",help="URL of external moses server.")
    
    # input / output
    aparser.add_argument("-i","--input",help="source file",default="-")
    aparser.add_argument("-r","--ref",help="reference translation",default=None)
    aparser.add_argument("-a","--aln",help="alignment",default=None)
    aparser.add_argument("-o","--output",default="-",help="output file")
    aparser.add_argument("-d","--debug",action="store_true",help="debug mode")
    
    # moses reporting options
    aparser.add_argument("-A","--with-alignment", dest="A",
                         help="include alignment in output", action="store_true")
    aparser.add_argument("-G","--with-graph",type=bool, default=False, dest="G",
                         help="include search graph info in output")
    aparser.add_argument("-T","--with-transopt",type=bool, default=False, dest = "T",
                         help="include translation options info in output")
    aparser.add_argument("-F","--report-all-factors", action="store_true",dest="F",
                         help="report all factors")
    aparser.add_argument("-n","--nbest",type=int,dest="nbest",default=0, 
                         help="size of nbest list")
    aparser.add_argument("-N","--nbest-file",dest="nbestFile",default=0,
                         help="output file for nbest list")
    aparser.add_argument("-u","--nbest-distinct",type=bool,dest="U",default=False,
                         help="report all factors")

    return aparser.parse_args(my_args)
    
def translate(proxy, args, line):
    if type(line) is unicode:
        param = { 'text' : line.strip().encode('utf8') }
    elif type(line) is str:
        param = { 'text' : line.strip().encode('utf8') }
    else:
        raise Exception("Can't handle input")
    if args.A: param['align'] = True
    if args.T: param['topt']  = True
    if args.F: param['report-all-factors'] = True
    if args.nbest: 
        param['nbest'] = int(args.nbest)
        param['add-score-breakdown'] = True
        pass
    if args.U: 
        param['nbest-distinct'] = True
        pass
    attempts = 0
    while attempts < 120:
        try:
            return proxy.translate(param)
        except:
            print >>sys.stderr, "Waiting", proxy
            attempts += 1
            time.sleep(5)
            pass
        pass
    raise Exception("Exception: could not reach translation server.")
    

def read_data(fname):
    """
    Read and return data (source, target or alignment) from file fname.
    """
    if fname[-3:] == ".gz":
        foo = Popen(["zcat",fname],stdout=PIPE)\
            .communicate()[0]\
            .strip().split('\n')
    else:
        foo = [x.strip() for x in open(fname).readlines()]
        pass
    return foo

def repack_result(id,result):
    global args
    if args.nbest:
        for h in result['nbest']:
            fields = (id,h['hyp'],h['fvals'],h['totalScore'])
            print >>NBestFile,"%d ||| %s ||| %s ||| %f"%fields
            pass
        pass
    if 'align' in result:
        t = result['text'].split()
        span = ''
        i = 0
        k = 0
        for a in result['align']:
            k = a['tgt-start']
            if k: print " ".join(t[i:k]).encode('utf8'),span,
            i = k
            span = "|%d %d|"%(a['src-start'],a['src-end'])
            pass
        print " ".join(t[k:]).encode('utf8'),span
        pass
    else:
        print result['text'].encode('utf8')
        pass
    return

if __name__ == "__main__":
    my_args, mo_args = split_args(sys.argv[1:])

    global args
    args = interpret_args(my_args)

    if "-show-weights" in mo_args:
        # this is for use during tuning, where moses is called to get a list of 
        # feature names
        devnull = open(os.devnull,"w")
        mo = Popen(mserver.cmd + mo_args,stdout=PIPE,stderr=devnull)
        print mo.communicate()[0].strip()
        sys.exit(0)
        pass

    if args.nbest:
        if args.nbestFile:
            NBestFile = open(args.nbestFile,"w")
        else:
            NBestFile = sys.stdout
            pass
        pass

    if args.url:
        mserver.connect(args.url)
    else:
        mserver.start(args=mo_args,debug=args.debug)
        pass

    ref = None
    aln = None
    if args.ref: ref = read_data(args.ref)
    if args.aln: aln = read_data(args.aln)

    if (args.input == "-"):
        line = sys.stdin.readline()
        id = 0
        while line:
            result = translate(mserver.proxy,args,line)
            repack_result(id,result)
            line = sys.stdin.readline()
            id += 1
            pass
        pass
    else:
        src = read_data(args.input)
        for i in xrange(len(src)):
            result = translate(mserver.proxy,args,src[i])
            repack_result(id,result)
            if  ref and aln:
                result = mserver.proxy.updater({'source'    : src[i],
                                                'target'    : ref[i],
                                                'alignment' : aln[i]})
                pass
            pass
        pass
    pass
