#! /usr/bin/env python
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.
#
# Script contributed by Precision Translation Tools.

"""Run Moses `score` jobs in parallel.

This script is a replacement for `score-parallel.perl`.  The two are similar,
but there are differences in usage.  In addition, this script can be called
directly from Python code without the need to run it as a separate process.
"""

from __future__ import (
    absolute_import,
    print_function,
    unicode_literals,
    )

__metaclass__ = type

from argparse import ArgumentParser
from contextlib import contextmanager
from datetime import datetime
import errno
import gzip
from multiprocessing import Pool
import os
import os.path
import pipes
from shutil import rmtree
from subprocess import check_call
import sys
import tempfile


def get_unicode_type():
    """Return the Unicode string type appropriate to this Python version."""
    if sys.version_info.major <= 2:
        # Unicode string type.  In Python 2 this is the "unicode" type,
        # while "str" is a binary string type.
        return unicode
    else:
        # Unicode string type.  In Python 3 this is the default "str" type.
        # The binary string type is now called "bytes".
        return str


UNICODE_TYPE = get_unicode_type()


class CommandLineError(Exception):
    """Invalid command line."""


class ProgramFailure(Exception):
    """Failure, not a bug, which is reported neatly to the user."""


def parse_args():
    """Parse command line arguments, return as `Namespace`."""
    parser = ArgumentParser(description=__doc__)
    parser.add_argument(
        '--extract-file', '-e', metavar='PATH', required=True,
        help=(
            "Path to input file: extract file (e.g. 'extract.sorted.gz' or "
            "'extract.inv.sorted.gz').  Required."))
    parser.add_argument(
        '--lex-file', '-l', metavar='PATH', required=True,
        help=(
            "Path to input file: lex file (e.g. 'lex.f2e' or 'lex.e2f').  "
            "Required."))
    parser.add_argument(
        '--output', '-o', metavar='PATH', required=True,
        help=(
            "Write phrase table to file PATH (e.g. 'phrase-table.half.f2e' "
            "or 'phrase-table.half.e2f').  Required."))
    parser.add_argument(
        '--inverse', '-i', action='store_true',
        help="Inverse scoring.  Defaults to direct scoring.")
    parser.add_argument(
        '--labels-file', '-L', metavar='PATH',
        help="Also write source labels to file PATH.")
    parser.add_argument(
        '--parts-of-speech', '-p', metavar='PATH',
        help="Also write parts-of-speech file to PATH.")
    parser.add_argument(
        '--flexibility-score', '-F', metavar='PATH',
        help="Path to the 'flexibility_score.py' script.  Defaults to none.")
    parser.add_argument(
        '--hierarchical', '-H', action='store_true',
        help="Process hierarchical rules.")
    parser.add_argument(
        '--args', '-a', metavar='ARGUMENTS',
        help="Additional arguments for `score` and `flexibility_score`.")
    parser.add_argument(
        '--sort', '-s', action='store_true',
        help="Sort output file.")
    parser.add_argument(
        '--jobs', '-j', metavar='N', type=int, default=1,
        help="Run up to N jobs in parallel.  Defaults to %(default)s.")
    parser.add_argument(
        '--score-exe', '-x', metavar='PROGRAM',
        help="Name of, or path to, the 'score' executable.")
    parser.add_argument(
        '--sort-command', '-S', metavar='COMMAND-LINE',
        help=(
            "Command line for sorting text files to standard output.  "
            "Must support operation as a pipe, as well as input files named "
            "as command-line arguments."))
    parser.add_argument(
        '--gzip-command', '-z', metavar='PROGRAM',
        help="Path to a gzip or pigz executable.")
    parser.add_argument(
        '--verbose', '-v', action='store_true',
        help="Print what's going on.")
    parser.add_argument(
        '--debug', '-d', action='store_true',
        help="Don't delete temporary directories when done.")
    return parser.parse_args()


