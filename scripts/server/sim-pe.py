#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Written by Ulrich Germann on the basis of contrib/server/client.py.
# This script simulates post-editing of MT output and incrementally
# updates the dynamic phrase tables in the moses server.

import xmlrpclib,datetime,argparse,sys,os,time
from subprocess import *

# We must perform some custom argument processing, as moses parameter
# specifications do not comply with the standards used in standard
# argument parsing packages; an isolated double dash separates script
# arguments from moses arguments

MosesProcess = None 
NBestFile    = None

def shutdown():
    if MosesProcess:
        if args.debug:
            print >>sys.stderr,"shutting down moses server"
            pass
        MosesProcess.terminate()
        pass
    return

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

def launch_moses(mo_args):
    """
    Spawn a moses server process. Return URL of said process.
    """
    global MosesProcess
    try:
        port_index = mo_args.index("--server-port") + 1
    except:
        mo_args.extend(["--server-port","7777"])
        port_index = len(mo_args) - 1
        pass
    port = find_free_port(int(mo_args[port_index]))
    if not port:
        print >>sys.stderr, "FATAL ERROR: No available port for moses server!"
        sys.exit(1)
        pass
    if args.debug:
        MosesProcess = Popen([args.servercmd] + mo_args)
    else:
        devnull = open(os.devnull,"w")
        MosesProcess = Popen([args.servercmd] + mo_args, 
                             stderr=devnull, stdout=devnull)
    if MosesProcess.poll():
        print >>sys.stderr, "FATAL ERROR: Could not launch moses server!"
        sys.exit(1)
        pass
    if args.debug:
        print >>sys.stderr,"MOSES port is %d."%port 
        print >>sys.stderr,"Moses poll status is", MosesProcess.poll()
        pass
    return "http://localhost:%d"%port

def split_args(all_args):
    """
    Split argument list all_args into script-specific 
    and moses-specific arguments.
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
            my_args.extend(["--src",m_args[i+1]])
            mo_args[i:i+2] = []
            
        elif mo_args[i] == "-inputtype":
            if mo_args[i+1] != "0":
                # not yet supported! Therefore:
                errmsg  = "FATAL ERROR: "
                errmsg += "%s only supports plain text input at this point."
                print >>sys.stderr,errmsg%sys.argv[0]
                sys.exit(1)
                pass
            my_args.extend(["--input-type",mo_args[i+1]])
            mo_args[i:i+2] = []
            
        elif mo_args[i] == "-lattice-samples":
            my_args.extend(["--lattice-sample",mo_args[i+2]])
            my_args.extend(["--lattice-sample-file",mo_args[i+1]])
            mo_args[i:i+3] = []
                # not yet supported! Therefore:
            errmsg  = "FATAL ERROR: "
            errmsg += "%s does not yet support lattice sampling."
            print >>sys.stderr,errmsg%sys.argv[0]
            sys.exit(1)
            
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

    # interfacing with moses
    # aparser.add_argument("-m","--moses-cmd",default="moses",dest="mosescmd",
    #                      help="path to standard moses command")
    aparser.add_argument("-s","--server-cmd",default="mosesserver",
                         dest="servercmd", help="path to moses server command")
    aparser.add_argument("-u","--url",help="URL of external moses server.")
    
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
    aparser.add_argument("-U","--nbest-distinct",type=bool,dest="U",default=False,
                         help="report all factors")

    return aparser.parse_args(my_args)
    
def translate(proxy,args,s):
    param = {'text':s.strip()}
    if args.A:     param['align'] = True
    if args.T:     param['topt']  = True
    if args.F:     param['report-all-factors'] = True
    if args.nbest: 
        param['nbest'] = int(args.nbest)
        param['add-score-breakdown'] = True
        pass
    if args.U:     param['nbest-distinct'] = True
    
    try:
        ret = proxy.translate(param)
    except:
        return None
        pass
    return ret

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
        if not NBestFile:
            shutdown()
            assert NBestFile
            sys.exit(1)
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
            if k: print " ".join(t[i:k]),span,
            i = k
            span = "|%d %d|"%(a['src-start'],a['src-end'])
            pass
        print " ".join(t[k:]),span
        pass
    else:
        print result['text']
        pass
    return

if __name__ == "__main__":
    my_args, mo_args = split_args(sys.argv[1:])

    global args
    args = interpret_args(my_args)
    if "-show-weights" in mo_args:
        devnull = open(os.devnull,"w")
        mo = Popen([args.servercmd] + mo_args,stdout=PIPE,stderr=devnull)
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
    if "url" not in args or not args.url:
        url = launch_moses(mo_args)
    else:
        url = args.url
        pass
    if url[:4]  != "http":  url = "http://%s"%url
    if url[-5:] != "/RPC2": url += "/RPC2"
    proxy = xmlrpclib.ServerProxy(url)

    ret = None
    aln = None
    if args.ref: ref = read_data(args.ref)
    if args.aln: aln = read_data(args.aln)

    if (args.input == "-"):
        line = sys.stdin.readline()
        id = 0
        while line:
            result = translate(proxy,args,line)
            repack_result(id,result)
            line = sys.stdin.readline()
            id += 1
            pass
        pass
    else:
        src = read_data(args.src)
        for i in xrange(len(src)):
            if  ref and aln:
                result = proxy.updater({'source'    : src[i],
                                        'target'    : ref[i],
                                        'alignment' : aln[i]})
                repack_result(i,result)
                pass
            pass
        pass
    pass
    
shutdown()
