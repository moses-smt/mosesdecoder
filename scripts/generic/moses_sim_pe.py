#!/usr/bin/env python

# Written by Michael Denkowski
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""Parallelize decoding with simulated post-editing via moses XML input.

(XML entities need to be escaped in tokenization).  Memory mapped
dynamic phrase tables (Ulrich Germann,
www.statmt.org/moses/?n=Moses.AdvancedFeatures#ntoc40) and language models
(Kenneth Heafield,
http://www.statmt.org/moses/?n=FactoredTraining.BuildingLanguageModel#ntoc19)
facilitate memory efficient multi process decoding.  Input is divided into
batches, each of which is decoded sequentially.  Each batch pre-loads the
data from previous batches.

To use in tuning, run mert-moses.pl with --sim-pe=SYMAL where SYMAL is the
alignment from input to references.  Specify the number of jobs with
--decoder-flags="-threads N".
"""

import gzip
import itertools
import math
import os
import shutil
import subprocess
import sys
import tempfile
import threading

HELP = '''Moses with simulated post-editing

Usage:
    {} moses-cmd -config moses.ini -input-file text.src -ref text.tgt \
    -symal text.src-tgt.symal [options] [decoder flags]

Options:
    -threads N: number of decoders to run in parallel \
(default read from moses.ini, 1 if not present)
    -n-best-list nbest.out N [distinct]: location and size of N-best list
    -show-weights: for mert-moses.pl, just call moses and exit
    -tmp: location of temp directory (default /tmp)

Other options (decoder flags) are passed through to moses-cmd\n'''


class ProgramFailure(Exception):
    """Known kind of failure, with a known presentation to the user.

    Error message will be printed, and the program will return an error,
    but no traceback will be shown to the user.
    """


class Progress:
    """Provides progress bar."""

    def __init__(self):
        self.i = 0
        self.lock = threading.Lock()

    def inc(self):
        self.lock.acquire()
        self.i += 1
        if self.i % 100 == 0:
            sys.stderr.write('.')
            if self.i % 1000 == 0:
                sys.stderr.write(' [{}]\n'.format(self.i))
            sys.stderr.flush()
        self.lock.release()

    def done(self):
        self.lock.acquire()
        if self.i % 1000 != 0:
            sys.stderr.write('\n')
        self.lock.release()


def atomic_io(cmd, in_file, out_file, err_file, prog=None):
    """Run with atomic (synchronous) I/O."""
    with open(in_file, 'r') as inp, open(out_file, 'w') as out, open(err_file, 'w') as err:
        p = subprocess.Popen(
            cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=err)
        while True:
            line = inp.readline()
            if not line:
                break
            p.stdin.write(line)
            out.write(p.stdout.readline())
            out.flush()
            if prog:
                prog.inc()
        p.stdin.close()
        p.wait()


def gzopen(f):
    """Open plain or gzipped text."""
    return gzip.open(f, 'rb') if f.endswith('.gz') else open(f, 'r')


def wc(f):
    """Word count."""
    i = 0
    for line in gzopen(f):
        i += 1
    return i


def write_gzfile(lines, f):
    """Write lines to gzipped file."""
    out = gzip.open(f, 'wb')
    for line in lines:
        out.write('{}\n'.format(line))
    out.close()


def main(argv):
    # Defaults
    moses_ini = None
    moses_ini_lines = None
    text_src = None
    text_tgt = None
    text_symal = None
    text_len = None
    threads_found = False
    threads = 1
    n_best_out = None
    n_best_size = None
    n_best_distinct = False
    hg_ext = None
    hg_dir = None
    tmp_dir = '/tmp'
    xml_found = False
    xml_input = 'exclusive'
    show_weights = False
    mmsapt_dynamic = []
    mmsapt_static = []
    mmsapt_l1 = None
    mmsapt_l2 = None

    # Decoder command
    cmd = argv[1:]

    # Parse special options and remove from cmd
    i = 1
    while i < len(cmd):
        if cmd[i] in ('-f', '-config'):
            moses_ini = cmd[i + 1]
            cmd = cmd[:i] + cmd[i + 2:]
        elif cmd[i] in ('-i', '-input-file'):
            text_src = cmd[i + 1]
            cmd = cmd[:i] + cmd[i + 2:]
        elif cmd[i] == '-ref':
            text_tgt = cmd[i + 1]
            cmd = cmd[:i] + cmd[i + 2:]
        elif cmd[i] == '-symal':
            text_symal = cmd[i + 1]
            cmd = cmd[:i] + cmd[i + 2:]
        elif cmd[i] in ('-th', '-threads'):
            threads_found = True
            threads = int(cmd[i + 1])
            cmd = cmd[:i] + cmd[i + 2:]
        elif cmd[i] == '-n-best-list':
            n_best_out = cmd[i + 1]
            n_best_size = cmd[i + 2]
            # Optional "distinct"
            if i + 3 < len(cmd) and cmd[i + 3] == 'distinct':
                n_best_distinct = True
                cmd = cmd[:i] + cmd[i + 4:]
            else:
                cmd = cmd[:i] + cmd[i + 3:]
        elif cmd[i] == '-output-search-graph-hypergraph':
            # cmd[i + 1] == true
            hg_ext = cmd[i + 2]
            if i + 3 < len(cmd) and cmd[i + 3][0] != '-':
                hg_dir = cmd[i + 3]
                cmd = cmd[:i] + cmd[i + 4:]
            else:
                hg_dir = 'hypergraph'
                cmd = cmd[:i] + cmd[i + 3:]
        elif cmd[i] == '-tmp':
            tmp_dir = cmd[i + 1]
            cmd = cmd[:i] + cmd[i + 2:]
        # Handled specially to make sure XML input is turned on somewhere
        elif cmd[i] in ('-xi', '-xml-input'):
            xml_found = True
            xml_input = cmd[i + 1]
            cmd = cmd[:i] + cmd[i + 2:]
        # Handled specially for mert-moses.pl
        elif cmd[i] == '-show-weights':
            show_weights = True
            # Do not remove from cmd
            i += 1
        else:
            i += 1

    # Read moses.ini
    if moses_ini:
        moses_ini_lines = [line.strip() for line in open(moses_ini, 'r')]
        i = 0
        while i < len(moses_ini_lines):
            # PhraseDictionaryBitextSampling name=TranslationModel0
            # output-factor=0 num-features=7 path=corpus. L1=src L2=tgt
            # pfwd=g pbwd=g smooth=0 sample=1000 workers=1
            if moses_ini_lines[i].startswith('PhraseDictionaryBitextSampling'):
                for (k, v) in (pair.split('=') for pair in moses_ini_lines[i].split()[1:]):
                    if k == 'name':
                        # Dynamic means update this model
                        if v.startswith('Dynamic'):
                            mmsapt_dynamic.append(v)
                            moses_ini_lines[i] += '{mmsapt_extra}'
                        else:
                            mmsapt_static.append(v)
                    elif k == 'L1':
                        if mmsapt_l1 and v != mmsapt_l1:
                            raise ProgramFailure(
                                'Error: All PhraseDictionaryBitextSampling '
                                'entries should have same L1: '
                                '{} != {}\n'.format(v, mmsapt_l1))
                        mmsapt_l1 = v
                    elif k == 'L2':
                        if mmsapt_l2 and v != mmsapt_l2:
                            raise ProgramFailure(
                                'Error: All PhraseDictionaryBitextSampling '
                                'entries should have same L2: '
                                '{} != {}\n'.format(v, mmsapt_l2))
                        mmsapt_l2 = v
            # [threads]
            # 8
            elif moses_ini_lines[i] == '[threads]':
                # Prefer command line over moses.ini
                if not threads_found:
                    threads = int(moses_ini_lines[i + 1])
                i += 1
            # [xml-input]
            # exclusive
            elif moses_ini_lines[i] == '[xml-input]':
                # Prefer command line over moses.ini
                if not xml_found:
                    xml_found = True
                    xml_input = moses_ini_lines[i + 1]
                i += 1
            i += 1

    # If mert-moses.pl passes -show-weights, just call moses
    if show_weights:
        # re-append original moses.ini
        cmd.append('-config')
        cmd.append(moses_ini)
        sys.stdout.write(subprocess.check_output(cmd))
        sys.stdout.flush()
        sys.exit(0)

    # Input length
    if text_src:
        text_len = wc(text_src)

    # Check inputs
    if not (len(cmd) > 0 and all((moses_ini, text_src, text_tgt, text_symal))):
        sys.stderr.write(HELP.format(argv[0]))
        sys.exit(2)
    if not (os.path.isfile(cmd[0]) and os.access(cmd[0], os.X_OK)):
        raise ProgramFailure(
            'Error: moses-cmd "{}" is not executable\n'.format(cmd[0]))
    if not mmsapt_dynamic:
        raise ProgramFailure((
            'Error: no PhraseDictionaryBitextSampling entries named '
            '"Dynamic..." found in {}.  See '
            'http://www.statmt.org/moses/?n=Moses.AdvancedFeatures#ntoc40\n'
            ).format(moses_ini))
    if wc(text_tgt) != text_len or wc(text_symal) != text_len:
        raise ProgramFailure(
            'Error: length mismatch between "{}", "{}", and "{}"\n'.format(
                text_src, text_tgt, text_symal))

    # Setup
    work_dir = tempfile.mkdtemp(prefix='moses.', dir=os.path.abspath(tmp_dir))
    threads = min(threads, text_len)
    batch_size = int(math.ceil(float(text_len) / threads))

    # Report settings
    sys.stderr.write(
        'Moses flags: {}\n'.format(
            ' '.join('\'{}\''.format(s) if ' ' in s else s for s in cmd[1:])))
    for (i, n) in enumerate(mmsapt_dynamic):
        sys.stderr.write(
            'Dynamic mmsapt {}: {} {} {}\n'.format(
                i, n, mmsapt_l1, mmsapt_l2))
    for (i, n) in enumerate(mmsapt_static):
        sys.stderr.write(
            'Static mmsapt {}: {} {} {}\n'.format(i, n, mmsapt_l1, mmsapt_l2))
    sys.stderr.write('XML mode: {}\n'.format(xml_input))
    sys.stderr.write(
        'Inputs: {} {} {} ({})\n'.format(
            text_src, text_tgt, text_symal, text_len))
    sys.stderr.write('Jobs: {}\n'.format(threads))
    sys.stderr.write('Batch size: {}\n'.format(batch_size))
    if n_best_out:
        sys.stderr.write(
            'N-best list: {} ({}{})\n'.format(
                n_best_out, n_best_size,
                ', distinct' if n_best_distinct else ''))
    if hg_dir:
        sys.stderr.write('Hypergraph dir: {} ({})\n'.format(hg_dir, hg_ext))
    sys.stderr.write('Temp dir: {}\n'.format(work_dir))

    # Accumulate seen lines
    src_lines = []
    tgt_lines = []
    symal_lines = []

    # Current XML source file
    xml_out = None

    # Split into batches.  Each batch after 0 gets extra files with data from
    # previous batches.
    # Data from previous lines in the current batch is added using XML input.
    job = -1
    lc = -1
    lines = itertools.izip(
        gzopen(text_src), gzopen(text_tgt), gzopen(text_symal))
    for (src, tgt, symal) in lines:
        (src, tgt, symal) = (src.strip(), tgt.strip(), symal.strip())
        lc += 1
        if lc % batch_size == 0:
            job += 1
            xml_file = os.path.join(work_dir, 'input.{}.xml'.format(job))
            extra_src_file = os.path.join(
                work_dir, 'extra.{}.{}.txt.gz'.format(job, mmsapt_l1))
            extra_tgt_file = os.path.join(
                work_dir, 'extra.{}.{}.txt.gz'.format(job, mmsapt_l2))
            extra_symal_file = os.path.join(
                work_dir, 'extra.{}.{}-{}.symal.gz'.format(
                    job, mmsapt_l1, mmsapt_l2))
            if job > 0:
                xml_out.close()
                write_gzfile(src_lines, extra_src_file)
                write_gzfile(tgt_lines, extra_tgt_file)
                write_gzfile(symal_lines, extra_symal_file)
            xml_out = open(xml_file, 'w')
            ini_file = os.path.join(work_dir, 'moses.{}.ini'.format(job))
            with open(ini_file, 'w') as moses_ini_out:
                if job == 0:
                    extra = ''
                else:
                    extra = ' extra={}'.format(
                        os.path.join(work_dir, 'extra.{}.'.format(job)))
                moses_ini_out.write(
                    '{}\n'.format(
                        '\n'.join(moses_ini_lines).format(mmsapt_extra=extra)))
        src_lines.append(src)
        tgt_lines.append(tgt)
        symal_lines.append(symal)
        # Lines after first start with update tag including previous
        # translation.
        # Translation of last line of each batch is included in extra for
        # next batch.
        xml_tags = []
        if lc % batch_size != 0:
            tag_template = (
                '<update '
                'name="{}" source="{}" target="{}" alignment="{}" /> ')
            for n in mmsapt_dynamic:
                # Note: space after tag.
                xml_tags.append(
                    tag_template.format(
                        n, src_lines[-2], tgt_lines[-2], symal_lines[-2]))
        xml_out.write('{}{}\n'.format(''.join(xml_tags), src))
    xml_out.close()

    # Run decoders in parallel
    workers = []
    prog = Progress()
    for i in range(threads):
        work_cmd = cmd[:]
        work_cmd.append('-config')
        work_cmd.append(os.path.join(work_dir, 'moses.{}.ini'.format(i)))
        # Workers use 1 CPU each
        work_cmd.append('-threads')
        work_cmd.append('1')
        if not xml_found:
            work_cmd.append('-xml-input')
            work_cmd.append(xml_input)
        if n_best_out:
            work_cmd.append('-n-best-list')
            work_cmd.append(os.path.join(work_dir, 'nbest.{}'.format(i)))
            work_cmd.append(str(n_best_size))
            if n_best_distinct:
                work_cmd.append('distinct')
        if hg_dir:
            work_cmd.append('-output-search-graph-hypergraph')
            work_cmd.append('true')
            work_cmd.append(hg_ext)
            work_cmd.append(os.path.join(work_dir, 'hg.{}'.format(i)))
        in_file = os.path.join(work_dir, 'input.{}.xml'.format(i))
        out_file = os.path.join(work_dir, 'out.{}'.format(i))
        err_file = os.path.join(work_dir, 'err.{}'.format(i))
        t = threading.Thread(
            target=atomic_io,
            args=(work_cmd, in_file, out_file, err_file, prog))
        workers.append(t)
        t.start()
    # Wait for all to finish
    for t in workers:
        t.join()
    prog.done()

    # Gather N-best lists
    if n_best_out:
        with open(n_best_out, 'w') as out:
            for i in range(threads):
                path = os.path.join(work_dir, 'nbest.{}'.format(i))
                for line in open(path, 'r'):
                    entry = line.partition(' ')
                    out.write(
                        '{} {}'.format(
                            int(entry[0]) + (i * batch_size), entry[2]))

    # Gather hypergraphs
    if hg_dir:
        if not os.path.exists(hg_dir):
            os.mkdir(hg_dir)
        shutil.copy(
            os.path.join(work_dir, 'hg.0', 'weights'),
            os.path.join(hg_dir, 'weights'))
        for i in range(threads):
            for j in range(batch_size):
                shutil.copy(
                    os.path.join(
                        work_dir, 'hg.{}'.format(i),
                        '{}.{}'.format(j, hg_ext)),
                    os.path.join(
                        hg_dir, '{}.{}'.format((i * batch_size) + j, hg_ext)))

    # Gather stdout
    for i in range(threads):
        for line in open(os.path.join(work_dir, 'out.{}'.format(i)), 'r'):
            sys.stdout.write(line)

    # Cleanup
    shutil.rmtree(work_dir)

if __name__ == '__main__':
    try:
        main(sys.argv)
    except ProgramFailure as error:
        sys.stderr.write("%s\n" % error)
        sys.exit(1)