def normalize_path(optional_path=None):
    """Return a cleaned-up version of a given filesystem path, or None.

    Converts the path to the operating system's native conventions, and
    removes redundancies like `.`.

    The return value will be `None`, an absolute path, or a relative path,
    same as the argument.  But it will have redundant path separators,
    unnecessary detours through parent directories, and use of the current
    directory "." removed.
    """
    if optional_path is None:
        return None
    else:
        path = os.path.normpath(optional_path)
        path = path.replace('/', os.path.sep)
        path = path.replace('\\', os.path.sep)
        return path


def quote(path):
    """Quote and escape a filename for use in a shell command.

    The Windows implementation is very limited and will break on anything
    more advanced than a space.
    """
    if os.name == 'posix':
        return pipes.quote(path)
    else:
        # TODO: Improve escaping for Windows.
        return '"%s"' % path


def sanitize_args(args):
    """Check `args` for sanity, clean up, and set nontrivial defaults."""
    if args.jobs < 1:
        raise CommandLineError("Number of parallel jobs must be 1 or more.")
    if args.sort_command is None:
        args.sort_command = find_first_executable(
            ['neandersort', 'gsort', 'sort'])
    if args.sort_command is None:
        raise CommandLineError(
            "No 'sort' command is available.  "
            "Choose one using the --sort-command option.")
    if args.gzip_command is None:
        args.gzip_command = find_first_executable(['pigz', 'gzip'])
    if args.gzip_command is None:
        raise CommandLineError(
            "No 'gzip' or 'pigz' command is available.  "
            "Choose one using the --gzip-command option.")
    if args.score_exe is None:
        # Look for "score" executable.  It may be in the current project
        # directory somewhere, or in the PATH.
        moses_dir = os.path.dirname(os.path.dirname(
            os.path.abspath(__file__)))
        args.score_exe = find_first_executable(
            ['score'],
            [
                moses_dir,
                os.path.join(moses_dir, 'phrase-extract'),
                os.path.join(moses_dir, 'binaries'),
            ])
    args.extract_file = normalize_path(args.extract_file)
    args.lex_file = normalize_path(args.lex_file)
    args.output = normalize_path(args.output)
    args.labels_file = normalize_path(args.labels_file)
    args.parts_of_speech = normalize_path(args.parts_of_speech)
    args.flexibility_score = normalize_path(args.flexibility_score)
    args.score_exe = normalize_path(args.score_exe)


def add_exe_suffix(program):
    """Return the full filename for an executable.

    On Windows, this adds a `.exe` suffix to the name.  On other
    systems, it returns the original name unchanged.
    """
    if os.name == 'nt':
        # Windows.
        return program + '.exe'
    else:
        # Assume POSIX or similar.
        return program


def find_executable(exe, extra_path=None):
    """Return full path to an executable of the given name, or `None`.

    If the given name is a qualified path to an executable, it will be returned
    unchanged.  A qualified path where no executable is found results in a
    `CommandLineError`.
    """
    if extra_path is None:
        extra_path = []

    if os.path.sep in exe:
        # The executable name includes a path.  Only one place it can be.
        if not os.path.isfile(exe) or not os.access(exe, os.X_OK):
            raise CommandLineError("Not an executable: '%s'." % exe)
        return exe

    for path in extra_path + os.getenv('PATH').split(os.pathsep):
        full_path = os.path.join(path, exe)
        if os.access(full_path, os.X_OK):
            return full_path
    return None


def find_first_executable(candidates, extra_path=None):
    """Find the first available of the given candidate programs.

    :raise ProgramFailure: If none of `candidates` was found.
    """
    for program in candidates:
        executable = find_executable(add_exe_suffix(program), extra_path)
        if executable is not None:
            return executable
    raise ProgramFailure(
        "Could not find any of these executables in path: %s."
        % ', '.join(candidates))


