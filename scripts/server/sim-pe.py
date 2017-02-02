#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Written by Ulrich Germann on the basis of contrib/server/client.py.
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""Simulate post-editing of MT output.

Incrementally updates the dynamic phrase tables in the moses server.
"""

import argparse
import os
import sys
import time
import xmlrpclib
import moses
from subprocess import (
    PIPE,
    Popen,
    )


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
            my_args.extend(["-i", all_args[i + 1]])
            i += 1
        elif all_args[i] == "-inputtype":
            if all_args[i + 1] != "0":
                # Not yet supported! Therefore:
                errmsg = (
                    "FATAL ERROR: "
                    "%s only supports plain text input at this point."
                    % sys.argv[0])
                raise Exception(errmsg)
            # my_args.extend(["--input-type",all_args[i+1]])
            i += 1
        elif all_args[i] == "-lattice-samples":
            # my_args.extend(["--lattice-sample",all_args[i+2]])
            # my_args.extend(["--lattice-sample-file",all_args[i+1]])
            # mo_args[i:i+3] = []
            # i += 2
            # This is not yet supported! Therefore:
            errmsg = (
                "FATAL ERROR: %s does not yet support lattice sampling."
                % sys.argv[0])
            raise Exception(errmsg)

        elif all_args[i] == "-n-best-list":
            my_args.extend(["--nbest", all_args[i + 2]])
            my_args.extend(["--nbest-file", all_args[i + 1]])
            i += 2

        elif all_args[i] == "-n-best-distinct":
            my_args.extend(["-u"])

        else:
            arglist.append(all_args[i])
            pass

        i += 1
        pass
    return my_args, mo_args


def interpret_args(my_args):
    """
    Parse script-specific argument list.
    """
    aparser = argparse.ArgumentParser()

    aparser.add_argument(
        "-s", "--server-cmd", default="mosesserver", dest="servercmd",
        help="Path to moses server command.")
    aparser.add_argument(
        "--url", help="URL of external moses server.")
    aparser.add_argument(
        "-p", "--port", type=int, default=7447,
        help="Port number to be used for server.")

    # Input / output.
    aparser.add_argument(
        "-i", "--input", default='-', help="source file")
    aparser.add_argument(
        "-r", "--ref", default=None, help="Reference translation.")
    aparser.add_argument(
        "-a", "--aln", default=None, help="Alignment.")
    aparser.add_argument(
        "-o", "--output", default="-", help="Output file.")
    aparser.add_argument(
        "-d", "--debug", action='store_true', help="Debug mode.")

    # Moses reporting options.
    aparser.add_argument(
        "-A", "--with-alignment", dest="A", action='store_true',
        help="Include alignment in output.")
    aparser.add_argument(
        "-G", "--with-graph", type=bool, default=False, dest="G",
        help="Include search graph info in output.")
    aparser.add_argument(
        "-T", "--with-transopt", type=bool, default=False, dest="T",
        help="Include translation options info in output.")
    aparser.add_argument(
        "-F", "--report-all-factors", action="store_true", dest="F",
        help="Report all factors.")
    aparser.add_argument(
        "-n", "--nbest", type=int, dest="nbest", default=0,
        help="Size of nbest list.")
    aparser.add_argument(
        "-N", "--nbest-file", dest="nbestFile", default=0,
        help="Output file for nbest list.")
    aparser.add_argument(
        "-u", "--nbest-distinct", type=bool, dest="U", default=False,
        help="Report all factors.")

    return aparser.parse_args(my_args)


def translate(proxy, args, line):
    if type(line) is unicode:
        param = {'text': line.strip().encode('utf8')}
    elif type(line) is str:
        param = {'text': line.strip()}
    else:
        raise Exception("Can't handle input")
    if args.A:
        param['align'] = True
    if args.T:
        param['topt'] = True
    if args.F:
        param['report-all-factors'] = True
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
            sys.stderr.flush()
            print >>sys.stderr, " XMLRPC error:", e
            print >>sys.stderr, "Input was"
            print >>sys.stderr, param
            sys.exit(1)

        except IOError as e:
            print >>sys.stderr, (
                "I/O error({0}): {1}".format(e.errno, e.strerror))
            time.sleep(5)

        except:
            serverstatus = mserver.process.poll()
            if serverstatus is None:
                print >>sys.stderr, (
                    "Connection failed after %f seconds" % (time.time() - t1))
                attempts += 1
                if attempts > 10:
                    time.sleep(10)
                else:
                    time.sleep(5)
            else:
                print >>sys.stderr, (
                    "Oopsidaisy, server exited with code %d (signal %d)"
                    % (serverstatus / 256, serverstatus % 256))
                pass
            pass
        pass
    raise Exception("Exception: could not reach translation server.")


def read_data(fname):
    """
    Read and return data (source, target or alignment) from file fname.
    """
    if fname[-3:] == ".gz":
        process = Popen(["zcat", fname], stdout=PIPE)
        stdout, _ = process.communicate()
        foo = stdout.strip().split('\n')
    else:
        foo = [x.strip() for x in open(fname).readlines()]
    return foo


def repack_result(idx, result):
    global args
    if args.nbest:
        for h in result['nbest']:
            fields = [idx, h['hyp'], h['fvals'], h['totalScore']]
            for i in xrange(len(fields)):
                if type(fields[i]) is unicode:
                    fields[i] = fields[i].encode('utf-8')
                    pass
                pass
            # Print fields.
            print >>NBestFile, "%d ||| %s ||| %s ||| %f" % tuple(fields)
        pass
    if 'align' in result:
        t = result['text'].split()
        span = ''
        i = 0
        k = 0
        for a in result['align']:
            k = a['tgt-start']
            if k:
                print " ".join(t[i:k]).encode('utf8'), span,
            i = k
            span = "|%d %d|" % (a['src-start'], a['src-end'])
        print " ".join(t[k:]).encode('utf8'), span
    else:
        print result['text'].encode('utf8')


if __name__ == "__main__":
    my_args, mo_args = split_args(sys.argv[1:])

    # print "MY ARGS", my_args
    # print "MO_ARGS", mo_args

    global args
    args = interpret_args(my_args)

    if "-show-weights" in mo_args:
        # This is for use during tuning, where moses is called to get a list
        # of feature names.
        devnull = open(os.devnull, "w")
        mo = Popen(mserver.cmd + mo_args, stdout=PIPE, stderr=devnull)
        print mo.communicate()[0].strip()
        sys.exit(0)
        pass

    if args.nbest:
        if args.nbestFile:
            NBestFile = open(args.nbestFile, "w")
        else:
            NBestFile = sys.stdout
            pass
        pass

    ref = None
    aln = None
    if args.ref:
        ref = read_data(args.ref)
    if args.aln:
        aln = read_data(args.aln)

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
            result = translate(mserver.proxy, args, line)
            repack_result(idx, result)
            line = sys.stdin.readline()
            idx += 1
    else:
        src = read_data(args.input)
        for i in xrange(len(src)):
            result = translate(mserver.proxy, args, src[i])
            repack_result(i, result)
            if args.debug:
                print >>sys.stderr, result['text'].encode('utf-8')
                pass
            if ref and aln:
                result = mserver.proxy.updater({
                    'source': src[i],
                    'target': ref[i],
                    'alignment': aln[i],
                    })
