#!/usr/bin/env python

# Written by Michael Denkowski
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

'''Parallelize decoding with multiple instances of moses on a local machine

Parallel decoding works well with memory mapped models:

Compact phrase table and reordering (Marcin Junczys-Dowmunt)
http://www.statmt.org/moses/?n=Advanced.RuleTables#ntoc3
http://www.statmt.org/moses/?n=Advanced.RuleTables#ntoc4

Dynamic phrase table (Ulrich Germann)
http://www.statmt.org/moses/?n=Advanced.Incremental#ntoc3

Language model (Kenneth Heafield)
http://www.statmt.org/moses/?n=FactoredTraining.BuildingLanguageModel#ntoc19

To use with mert-moses.pl, activate --multi-moses and set the number of moses
instances with --decoder-flags='-threads N'
'''

import os
import Queue
import signal
import subprocess
import sys
import threading

HELP = '''Multiple process decoding with Moses

Usage:
    {} moses -config moses.ini [options] [decoder flags]

Options:
    -threads N: number of decoders to run in parallel
            (default 1, overrides moses.ini)
    -n-best-list nbest.out N [distinct]: location and size of N-best list
    -show-weights: for mert-moses.pl, just call moses and exit

Other options (decoder flags) are passed through to moses instances
'''

# Defaults
INPUT = sys.stdin
THREADS = 1
DONE = threading.Event()
PID = os.getpid()

def kill_main(msg):
    '''kill -9 the main thread to stop everything immediately'''
    sys.stderr.write('{}\n'.format(msg))
    os.kill(PID, signal.SIGKILL)


def gzopen(f):
    '''Open plain or gzipped text'''
    return gzip.open(f, 'rb') if f.endswith('.gz') else open(f, 'r')


def run_instance(cmd, tasks, n_best=False):
    '''Run an instance of moses that processes tasks (input lines) from a
    queue'''
    # Main loop: take tasks as they are available, complete them, and mark them
    # as done.  Stop when DONE encountered.
    try:
        moses = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        while True:
            (id, line, out, event) = tasks.get()
            if event == DONE:
                break
            if n_best:
                # Input line followed by blank line (sentinel)
                moses.stdin.write(line)
                moses.stdin.write('\n')
                # Read and copy lines until sentinel line, copy real line id
                # id ||| hypothesis words  ||| feature scores ||| total score
                line = moses.stdout.readline()
                (first_i, rest) = line.split(' ||| ', 1)
                out.append(' ||| '.join((id, rest)))
                while True:
                    line = moses.stdout.readline()
                    (i, rest) = line.split(' ||| ', 1)
                    if i != first_i:
                        break
                    out.append(' ||| '.join((id, rest)))
            else:
                moses.stdin.write(line)
                out.append(moses.stdout.readline())
            event.set()
        moses.stdin.close()
        moses.wait()
    except:
        kill_main('Error communicating with moses instance: see stderr')


def write_results(results, n_best=False, n_best_out=None):
    '''Write out results (output lines) from a queue as they are populated'''
    while True:
        (id, line, out, event) = results.get()
        if event == DONE:
            break
        event.wait()
        if n_best:
            # Write top-best and N-best
            # id ||| hypothesis words  ||| feature scores ||| total score
            top_best = out[0].split(' ||| ', 2)[1]
            # Except don't write top-best if writing N-best to stdout "-"
            if n_best_out != sys.stdout:
                sys.stdout.write('{}\n'.format(top_best))
                sys.stdout.flush()
            for line in out:
                n_best_out.write(line)
            n_best_out.flush()
        else:
            sys.stdout.write(out[0])
            sys.stdout.flush()


def main(argv):
    # Defaults
    moses_ini = None
    input = INPUT
    threads = THREADS
    n_best = False
    n_best_file = None
    n_best_size = None
    n_best_distinct = False
    n_best_out = None
    show_weights = False

    # Decoder command
    cmd = argv[1:]

    # Parse special options and remove from cmd
    i = 1
    while i < len(cmd):
        if cmd[i] in ('-f', '-config'):
            moses_ini = cmd[i + 1]
            # Do not remove from cmd
            i += 2
        elif cmd[i] in ('-i', '-input-file'):
            input = gzopen(cmd[i + 1])
            cmd = cmd[:i] + cmd[i + 2:]
        elif cmd[i] in ('-th', '-threads'):
            threads_found = True
            threads = int(cmd[i + 1])
            cmd = cmd[:i] + cmd[i + 2:]
        elif cmd[i] == '-n-best-list':
            n_best = True
            n_best_file = cmd[i + 1]
            n_best_size = cmd[i + 2]
            # Optional "distinct"
            if i + 3 < len(cmd) and cmd[i + 3] == 'distinct':
                n_best_distinct = True
                cmd = cmd[:i] + cmd[i + 4:]
            else:
                cmd = cmd[:i] + cmd[i + 3:]
        # Handled specially for mert-moses.pl
        elif cmd[i] == '-show-weights':
            show_weights = True
            # Do not remove from cmd
            i += 1
        else:
            i += 1

    # If mert-moses.pl passes -show-weights, just call moses
    if show_weights:
        sys.stdout.write(subprocess.check_output(cmd))
        sys.stdout.flush()
        sys.exit(0)

    # Check inputs
    if not (len(cmd) > 0 and moses_ini):
        sys.stderr.write(HELP.format(argv[0]))
        sys.exit(2)
    if not (os.path.isfile(cmd[0]) and os.access(cmd[0], os.X_OK)):
        raise ProgramFailure(
            'Error: moses "{}" is not executable\n'.format(cmd[0]))

    # Report settings
    sys.stderr.write('Moses flags: {}\n'.format(' '.join('\'{}\''.format(s) if ' ' in s else s for s in cmd[1:])))
    sys.stderr.write('Instances: {}\n'.format(threads))
    if n_best:
        sys.stderr.write('N-best list: {} ({}{})\n'.format(n_best_file, n_best_size, ', distinct' if n_best_distinct else ''))

    # Task and result queues (buffer 8 * threads input lines)
    tasks = Queue.Queue(maxsize=(8 * threads))
    results = Queue.Queue()

    # N-best capture
    if n_best:
        cmd.append('-n-best-list')
        cmd.append('-')
        cmd.append(n_best_size)
        if n_best_distinct:
            cmd.append('distinct')
        if n_best_file == '-':
            n_best_out = sys.stdout
        else:
            n_best_out = open(n_best_file, 'w')

    # Start instances
    instances = []
    for _ in range(threads):
        t = threading.Thread(target=run_instance, args=(cmd, tasks, n_best))
        instances.append(t)
        t.start()

    # Start results writer
    writer = threading.Thread(target=write_results, args=(results, n_best, n_best_out))
    writer.start()

    # Main loop: queue task for each input line
    id = 0
    while True:
        line = input.readline()
        if not line:
            break
        # (input, out lines, err lines, "done" event)
        task = (str(id), line, [], threading.Event())
        results.put(task)
        tasks.put(task)
        id += 1

    # Tell instances to exit
    for t in instances:
        tasks.put((None, None, None, DONE))
    for t in instances:
        t.join()

    # Stop results writer
    results.put((None, None, None, DONE))
    writer.join()

    # Cleanup
    if n_best:
        n_best_out.close()


if __name__ == '__main__':
    main(sys.argv)
