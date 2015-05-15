#! /usr/bin/env python

"""Reformat source code."""

# This script should run on Python 2 (2.7 or better) or 3.  Try to keep it
# future-compatible.
from __future__ import (
    absolute_import,
    print_function,
    unicode_literals,
    )

from argparse import ArgumentParser
from os import (
    getcwd,
    listdir,
    walk,
    )
import os.path
from subprocess import (
    PIPE,
    Popen,
    )


def run_command(command_line, verbose=False, not_really=False, **kwargs):
    """Run a command through `subprocess.Popen`.

    :param command_line: List of command and its arguments.
    :param verbose: Print what you're doing?
    :param not_really: Skip executing the command, just turn empty.
    :param **kwargs: Other keyword arguments are passed on to `Popen`.
    :return: Command's standard output.
    :raises Exception: If command returns nonzero exit status.
    """
    if verbose:
        print("Running: " + '  '.join(command_line))
    if not_really:
        return ''
    process = Popen(command_line, stdout=PIPE, stderr=PIPE, **kwargs)
    stdout, stderr = process.communicate()
    if process.returncode != 0:
        raise Exception(
            "Command '%s' returned nonzero: %s\n(Output was: %s)"
            % (command_line, stderr, stdout))
    return stdout


def find_files(root_dir, skip_at_root=None, suffixes=None):
    """Find files meeting the given criteria.

    :param root_dir: Root source directory.
    :param skip_at_root: A sequence of files and directories at the
        root level that should not be included in the search.
    :param suffixes: Filename suffixes that you're looking for.  Only files
        with the given suffix in their name will be returned.
    """
    if skip_at_root is None:
        skip_at_root = []
    skip_at_root = [os.path.normpath(path) for path in skip_at_root]
    if suffixes is None:
        suffixes = []
    suffixes = frozenset(suffixes)

    files = set()
    starting_points = [
        os.path.normpath(os.path.join(root_dir, path))
        for path in sorted(set(listdir(root_dir)) - set(skip_at_root))]
    for entry in starting_points:
        for dirpath, _, filenames in walk(entry):
            for filename in filenames:
                _, suffix = os.path.splitext(filename)
                if suffix in suffixes:
                    files.add(os.path.join(dirpath, filename))

    return sorted(files)


def list_c_like_files(root_dir):
    """Return a list of C/C++ source files to beautify.

    This is currently still tailored for the Moses source tree.  We may
    generalize it later so we can run it on other codebases as well.

    Calls GNU `find` to find files.  It must be in your path.

    :param root_dir: Root source directory.
    """
    # Filename suffixes for C/C++ source files.
    c_like_suffixes = {
        '.c',
        '.cc',
        '.cpp',
        '.cxx',
        '.h',
        '.hh',
        '.hpp',
    }

    # Entries in the root directory that we should ignore.  Usually the
    # reason is that they contain source that is maintained elsewhere.
    skip_at_root = {
        'boost',
        'contrib',
        'irstlm',
        'jam-files',
        'lm',
        'pcfg-common',
        'randlm',
        'search',
        'srilm',
        'syntax-common',
        'UG',
        'util',
        'xmlrpc-c',
        '.git',
    }
    return find_files(
        root_dir, skip_at_root=skip_at_root, suffixes=c_like_suffixes)


EXPECTED_ASTYLE_VERSION = "Artistic Style Version 2.01"


def check_astyle_version(verbose=False, not_really=False):
    """Run `astyle`, to see if it returns the expected version number.

    :raises Exception: If `astyle` is not the expected version.
    """
    # We'll be parsing astyle's output.  Run with C locale to avoid getting
    # translated output.
    output = run_command(
        ['astyle', '--version'], verbose=verbose, not_really=not_really,
        env={'LC_ALL': 'C'})
    output = output.strip()
    if not_really and output != EXPECTED_ASTYLE_VERSION:
        raise Exception(
            "Expected astyle 2.01, but got version string '%s'." % output)


def run_astyle(source_files, verbose=False, not_really=False):
    """Run `astyle` on the given C/C++ source files."""
    command_line = ['astyle', '--style=k&r', '-s2']
    if verbose:
        command_line += ['-v']
    run_command(
        command_line + source_files, verbose=verbose, not_really=not_really)


def list_whitespaceable_files(root_dir):
    """Return a list of files where we should clean up whitespace.

    This includes C/C++ source files, but possibly also other file types.
    """
    return list_c_like_files(root_dir)


def strip_trailing_whitespace(files, verbose=False, not_really=False):
    """Remove trailing whitespace from given text files.

    Uses the GNU `sed` command-line tool.  It must be in your path.

    :param files: A list of text files.
    :param verbose: Print what you're doing?
    :param not_really: Don't actually make any changes.
    """
    command_line = ['sed', '-i', '-e', 's/[[:space:]]*$//', '--']
    run_command(command_line + files, verbose=verbose, not_really=not_really)


def chunk_file_list(files, files_per_run=20):
    """Iterator: break a list of files up into chunks.

    :param files: Paths of files to process.
    :param files_per_run: Maximum number of files to process in one
        command invocation.  Stops the command line from getting too long.
    """
    files = list(files)
    while files != []:
        yield files[:files_per_run]
        files = files[files_per_run:]


def parse_arguments():
    """Parse command-line arguments, return as Namespace object."""
    parser = ArgumentParser(description="Reformat the Moses source code.")
    parser.add_argument(
        '--verbose', '-v', action='store_true',
        help="Print whatever is happening to standard output.")
    parser.add_argument(
        '--root-dir', '-r', metavar='DIR', default=getcwd(),
        help="Project root directory.  Defaults to current directory.")
    parser.add_argument(
        '--any-astyle', action='store_true',
        help="Accept any version of astyle.")
    parser.add_argument(
        '--not-really', '-n', action='store_true',
        help="Don't actually change any files.")
    return parser.parse_args()


def main():
    """Find and format source files."""
    args = parse_arguments()

    if not args.any_astyle:
        check_astyle_version(verbose=args.verbose)

    c_like_files = list_c_like_files(args.root_dir)
    for chunk in chunk_file_list(c_like_files):
        run_astyle(chunk, verbose=args.verbose, not_really=args.not_really)

    whitespace_files = list_whitespaceable_files(args.root_dir)
    for chunk in chunk_file_list(whitespace_files):
        strip_trailing_whitespace(
            chunk, verbose=args.verbose, not_really=args.not_really)


if __name__ == '__main__':
    main()
