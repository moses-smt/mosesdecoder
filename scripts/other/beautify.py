#! /usr/bin/env python

"""Reformat project source code.

This requires "astyle" and GNU "sed" in your path.  A uniform exact version of
astyle is required.  Even slightly different versions can produce many unwanted
changes.
"""

# This script should run on Python 2 (2.7 or better) or 3.  Try to keep it
# future-compatible.
from __future__ import (
    absolute_import,
    print_function,
    unicode_literals,
    )

from argparse import ArgumentParser
from errno import ENOENT
from os import (
    getcwd,
    walk,
    )
import os.path
from subprocess import (
    PIPE,
    Popen,
    )

# Name of the "ignore" file.
BEAUTIFY_IGNORE = '.beautify-ignore'


def read_ignore_file(root_dir):
    """Read the `BEAUTIFY_IGNORE` file from `root_dir`.

    :param root_dir: Root source directory.
    :return: A list of path prefixes that this script should ignore.
    """
    ignore_file = os.path.join(root_dir, BEAUTIFY_IGNORE)
    try:
        with open(ignore_file) as ignore_file:
            ignore_contents = ignore_file.read()
    except IOError as error:
        if error.errno == ENOENT:
            raise Exception(
                "No .gitignore file found in %s.  "
                "Is it really the project's root directory?"
                % root_dir)
        else:
            raise
    ignore_contents = ignore_contents.decode('utf-8')
    prefixes = []
    for line in ignore_contents.splitlines():
        if line != '' and not line.startswith('#'):
            prefixes.append(line.strip())
    return prefixes


def run_command(command_line, verbose=False, dry_run=False, **kwargs):
    """Run a command through `subprocess.Popen`.

    :param command_line: List of command and its arguments.
    :param verbose: Print what you're doing?
    :param dry_run: Skip executing the command, just turn empty.
    :param **kwargs: Other keyword arguments are passed on to `Popen`.
    :return: Tuple of command's standard output and standard error output.
    :raises Exception: If command returns nonzero exit status.
    """
    if verbose:
        print("Running: " + '  '.join(command_line))
    if dry_run:
        return ''
    process = Popen(command_line, stdout=PIPE, stderr=PIPE, **kwargs)
    stdout, stderr = process.communicate()
    if process.returncode != 0:
        raise Exception(
            "Command '%s' returned nonzero: %s\n(Output was: %s)"
            % (command_line, stderr, stdout))
    return stdout, stderr


def matches_prefix(dirpath, filename, prefixes):
    """Does `dirpath/filename` match any of `prefixes`?"""
    full_path = os.path.join(dirpath, filename)
    for prefix in prefixes:
        if full_path == prefix:
            # Exact match on the full path.
            return True
        if full_path.startswith(prefix + '/'):
            # File is in a directory that matches a prefix.
            # (We have to add our own slash, or "foo/bar" will match prefix
            # "fo", which is probably not what you want.)
            return True
    return False


def find_files(root_dir, ignore=None, suffixes=None):
    """Find files meeting the given criteria.

    :param root_dir: Root source directory.
    :param ignore: A sequence of files and directories (relative to `root_dir`)
        that should not be included in the search.
    :param suffixes: Filename suffixes that you're looking for.  Only files
        with the given suffix in their name will be returned.
    """
    root_dir = os.path.abspath(os.path.normpath(root_dir))
    if ignore is None:
        ignore = []
    ignore = [
        os.path.join(root_dir, os.path.normpath(path))
        for path in ignore]
    if suffixes is None:
        suffixes = []
    suffixes = frozenset(suffixes)

    files = set()
    for dirpath, _, filenames in walk(os.path.normpath(root_dir)):
        dirpath = os.path.normpath(dirpath)
        for filename in filenames:
            if matches_prefix(dirpath, filename, ignore):
                continue
            _, suffix = os.path.splitext(filename)
            if suffix not in suffixes:
                continue
            files.add(os.path.join(dirpath, filename))

    return sorted(files)


def list_c_like_files(root_dir, ignore):
    """Return a list of C/C++ source files to beautify.

    This is currently still tailored for the Moses source tree.  We may
    generalize it later so we can run it on other codebases as well.

    Calls GNU `find` to find files.  It must be in your path.

    :param root_dir: Root source directory.
    :param ignore: Sequence of path prefixes that should be ignored.
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

    return find_files(root_dir, ignore=ignore, suffixes=c_like_suffixes)


EXPECTED_ASTYLE_VERSION = "Artistic Style Version 2.01"


def check_astyle_version(verbose=False):
    """Run `astyle`, to see if it returns the expected version number.

    This matters, because small changes in version numbers can come with
    enormous diffs.

    :raises Exception: If `astyle` is not the expected version.
    """
    # We'll be parsing astyle's output.  Run with C locale to avoid getting
    # translated output.
    # The output goes to stderr.
    _, version = run_command(
        ['astyle', '--version'], verbose=verbose, env={'LC_ALL': 'C'})
    version = version.strip()
    if version != EXPECTED_ASTYLE_VERSION:
        raise Exception(
            "Wrong astyle version.  "
            "Expected '%s', but got version string '%s'."
            % (EXPECTED_ASTYLE_VERSION, version))


def run_astyle(source_files, verbose=False, dry_run=False):
    """Run `astyle` on the given C/C++ source files."""
    command_line = ['astyle', '--style=k&r', '-s2']
    if verbose:
        command_line += ['-v']
    run_command(
        command_line + source_files, verbose=verbose, dry_run=dry_run)


def list_whitespaceable_files(root_dir, ignore):
    """Return a list of files where we should clean up whitespace.

    This includes C/C++ source files, but possibly also other file types.

    :param root_dir: Root source directory.
    :param ignore: Sequence of path prefixes that should be ignored.
    """
    return list_c_like_files(root_dir, ignore=ignore)


def strip_trailing_whitespace(files, verbose=False, dry_run=False):
    """Remove trailing whitespace from given text files.

    Uses the GNU `sed` command-line tool.  It must be in your path.

    :param files: A list of text files.
    :param verbose: Print what you're doing?
    :param dry_run: Don't actually make any changes.
    """
    command_line = ['sed', '-i', '-e', 's/[[:space:]]*$//', '--']
    run_command(command_line + files, verbose=verbose, dry_run=dry_run)


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
    parser = ArgumentParser(description=__doc__)
    parser.add_argument(
        '--verbose', '-v', action='store_true',
        help="Print whatever is happening to standard output.")
    parser.add_argument(
        '--root-dir', '-r', metavar='DIR', default=getcwd(),
        help="Project root directory.  Defaults to current directory.")
    parser.add_argument(
        '--dry-run', '-n', action='store_true',
        help="Don't actually change any files.")
    return parser.parse_args()


def main():
    """Find and format source files."""
    args = parse_arguments()

    ignore = read_ignore_file(args.root_dir)
    check_astyle_version(verbose=args.verbose)

    c_like_files = list_c_like_files(args.root_dir, ignore=ignore)
    for chunk in chunk_file_list(c_like_files):
        run_astyle(chunk, verbose=args.verbose, dry_run=args.dry_run)

    whitespace_files = list_whitespaceable_files(args.root_dir, ignore=ignore)
    for chunk in chunk_file_list(whitespace_files):
        strip_trailing_whitespace(
            chunk, verbose=args.verbose, dry_run=args.dry_run)


if __name__ == '__main__':
    main()