def execute_shell(command, verbose=False):
    """Run `command` string through the shell.

    Inherits environment, but sets `LC_ALL` to `C` for predictable results,
    especially from sort commands.

    This uses a full-featured shell, including pipes, substitution, etc.  So
    remember to quote/escape arguments where appropriate!
    """
    assert isinstance(command, UNICODE_TYPE), (
        "Wrong argument for execute_shell.")
    if verbose:
        print("Executing: %s" % command)
    env = os.environ.copy()
    if os.name == 'posix':
        env['LC_ALL'] = 'C'
    check_call(command, shell=True, env=env)


@contextmanager
def tempdir(keep=False):
    """Context manager: temporary directory."""
    directory = tempfile.mkdtemp()
    yield directory
    if not keep:
        rmtree(directory)


def make_dirs(path):
    """Equivalent to `mkdir -p -- path`."""
    try:
        os.makedirs(path)
    except OSError as error:
        if error.errno != errno.EEXIST:
            raise


def open_file(path, mode='r'):
    """Open a file, which may be gzip-compressed."""
    if path.endswith('.gz'):
        return gzip.open(path, mode)
    else:
        return open(path, mode)


def count_lines(filename):
    """Count the number of lines in `filename` (may be gzip-compressed)."""
    count = 0
    with open_file(filename) as stream:
        for _ in stream:
            count += 1
    return count


def set_temp_dir():
    """Set temporary directory to `$MOSES_TEMP_DIR`, if set.

    Create the directory if necessary.
    """
    temp_dir = os.getenv('MOSES_TEMP_DIR')
    if temp_dir is not None:
        make_dirs(temp_dir)
        tempfile.tempdir = temp_dir


def strip_newline(line):
    """Remove trailing carriage return and/or line feed, if present."""
    if line.endswith('\n'):
        line = line[:-1]
    if line.endswith('\r'):
        line = line[:-1]
    return line


def open_chunk_file(split_dir, chunk_number):
    """Open a file to write one chunk of the extract file."""
    return open_file(
        os.path.join(split_dir, 'extract.%d.gz' % chunk_number), 'w')


def name_context_chunk_file(split_dir, chunk_number):
    """Compose file name for one chunk of the extract context file."""
    return os.path.join(
        split_dir, 'extract.context.%d.gz' % chunk_number)


def extract_source_phrase(line):
    """Extract the source phrase from an extract-file line."""
    return line.split(b'|||', 1)[0]


def cut_context_file(last_source_phrase, chunk_file, last_line,
                     context_stream):
    """Write one chunk of extract context file into its own file.

    :param last_source_phrase: Last source phrase that should be in the
        chunk.  Stop processing after this source phrase.
    :param chunk_file: Path to the extract context file for this chunk.
    :param last_line: Previously read line that may still need writing.
    :param context_stream: Extract context file, opened for reading.
    :return: Last line read from `context_stream`.  This line will still
        need processing.
    """
    # TODO: Use open_file.
    with gzip.open(chunk_file, 'w') as chunk:
        if last_line is not None:
            chunk.write('%s\n' % last_line)

        # Are we processing our last source phrase yet?
        on_last_source_phrase = False

        # Write all lines in context file until we meet last source phrase
        # in extract file.
        for line in context_stream:
            # Reading from a gzip file returns lines *including the newline*.
            # Either way, we want to ignore carriage returns as well.
            line = strip_newline(line)
            source_phrase = extract_source_phrase(line)
            if on_last_source_phrase and source_phrase != last_source_phrase:
                # First new source phrase after our last one.  We're done.
                return line
            else:
                # Still adding lines to our chunk.
                chunk.write('%s\n' % line)
                if source_phrase == last_source_phrase:
                    # We're on our last source phrase now.
                    on_last_source_phrase = True


