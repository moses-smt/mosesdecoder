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
    arglist = mo_args
    i = 0
    # IMPORTANT: the code below must be coordinated with 
    # - the evolution of moses command line arguments
    # - mert-moses.pl 
    while i < len(all_args):
        # print i,"MY_ARGS", my_args
        # print i,"MO_ARGS", mo_args
        if all_args[i] == "--[":
            arglist = my_args
        elif all_args[i] == "--]":
            arglist = mo_args
        elif all_args[i] == "-i" or all_args[i] == "-input-file":
            my_args.extend(["-i",all_args[i+1]])
            i += 1
        elif all_args[i] == "-inputtype":
            if all_args[i+1] != "0":
                # not yet supported! Therefore:
                errmsg  = "FATAL ERROR: %s "%sys.argv[0]
                errmsg += "only supports plain text input at this point."
                raise Exception(errsmg)
            # my_args.extend(["--input-type",all_args[i+1]])
            i += 1
        elif all_args[i] == "-lattice-samples":
            # my_args.extend(["--lattice-sample",all_args[i+2]])
            # my_args.extend(["--lattice-sample-file",all_args[i+1]])
            # mo_args[i:i+3] = []
            # i += 2
            # This is not yet supported! Therefore:
            errmsg  = "FATAL ERROR: %s "%sys.argv[0]
            errmsg += "does not yet support lattice sampling."
            raise Exception(errsmg)
        
        elif all_args[i] == "-n-best-list":
            my_args.extend(["--nbest",all_args[i+2]])
            my_args.extend(["--nbest-file",all_args[i+1]])
            i += 2

        elif all_args[i] == "-n-best-distinct":
            my_args.extend(["-u"])

        else:
            arglist.append(all_args[i])
            pass

        i += 1
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
    aparser.add_argument("-p","--port", type=int, default=7447,
                         help="port number to be used for server")
    
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
        param = { 'text' : line.strip() }
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
    while attempts < 20:
        t1 = time.time()
        try:
            return proxy.translate(param) 

        # except xmlrpclib.Fault as e:
        # except xmlrpclib.ProtocolError as e:
        # except xmlrpclib.ResponseError as e:
        except xmlrpclib.Error as e:
            time.sleep(2) # give all the stderr stuff a chance to be flushed
            print >>sys.stderr," XMLRPC error:",e
            print >>sys.stderr, "Input was"
            print >>sys.stderr, param
            sys.exit(1)

        except IOError as e:
            print >>sys.stderr,"I/O error({0}): {1}".format(e.errno, e.strerror)
            time.sleep(5)

        except:
            serverstatus = mserver.process.poll()
            if serverstatus == None:
                print >>sys.stderr, "Connection failed after %f seconds"%(time.time()-t1)
                attempts += 1
                if attempts > 10:
                    time.sleep(10)
                else:
                    time.sleep(5)
                    pass
            else:
                
                print >>sys.stderr, "Oopsidaisy, server exited with code %d (signal %d)"\
                    %(serverstatus/256,serverstatus%256)
                pass
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

def repack_result(idx,result):
    global args
    if args.nbest:
        for h in result['nbest']:
            fields = [idx,h['hyp'],h['fvals'],h['totalScore']]
            for i in xrange(len(fields)):
                if type(fields[i]) is unicode:
                    fields[i] = fields[i].encode('utf-8')
                    pass
                pass
            # print fields
            print >>NBestFile,"%d ||| %s ||| %s ||| %f"%tuple(fields)
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

    # print "MY ARGS", my_args
    # print "MO_ARGS", mo_args

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

    ref = None
    aln = None
    if args.ref: ref = read_data(args.ref)
    if args.aln: aln = read_data(args.aln)

    if ref and aln:
        try:
            mo_args.index("--serial")
        except:
            mo_args.append("--serial")
            pass
        pass

    if args.url:
        mserver.connect(args.url)
    else:
        mserver.start(args=mo_args, port=args.port, debug=args.debug)
        pass

    if (args.input == "-"):
        line = sys.stdin.readline()
        idx = 0
        while line:
            result = translate(mserver.proxy,args,line)
            repack_result(idx,result)
            line = sys.stdin.readline()
            idx += 1
            pass
        pass
    else:
        src = read_data(args.input)
        for i in xrange(len(src)):
            result = translate(mserver.proxy,args,src[i])
            repack_result(i,result)
            if args.debug:
                print >>sys.stderr, result['text'].encode('utf-8')
                pass
            if  ref and aln:
                result = mserver.proxy.updater({'source'    : src[i],
                                                'target'    : ref[i],
                                                'alignment' : aln[i]})
                pass
            pass
        pass
    pass
