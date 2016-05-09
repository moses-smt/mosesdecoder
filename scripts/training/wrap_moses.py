#!/usr/bin/env python
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

r'''Generic Moses Wrapper

Run moses, wrapping various inputs and outputs
(useful as decoder-executable for mert-moses.pl)

mert-moses.pl \
  --decoder wrap_moses.py --input src --refs ref --config moses.ini \
  --decoder-flags="--wrap-input-file my_preproc_script.sh \
  --wrap-n-best-list my_postproc_script.sh"

Commands are run through shell, so they may contain multiple piped commands

Anything not in the following list is passed through to moses as decoder flags
'''

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

# ../../bin/moses
MOSES = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__)))), 'bin', 'moses')


def popen(cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE):
    '''Open command for streaming'''
    return subprocess.Popen(cmd, shell=shell, stdin=stdin, stdout=stdout)


def main():

    # Special args
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--moses', help='Moses executable (default: {})'.format(MOSES),
        default=MOSES)
    parser.add_argument(
        '--tmp', help='Temp directory parent (default: /tmp)', default='/tmp')
    parser.add_argument(
        '--wrap-input-file', metavar='CMD',
        help='Pipe input file through this command')
    parser.add_argument(
        '--wrap-n-best-list', metavar='CMD',
        help='Pipe n-best list through this command')
    parser.add_argument(
        '--wrap-stdin', metavar='CMD', help='Pipe stdin through this command')
    parser.add_argument(
        '--wrap-stdout', metavar='CMD', help='Pipe stdout through this command')

    # Help message
    if len(sys.argv) == 1:
        sys.stderr.write(__doc__)
        parser.print_help()
        sys.exit(2)

    # Parse/split args
    (args, moses_args) = parser.parse_known_args()
    moses_arg_set = set(moses_args)

    # If mert-moses.pl passes -show-weights, just call moses
    if '--show-weights' in moses_arg_set or '-show-weights' in moses_arg_set:
        sys.stdout.write(subprocess.check_output([args.moses] + moses_args))
        sys.stdout.flush()
        sys.exit(0)

    # Scan moses args and sanity check
    input_file = None
    input_file_i = None
    n_best_list = None
    n_best_list_i = None
    if not os.path.exists(args.moses):
        sys.stderr.write(
            'Error: cannot find moses executable at "{}", '
            'specify with --moses\n'.format(args.moses))
        sys.exit(1)
    if args.wrap_input_file and args.wrap_stdin:
        sys.stderr.write(
            'Error: cannot use both --wrap-input-file and --wrap-stdin\n')
        sys.exit(1)
    if args.wrap_input_file:
        try:
            input_file_i = moses_args.index('--input-file') + 1
        except ValueError:
            sys.stderr.write(
                'Error: --wrap-input-file requires --input-file\n')
            sys.exit(1)
        input_file = moses_args[input_file_i]
    if args.wrap_n_best_list:
        try:
            n_best_list_i = moses_args.index('--n-best-list') + 1
        except ValueError:
            try:
                n_best_list_i = moses_args.index('-n-best-list') + 1
            except ValueError:
                sys.stderr.write(
                    'Error: --wrap-n-best-list requires --n-best-list\n')
                sys.exit(1)
        n_best_list = moses_args[n_best_list_i]
    # Don't read from stdin if input file specified
    stream_input = not (
        '--input-file' in moses_arg_set or '-input-file' in moses_arg_set
        or '-i' in moses_arg_set)

    # Setup temp dir
    tmp = tempfile.mkdtemp(prefix=os.path.join(args.tmp, 'moses.'))

    # Preprocess input
    moses_input_file = None
    if args.wrap_input_file:
        moses_input_file = os.path.join(tmp, 'input_file')
        subprocess.call('{} <{} >{}'.format(
            args.wrap_input_file, input_file, moses_input_file), shell=True)
    # Postprocess file name
    moses_n_best_list = os.path.join(tmp, 'n_best_list')

    # Moses command
    moses_cmd = moses_args[:]
    if args.wrap_input_file:
        moses_cmd[input_file_i] = moses_input_file
    if args.wrap_n_best_list:
        moses_cmd[n_best_list_i] = moses_n_best_list
    moses_cmd = [args.moses] + moses_cmd

    # Start processes
    wrap_stdin = None
    moses_stdin = subprocess.PIPE
    if args.wrap_stdin:
        wrap_stdin = popen(args.wrap_stdin)
        moses_stdin = wrap_stdin.stdout
    moses = None
    wrap_stdout = None
    if args.wrap_stdout:
        # Wrap stdout
        moses = popen(moses_cmd, shell=False, stdin=moses_stdin)
        wrap_stdout = popen(
            args.wrap_stdout, stdin=moses.stdout, stdout=sys.stdout)
    else:
        # Don't wrap stdout
        moses = popen(
            moses_cmd, shell=False, stdin=moses_stdin, stdout=sys.stdout)

    # Run pipeline
    stdin = wrap_stdin.stdin if wrap_stdin else moses.stdin
    if stream_input:
        while True:
            line = sys.stdin.readline()
            if not line:
                break
            stdin.write(line)
            stdin.flush()
    stdin.close()
    if wrap_stdin:
        wrap_stdin.wait()
    moses.wait()
    if wrap_stdout:
        wrap_stdout.wait()

    # Postprocess n-best list
    if args.wrap_n_best_list:
        subprocess.call('{} <{} >{}'.format(
            args.wrap_n_best_list, moses_n_best_list, n_best_list), shell=True)

    # Cleanup
    shutil.rmtree(tmp)


if __name__ == '__main__':
    main()