def split_extract_files(split_dir, extract_file, extract_context_file=None,
                        jobs=1):
    """Split extract file into chunks, so we can process them in parallel.

    :param split_dir: A temporary directory where this function can write
        temporary files.  The caller must ensure that this directory will be
        cleaned up after it's done with the files.
    :return: An iterable of tuples.  Each tuple hols a partial extract file,
        and the corresponding context file.  The files may be in `split_dir`,
        or there may just be the original extract file.
    """
    if jobs == 1:
        # No splitting needed.  Read the original file(s).
        return [(extract_file, extract_context_file)]

    # Otherwise: split files.
    files = []
    num_lines = count_lines(extract_file)
    chunk_size = (num_lines + jobs - 1) / jobs
    assert isinstance(chunk_size, int)

    line_count = 0
    chunk_number = 0
    prev_source_phrase = None
    last_line_context = None
    extract_stream = open_file(extract_file)
    chunk_file = open_chunk_file(split_dir, chunk_number)
    if extract_context_file is None:
        chunk_context_file = None
    if extract_context_file is not None:
        context_stream = open_file(extract_context_file)

    for line in extract_stream:
        line_count += 1
        line = line.decode('utf-8')
        line = strip_newline(line)
        if line_count >= chunk_size:
            # At or over chunk size.  Cut off at next source phrase change.
            source_phrase = extract_source_phrase(line)
            if prev_source_phrase is None:
                # Start looking for a different source phrase.
                prev_source_phrase = source_phrase
            elif source_phrase == prev_source_phrase:
                # Can't cut yet.  Still working on the same source phrase.
                pass
            else:
                # Hit first new source phrase after chunk limit.  Cut new
                # file(s).
                chunk_file.close()
                if extract_context_file is not None:
                    chunk_context_file = name_context_chunk_file(
                        split_dir, chunk_number)
                    last_line_context = cut_context_file(
                        prev_source_phrase, chunk_context_file,
                        last_line_context, context_stream)
                files.append((chunk_file.name, chunk_context_file))

                # Start on new chunk.
                prev_source_phrase = None
                line_count = 0
                chunk_number += 1
                chunk_file = open_chunk_file(split_dir, chunk_number)
        chunk_file.write(('%s\n' % line).encode('utf-8'))

    chunk_file.close()
    if extract_context_file is not None:
        chunk_context_file = name_context_chunk_file(split_dir, chunk_number)
        last_line_context = cut_context_file(
            prev_source_phrase, chunk_number, last_line_context,
            context_stream)
    files.append((chunk_file.name, chunk_context_file))
    return files


def compose_score_command(extract_file, context_file, half_file,
                          flex_half_file, args):
    """Compose command line text to run one instance of `score`.

    :param extract_file: One chunk of extract file.
    :param context_file: If doing flexibility scoring, one chunk of
        extract context file.  Otherwise, None.
    :param half_file: ???
    :param flex_half_file: ???
    :param args: Arguments namespace.
    """
    command = [
        args.score_exe,
        extract_file,
        args.lex_file,
        half_file,
        ]
    if args.args not in (None, ''):
        command.append(args.args)
    other_args = build_score_args(args)
    if other_args != '':
        command.append(other_args)
    if context_file is not None:
        command += [
            '&&',
            find_first_executable(['bzcat']),
            half_file,
            '|',
            quote(args.flexibility_score),
            quote(context_file),
            ]
        if args.inverse:
            command.append('--Inverse')
        if args.hierarchical:
            command.append('--Hierarchical')
        command += [
            '|',
            quote(args.gzip_command),
            '-c',
            '>%s' % quote(flex_half_file),
            ]
    return ' '.join(command)


def score_parallel(split_dir, file_pairs, args):
    """Run the `score` command in parallel.

    :param split_dir: Temporary directory where we can create split files.
    :param file_pairs: Sequence of tuples for the input files, one tuple
        per chunk of the work.  Each tuple consists of a partial extract
        file, and optionally a partial extract context file.
    :param args: Arguments namespace.
    :return: A list of tuples.  Each tuple contains two file paths.  The first
        is for a partial half-phrase-table file.  The second is for the
        corresponding partial flex file, if a context file is given; or
        `None` otherwise.
    """
    partial_files = []
    # Pool of worker processes for executing the partial "score" invocations
    # concurrently.
    pool = Pool(args.jobs)
    try:
        for chunk_num, file_pair in enumerate(file_pairs):
            half_file = os.path.join(
                split_dir, 'phrase-table.half.%06d.gz' % chunk_num)
            extract_file, context_file = file_pair
            if context_file is None:
                flex_half_file = None
            else:
                flex_half_file = os.path.join(
                    split_dir, 'phrase-table.half.%06d.flex.gz' % chunk_num)
            # Pickling of arguments for the pool is awkward on Windows, so
            # keep them simple.  Compose the command line in the parent
            # process, then hand them to worker processes which execute them.
            command_line = compose_score_command(
                extract_file, context_file, half_file, flex_half_file, args)
            pool.apply_async(
                execute_shell, (command_line, ), {'verbose': args.verbose})
            partial_files.append((half_file, flex_half_file))
        pool.close()
    except BaseException:
        pool.terminate()
        raise
    finally:
        pool.join()
    return partial_files


