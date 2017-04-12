#!/usr/bin/env python

# Written by Michael Denkowski
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

'''Parallelize decoding with multiple instances of moses on a local machine

To use with mert-moses.pl, activate --multi-moses and set the number of moses
instances and threads per instance with --decoder-flags='--threads P:T:E'

This script runs a specified number of moses instances, each using one or more
threads.  The highest speed is generally seen with many single-threaded
instances while the lowest memory usage is seen with a single many-threaded
instance.  It is recommended to use the maximum number of instances that will
fit into memory (up to the number of available CPUs) and distribute CPUs across
them equally.  For example, a machine with 32 CPUs that can fit 3 copies of
moses into memory would use --threads 2:11:10 for 2 instances with 11 threads
each and an extra instance with 10 threads (3 instances total using all CPUs).

Memory mapped models can be shared by multiple processes and increase the number
of instances that can fit into memory:

Mmaped phrase tables (Ulrich Germann)
http://www.statmt.org/moses/?n=Advanced.Incremental#ntoc3

Mmaped mapped language models (Kenneth Heafield)
http://www.statmt.org/moses/?n=FactoredTraining.BuildingLanguageModel#ntoc19
'''

import collections
import os
import Queue
import signal
import subprocess
import sys
import threading
import time

HELP = '''Multiple process decoding with Moses

Usage:
    {} moses --config moses.ini [options] [decoder flags]

Options:
    --threads P:T:E
            P: Number of parallel instances to run
            T: Number of threads per instance
            E: Number of threads in optional extra instance
            (default 1:1:0, overrides [threads] in moses.ini.  Specifying T
             and E is optional, e.g. --threads 16 starts 16 single-threaded
             instances)
    --n-best-list nbest.out N [distinct]: location and size of N-best list
    --show-weights: for mert-moses.pl, just call moses and exit

Other options (decoder flags) are passed through to moses instances
'''

# Defaults
INPUT = sys.stdin
PROCS = 1
THREADS = 1
EXTRA = 0
DONE = threading.Event()
PID = os.getpid()
# A very long time, used as Queue operation timeout even though we don't
# actually want a timeout but we do want interruptibility
# (https://bugs.python.org/issue1360)
NEVER = 60 * 60 * 24 * 365 * 1000

# Single unit of computation: decode a line, output result, signal done
Task = collections.namedtuple('Task', ['id', 'line', 'out', 'event'])


def kill_main(msg):
    '''kill -9 the main thread to stop everything immediately'''
    sys.stderr.write('{}\n'.format(msg))
    os.kill(PID, signal.SIGKILL)


def gzopen(f):
    '''Open plain or gzipped text'''
    return gzip.open(f, 'rb') if f.endswith('.gz') else open(f, 'r')


def run_instance(cmd_base, threads, tasks, cpu_affinity, cpu_offset, n_best=False):
    '''Run an instance of moses that processes tasks (input lines) from a
    queue using a specified number of threads'''
    cmd = cmd_base[:]
    cmd.append('--threads')
    cmd.append(str(threads))

    if cpu_affinity:
       cmd.append('--cpu-affinity-offset')
       cmd.append(str(cpu_offset))

    #print 'BEFORE'
    #print cmd
    #print 'AFTER\n'

    try:
        # Queue of tasks instance is currently working on, limited to the number
        # of threads * 2 (minimal buffering).  The queue should be kept full for
        # optimal CPU usage.
        work = Queue.Queue(maxsize=(threads * 2))
        # Multi-threaded instance
        moses = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)

        # Read and handle instance output as available
        def handle_output():
            while True:
                # Output line triggers task completion
                line = moses.stdout.readline()
                # End of output (instance finished)
                if not line:
                    break
                task = work.get(timeout=NEVER)
                if n_best:
                    # Read and copy lines until sentinel line, copy real line id
                    # id ||| hypothesis words  ||| feature scores ||| total score
                    (first_i, rest) = line.split(' ||| ', 1)
                    task.out.append(' ||| '.join((task.id, rest)))
                    while True:
                        line = moses.stdout.readline()
                        (i, rest) = line.split(' ||| ', 1)
                        # Sentinel
                        if i != first_i:
                            break
                        task.out.append(' ||| '.join((task.id, rest)))
                else:
                    task.out.append(line)
                # Signal task done
                task.event.set()
        # Output thread
        handler = threading.Thread(target=handle_output, args=())
        # Daemon: guaranteed to finish before non-daemons
        handler.setDaemon(True)
        handler.start()

        # Input thread: take tasks as they are available and add them to work
        # queue.  Stop when DONE encountered.
        while True:
            task = tasks.get(timeout=NEVER)
            work.put(task, timeout=NEVER)
            if task.event == DONE:
                break
            if n_best:
                # Input line followed by blank line (sentinel)
                moses.stdin.write(task.line)
                moses.stdin.write('\n')
            else:
                moses.stdin.write(task.line)

        # Cleanup
        moses.stdin.close()
        moses.wait()
        handler.join()

    except:
        kill_main('Error with moses instance: see stderr')


