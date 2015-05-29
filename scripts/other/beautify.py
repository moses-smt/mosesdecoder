#! /usr/bin/env python
#
# Originally written in 2015 by Jeroen Vermeulen (Precision Translation Tools).
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""Reformat project source code, and/or check for style errors ("lint").

Formatting requires "astyle" and GNU "sed" in your path.  A uniform exact
version of astyle is required.  Even slightly different versions can produce
many unwanted changes.

Lint checking requires "pocketlint" in your path.
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
    CalledProcessError,
    PIPE,
    Popen,
    )
import sys


# Name of the "ignore" file.
BEAUTIFY_IGNORE = '.beautify-ignore'


class LintCheckFailure(Exception):
    """Lint was found, or the lint checker otherwise returned failure."""
    exit_code = 1


class ProgramFailure(Exception):
    """The program failed, but it's not a bug.  No traceback."""
    exit_code = 2


class CommandLineError(Exception):
    """Something wrong with the command-line arguments."""
    exit_code = 3


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
            raise ProgramFailure(
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

    The command's standard output is written to standard output, but also
    returned.  Its standard error output is returned if the command succeeds,
    but printed to our own standard error output if it fails.

    :param command_line: List of command and its arguments.
    :param verbose: Print what you're doing?
    :param dry_run: Skip executing the command, just turn empty.
    :param **kwargs: Other keyword arguments are passed on to `Popen`.
    :return: Tuple of command's standard output and standard error output.
    :raises CalledProcessError: If command returns nonzero exit status.
    """
    if verbose:
        print("Running: " + '  '.join(command_line))
    if dry_run:
        return '', ''
    process = Popen(command_line, stdout=PIPE, stderr=PIPE, **kwargs)
    stdout, stderr = process.communicate()
    sys.stdout.write(stdout)
    if process.returncode != 0:
        sys.stderr.write(stderr)
        raise CalledProcessError(
            returncode=process.returncode, cmd=command_line,
            output=(
                "Command '%s' returned nonzero: %s\n(Output was: %s)"
                % (command_line, stderr, stdout)))
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
    :param ignore: A sequence of files and directories (relative to
        `root_dir`) that should not be included in the search.
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
            file_path = os.path.join(dirpath, filename)
            if not os.path.islink(file_path):
                # Skip symlinks.  If they point to something that we need to
                # process then we'll probably find it.  If it doesn't, then
                # we should ignore it anyway.  What we should definitely not
                # do is overwrite a symlink with a reformatted regular file.
                files.add(file_path)

    return sorted(files)


# Filename suffixes for C/C++ source files.
C_LIKE_SUFFIXES = [
    '.c',
    '.cc',
    '.cpp',
    '.cxx',
    '.h',
    '.hh',
    '.hpp',
    '.hxx',
]


# Filename suffixes for Perl files.
PERL_SUFFIXES = [
    '.cgi',
    '.perl',
    '.pl',
    '.pm',
    ]


# Filename suffixes for types of files where it's probably safe and
# desirable to strip trailing whitespace.
WHITESPACEABLE_SUFFIXES = C_LIKE_SUFFIXES + [
    '.js',
    '.md',
    '.php',
    ]


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
        raise ProgramFailure(
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


def run_perltidy(source_files, verbose=False, dry_run=False):
    """Run `perltidy` on the given Perl source files."""
    command_line = [
        'perltidy',
        # Repeat until formatting stops changing:
        '--converge',
        # Write "if ($foo)", not "if ( $foo )"
        '--paren-tightness=2',
        # Write "} else {", with 'else' on the same line as the braces.
        '--cuddled-else',
    ]
    try:
        _, stderr = run_command(
            command_line + source_files, verbose=verbose, dry_run=dry_run)
    except OSError as error:
        if error.errno == ENOENT:
            raise ProgramFailure(
                "Could not run 'perltidy'.  Make sure that it is installed.")
        else:
            raise
    if stderr != '':
        sys.stderr.write(stderr)

    # Success doesn't tell us much.  If there are errors in the file,
    # perltidy will still return success, but it will write an additional
    # output file with .ERR appended to the input file name.
    # When that happens, we don't trust that reformatting is safe.
    for org_file in source_files:
        tidy_file = org_file + '.tdy'
        fail_file = org_file + '.ERR'
        if not os.path.isfile(tidy_file):
            # File did not get reformatted for whatever reason.
            continue
        if os.path.isfile(fail_file):
            # There were failures in this file.  Don't trust the result;
            # keep the original.
            os.remove(tidy_file)
        else:
            # Yup, this file looks OK.  Overwrite the original.
            os.rename(tidy_file, org_file)


def strip_trailing_whitespace(files, verbose=False, dry_run=False):
    """Remove trailing whitespace from given text files.

    Uses the GNU `sed` command-line tool.  It must be in your path.

    :param files: A list of text files.
    :param verbose: Print what you're doing?
    :param dry_run: Don't actually make any changes.
    """
    command_line = ['sed', '-i', '-e', 's/[[:space:]]*$//', '--']
    run_command(command_line + files, verbose=verbose, dry_run=dry_run)


def chunk_file_list(files, files_at_a_time=20):
    """Iterator: break a list of files up into chunks.

    :param files: Paths of files to process.
    :param files_at_a_time: Maximum number of files to process in one
        command invocation.  Stops the command line from getting too long.
    """
    files = list(files)
    while files != []:
        yield files[:files_at_a_time]
        files = files[files_at_a_time:]


def format_source(root_dir, ignore, verbose=False, dry_run=False,
                  files_at_a_time=20, skip_astyle=False, skip_perltidy=False):
    """Reformat source code.

    Uses `astyle` for C and C++.  Also uses GNU `sed` to strip trailing
    whitespace.
    """
    if not skip_astyle:
        check_astyle_version(verbose=verbose)
        c_like_files = find_files(
            root_dir, ignore=ignore, suffixes=C_LIKE_SUFFIXES)
        for chunk in chunk_file_list(c_like_files, files_at_a_time):
            run_astyle(chunk, verbose=verbose, dry_run=dry_run)

    if not skip_perltidy:
        perl_files = find_files(
            root_dir, ignore=ignore, suffixes=PERL_SUFFIXES)
        for chunk in chunk_file_list(perl_files, files_at_a_time):
            run_perltidy(chunk, verbose=verbose, dry_run=dry_run)

    whitespace_files = find_files(
        root_dir, ignore=ignore, suffixes=WHITESPACEABLE_SUFFIXES)
    for chunk in chunk_file_list(whitespace_files, files_at_a_time):
        strip_trailing_whitespace(chunk, verbose=verbose, dry_run=dry_run)


def check_lint(root_dir, ignore, verbose, dry_run, files_at_a_time,
               max_line_len, continue_on_error):
    """Check for lint.

    Unless `continue_on_error` is selected, returns `False` on the first
    iteration where lint is found, or where the lint checker otherwise
    returned failure.

    :return: Whether the check found everything OK.
    """
    success = True
    # Suffixes for types of file that pocketlint can check for us.
    pocketlint_suffixes = C_LIKE_SUFFIXES + PERL_SUFFIXES + [
        '.ini',
        # Don't check for now.  Styles differ too much.
        # '.css',
        '.js',
        '.md',
        '.cgi',
        '.php',
        '.py',
        '.sh',
        ]
    lintable_files = find_files(
        root_dir, ignore=ignore, suffixes=pocketlint_suffixes)
    command_line = ['pocketlint', '-m', '%d' % max_line_len, '--']
    for chunk in chunk_file_list(lintable_files, files_at_a_time):
        try:
            run_command(
                command_line + chunk, verbose=verbose, dry_run=dry_run)
        except CalledProcessError:
            success = False

        if not success and not continue_on_error:
            return False

    return success


def parse_arguments():
    """Parse command-line arguments, return as Namespace object."""
    parser = ArgumentParser(description=__doc__)
    parser.add_argument(
        '--format', '-f', action='store_true',
        help="Format source code.")
    parser.add_argument(
        '--lint', '-l', action='store_true',
        help="Check for lint.")
    parser.add_argument(
        '--verbose', '-v', action='store_true',
        help="Print whatever is happening to standard output.")
    parser.add_argument(
        '--root-dir', '-r', metavar='DIR', default=getcwd(),
        help="Project root directory.  Defaults to current directory.")
    parser.add_argument(
        '--dry-run', '-d', action='store_true',
        help="Don't actually change any files.")
    parser.add_argument(
        '--files-at-a-time', '-n', type=int, metavar='NUMBER', default=20,
        help=(
            "Process NUMBER files in one command line.  "
            "Defaults to %(default)s."))
    parser.add_argument(
        '--max-line-len', '-m', type=int, metavar='NUMBER', default=400,
        help=(
            "Allow maximum line length of NUMBER characters.  Default is "
            "%(default)s, optimal for humans is said to be somewhere around "
            "72, conventional is 78-80."))
    parser.add_argument(
        '--ignore-lint-error', '-i', action='store_true',
        help="Continue checking even if lint is found.")
    parser.add_argument(
        '--skip-astyle', '-A', action='store_true',
        help="Don't run astyle when formatting.")
    parser.add_argument(
        '--skip-perltidy', '-P', action='store_true',
        help="Don't run perltidy when formatting.")
    return parser.parse_args()


def main():
    """Find and format source files."""
    args = parse_arguments()
    if not args.format and not args.lint:
        raise CommandLineError("Select action: --format, --lint, or both.")

    ignore = read_ignore_file(args.root_dir)

    if args.format:
        format_source(
            args.root_dir, ignore, verbose=args.verbose,
            dry_run=args.dry_run, files_at_a_time=args.files_at_a_time,
            skip_astyle=args.skip_astyle, skip_perltidy=args.skip_perltidy)

    if args.lint:
        success = check_lint(
            args.root_dir, ignore, verbose=args.verbose,
            dry_run=args.dry_run, files_at_a_time=args.files_at_a_time,
            max_line_len=args.max_line_len,
            continue_on_error=args.ignore_lint_error)
        if not success:
            raise LintCheckFailure("Lint check failed.")


if __name__ == '__main__':
    try:
        main()
    except (CommandLineError, LintCheckFailure, ProgramFailure) as error:
        # This is a failure, but not a bug.  Print a friendly error
        # message, not a traceback.
        sys.stderr.write('%s\n' % error)
        sys.exit(error.exit_code)