def merge_and_sort(files, output, sort_command=None, gzip_exe=None,
                   verbose=False):
    """Merge partial files.

    :param files: List of partial half-phrase-table files.
    :param output: Path for resulting combined phrase-table file.
    """
# TODO: The Perl code mentioned "sort" and "flexibility_score" here.
# What do we do with those?

    # Sort whether we're asked to or not, as a way of combining the input
    # files.
    if sort_command == 'neandersort':
        # Neandersort transparently decompresses input and compresses output.
        check_call([
            'neandersort',
            '-o', output,
            ] + files)
    else:
        command = (
            "%(gzip)s -c -d %(files)s | "
            "%(sort)s | "
            "%(gzip)s -c >>%(output)s"
            % {
                'gzip': quote(gzip_exe),
                'sort': sort_command,
                'files': ' '.join(map(quote, files)),
                'output': quote(output),
            })
        execute_shell(command, verbose=verbose)


def build_score_args(args):
    """Compose command line for the `score` program."""
    command_line = []
    if args.labels_file:
        command_line += [
            '--SourceLabels',
            '--SourceLabelCountsLHS',
            '--SourceLabelSet',
            ]
    if args.parts_of_speech:
        command_line.append('--PartsOfSpeech')
    if args.inverse:
        command_line.append('--Inverse')
    if args.args is not None:
        command_line.append(args.args)
    return ' '.join(command_line)


def list_existing(paths):
    """Return, in the same order, those of the given files which exist."""
    return filter(os.path.exists, paths)


def compose_coc_path_for(path):
    """Compose COC-file path for the given file."""
    return '%s.coc' % path


def read_cocs(path):
    """Read COC file at `path`, return contents as tuple of ints."""
    with open(path) as lines:
        return tuple(
            int(line.rstrip('\r\n'))
            for line in lines
            )


def add_cocs(original, additional):
    """Add two tuples of COCs.  Extend as needed."""
    assert not (original is None and additional is None), "No COCs to add!"
    if original is None:
        return additional
    elif additional is None:
        return original
    else:
        common = tuple(lhs + rhs for lhs, rhs in zip(original, additional))
        return (
            common +
            tuple(original[len(common):]) +
            tuple(additional[len(common):]))


def merge_coc(files, output):
    """Merge COC files for the given partial files.

    Each COC file is a series of integers, one per line.  This reads them, and
    adds them up line-wise into one file of the same format: the sum of the
    numbers the respective files have at line 1, the sum of the numbers the
    respective files have at line 2, and so on.
    """
    assert len(files) > 0, "No partial files - no work to do."
    extract_files = [extract_file for extract_file, _ in files]
    if not os.path.exists(compose_coc_path_for(extract_files[0])):
        # Nothing to merge.
        return
    totals = None
# TODO: Shouldn't we just fail if any of these files is missing?
    for coc_path in list_existing(map(compose_coc_path_for, extract_files)):
        totals = add_cocs(totals, read_cocs(coc_path))

    # Write to output file.
    with open(output, 'w') as output_stream:
        for entry in totals:
            output_stream.write('%d\n' % entry)