def write_results(results, n_best=False, n_best_out=None):
    '''Write out results (output lines) from a queue as they are populated'''
    while True:
        task = results.get(timeout=NEVER)
        if task.event == DONE:
            break
        task.event.wait()
        if n_best:
            # Write top-best and N-best
            # id ||| hypothesis words  ||| feature scores ||| total score
            top_best = task.out[0].split(' ||| ', 2)[1]
            # Except don't write top-best if writing N-best to stdout "-"
            if n_best_out != sys.stdout:
                sys.stdout.write('{}\n'.format(top_best))
                sys.stdout.flush()
            for line in task.out:
                n_best_out.write(line)
            n_best_out.flush()
        else:
            sys.stdout.write(task.out[0])
            sys.stdout.flush()


def main(argv):
    # Defaults
    moses_ini = None
    input = INPUT
    procs = PROCS
    threads = THREADS
    extra = EXTRA
    n_best = False
    n_best_file = None
    n_best_size = None
    n_best_distinct = False
    n_best_out = None
    show_weights = False
    cpu_affinity = False

    # Decoder command
    cmd = argv[1:]

    # Parse special options and remove from cmd
    i = 1
    while i < len(cmd):
        if cmd[i] in ('-f', '-config', '--config'):
            moses_ini = cmd[i + 1]
            # Do not remove from cmd
            i += 2
        elif cmd[i] in ('-i', '-input-file', '--input-file'):
            input = gzopen(cmd[i + 1])
            cmd = cmd[:i] + cmd[i + 2:]
        elif cmd[i] in ('-th', '-threads', '--threads'):
            # P:T:E
            args = cmd[i + 1].split(':')
            procs = int(args[0])
            if len(args) > 1:
                threads = int(args[1])
            if len(args) > 2:
                extra = int(args[2])
            cmd = cmd[:i] + cmd[i + 2:]
        elif cmd[i] in ('-n-best-list', '--n-best-list'):
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
        elif cmd[i] in ('-show-weights', '--show-weights'):
            show_weights = True
            # Do not remove from cmd
            i += 1
        elif cmd[i] in ('-cpu-affinity', '--cpu-affinity'):
            cpu_affinity = True
            cmd = cmd[:i] + cmd[i + 1:]
        else:
            i += 1

    # If mert-moses.pl passes -show-weights, just call moses
    if show_weights:
        sys.stdout.write(subprocess.check_output(cmd))
        sys.stdout.flush()
        return

    # Check inputs
    if not (len(cmd) > 0 and moses_ini):
        sys.stderr.write(HELP.format(os.path.basename(argv[0])))
        sys.exit(2)
    if not (os.path.isfile(cmd[0]) and os.access(cmd[0], os.X_OK)):
        raise Exception('moses "{}" is not executable\n'.format(cmd[0]))

    # Report settings
    sys.stderr.write('Moses flags: {}\n'.format(' '.join('\'{}\''.format(s) if ' ' in s else s for s in cmd[1:])))
    sys.stderr.write('Instances:   {}\n'.format(procs))
    sys.stderr.write('Threads per: {}\n'.format(threads))
    if extra:
        sys.stderr.write('Extra:       {}\n'.format(extra))
    if n_best:
        sys.stderr.write('N-best list: {} ({}{})\n'.format(n_best_file, n_best_size, ', distinct' if n_best_distinct else ''))

    # Task and result queues (buffer 8 * total threads input lines)
    tasks = Queue.Queue(maxsize=(8 * ((procs * threads) + extra)))
    results = Queue.Queue()

    # N-best capture
    if n_best:
        cmd.append('--n-best-list')
        cmd.append('-')
        cmd.append(n_best_size)
        if n_best_distinct:
            cmd.append('distinct')
        if n_best_file == '-':
            n_best_out = sys.stdout
        else:
            n_best_out = open(n_best_file, 'w')

    # Start instances
    cpu_offset = -threads
    instances = []
    for i in range(procs + (1 if extra else 0)):
        if cpu_affinity:
           cpu_offset += threads

        t = threading.Thread(target=run_instance, args=(cmd, (threads if i < procs else extra), tasks, cpu_affinity, cpu_offset, n_best))
        instances.append(t)
        # Daemon: guaranteed to finish before non-daemons
        t.setDaemon(True)
        t.start()
        #time.sleep(1)

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
        task = Task(str(id), line, [], threading.Event())
        results.put(task, timeout=NEVER)
        tasks.put(task, timeout=NEVER)
        id += 1

    # Tell instances to exit
    for t in instances:
        tasks.put(Task(None, None, None, DONE), timeout=NEVER)
    for t in instances:
        t.join()

    # Stop results writer
    results.put(Task(None, None, None, DONE), timeout=NEVER)
    writer.join()

    # Cleanup
    if n_best:
        n_best_out.close()


if __name__ == '__main__':
    try:
        main(sys.argv)
    except:
        kill_main('Error with main I/O: see stderr')