def suffix_line_numbers(infile, outfile):
    """Rewrite `infile` to `outfile`; suffix line number to each line.

    The line number is zero-based, and separated from the rest of the line
    by a single space.
    """
    temp_file = '%s.numbering' % outfile
    with open(infile, 'r') as instream, open(outfile, 'w') as outstream:
        line_no = 0
        for line in instream:
            outstream.write(line)
            outstream.write(' %d\n' % line_no)
            line_no += 1
    os.rename(temp_file, outfile)


def compose_source_labels_path_for(path):
    """Return source labels file path for given file."""
    return '%s.syntaxLabels.src' % path


def merge_numbered_files(inputs, output, header_lines, sort_command,
                         verbose=False):
    """Sort and merge files `inputs`, add header and line numbers.

    :param inputs: Iterable of input files.
    :param output: Output file.
    :header_lines: Iterable of header lines.
    :sort_command: Command line for sorting input files.
    """
    sort_temp = '%s.sorting' % output
    with open(sort_temp, 'w') as stream:
        for line in header_lines:
            stream.write(line)
            stream.write('\n')
    execute_shell(
        "%s %s >>%s" % (
            sort_command,
            ' '.join(map(quote, inputs)),
            quote(sort_temp)),
        verbose=verbose)
    suffix_line_numbers(sort_temp, output)


def merge_source_labels(files, output, sort_command, verbose=False):
    """Merge source labels files."""
# TODO: Shouldn't we just fail if any of these files is missing?
    labels_files = list_existing(map(compose_source_labels_path_for, files))
    header = [
        'GlueTop',
        'GlueX',
        'SSTART',
        'SEND',
        ]
    merge_numbered_files(
        labels_files, output, header, sort_command, verbose=verbose)


def compose_parts_of_speech_path_for(path):
    """Return parts-of-speech file path for given file."""
    return '%s.partsOfSpeech' % path


def merge_parts_of_speech(files, output, sort_command, verbose=False):
    """Merge parts-of-speech files into output."""
# TODO: Shouldn't we just fail if any of these files is missing?
    parts_files = list_existing(map(compose_parts_of_speech_path_for, files))
    header = [
        'SSTART',
        'SEND',
        ]
    merge_numbered_files(
        parts_files, output, header, sort_command, verbose=verbose)


def main():
    """Command-line entry point.  Marshals and forwards to `score_parallel`."""
    args = parse_args()
    sanitize_args(args)
    set_temp_dir()

    if args.flexibility_score is None:
        extract_context_file = None
    else:
        extract_context_file = args.extract_file.replace(
            'extract.', 'extract.context.')

    if args.verbose:
        print("Started %s." % datetime.now())
        print("Using '%s' for gzip." % args.gzip_command)

    with tempdir(args.debug) as split_dir:
        extract_files = split_extract_files(
            split_dir, args.extract_file,
            extract_context_file=extract_context_file, jobs=args.jobs)

        scored_files = score_parallel(split_dir, extract_files, args)

        if args.verbose:
            sys.stderr.write("Finished score %s.\n" % datetime.now())

# TODO: Pass on "sort" and "flexibility-score" arguments?
        merge_and_sort(
            [phrase_chunk for phrase_chunk, _ in scored_files], args.output,
            sort_command=args.sort_command, gzip_exe=args.gzip_command,
            verbose=args.verbose)
        merge_coc(extract_files, compose_coc_path_for(args.output))

        if not args.inverse and args.labels_file is not None:
            if args.verbose:
                print("Merging source labels files.")
            merge_source_labels(
                extract_files, args.labels_file,
                sort_command=args.sort_command, verbose=args.verbose)

        if not args.inverse and args.parts_of_speech is not None:
            if args.verbose:
                print("Merging parts-of-speech files.")
            merge_parts_of_speech(
                extract_files, args.parts_of_speech,
                sort_command=args.sort_command, verbose=args.verbose)


if __name__ == '__main__':
    try:
        main()
    except ProgramFailure as error:
        sys.stderr.write('%s\n' % error)
        sys.exit(1)
    except CommandLineError as error:
        sys.stderr.write("Command line error: %s\n" % error)
        sys.exit(2)
