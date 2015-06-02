#! /usr/bin/env python
# -*- coding: utf8 -*-
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 3 or, at your option, any later version.


from __future__ import (
    absolute_import,
    print_function,
    unicode_literals,
    )

__version__ = '1.0'
__license__ = 'LGPL3'
__source__ = 'Precision Translation Tools Pte Lte'

import errno
from sys import stdout
from copy import deepcopy
from os.path import (
    dirname,
    basename,
    exists,
    realpath,
    )
from os import makedirs


root_escape = '%(escape-prefix)s'


class moses2_to_ini(object):

    def __init__(self, inp, out, escape_prefix):
        self.inp = inp
        self.out = out
        self.escape_prefix = escape_prefix
        self._config = {}

    def parse(self):
        key = ''
        section = None
        self._config = {}
        counter = 0

        with open(self.inp, 'rb') as f:
            contents = f.read().decode('utf8')

        lines = contents.splitlines()

        # Known feature/functions without attributes.
        attrless_ffs = [
            'UnknownWordPenalty',
            'WordPenalty',
            'PhrasePenalty',
            'Distortion',
            ]

        # Retrieve all values except feature/functions with attributes.
        for i, line in [(i, line.strip()) for i, line in enumerate(lines)
                        if line.strip() and not line.strip().startswith('#')]:

            if line.startswith('[') and line.endswith(']'):

                section = line.strip('] [')

                if section not in self._config.keys() + ['feature', 'weight']:
                    # New section not in config and not a reserved section.
                    counter = 0
                    key = section
                    self._config[key] = {}

            elif section == 'feature' and line in attrless_ffs:
                # Known feature/funcions without attributes.
                key = '%s0' % line
                if key not in self._config:
                    self._config[key] = {}
                self._config[key]['feature'] = line

            elif section == 'feature':
                # Skip feature/funcions with arguments.
                continue

            elif section == 'weight':
                # Add weight value to feature sections.
                config_items = [
                    (key.strip(), value.strip())
                    for key, value in [line.split('=', 1)]
                    ]
                for key, value in config_items:
                    if key not in self._config:
                        self._config[key] = {}
                    self._config[key]['weight'] = value

            else:
                self._config[key][counter] = line
                counter += 0

            lines[i] = ''

        # Second, match feature/functions attributes to [weight] section
        # values.
        stripped_lines = [line.strip() for line in lines]
        nonempty_lines = [
            line
            for line in stripped_lines
            if line != '' and not line.startswith('#')
            ]
        for i, line in enumerate(nonempty_lines):
            # Add "feature" to assist creating tmpdict for feature/functions.
            line = 'feature=%s' % line
            tmpdict = dict([key.split('=', 1) for key in line.split()])

            # Feature/functions 'name' attribute must match an entry in
            # [weight].
            if tmpdict.get('name') not in self._config:
                raise RuntimeError('malformed moses.ini v2 file')

            config_items = [
                (key.strip(), value.strip())
                for key, value in tmpdict.items()
                if key.strip() != 'name'
                ]
            for key, value in config_items:
                self._config[tmpdict['name']][key] = value

        return deepcopy(self._config)

    def render(self, config):
        self._config = deepcopy(config)
        _config = deepcopy(config)
        lines = _tolines(_config, self.escape_prefix)
        if self.out == '-':
            stdout.write('\n'.join(lines))
        else:
            contents = '\r\n'.join(lines)
            makedir(dirname(self.out))
            with open(self.out, 'wb') as f:
                f.write(contents.encode('utf8'))

    def __str__(self):
        return '\n'.join(_tolines(self._config, self.escape_prefix))

    @property
    def config(self):
        return deepcopy(self._config)


def _tolines(config, escape_prefix):

    section_names = sorted(config)
    lines = []

    # Group feature/functions first.
    group_ffs = [
        name
        for name in section_names
        if name[-1].isdigit()
    ]
    for sectionname in group_ffs:
        section = config[sectionname]
        lines.append('[%s]' % sectionname)
        for option, value in section.items():
            if option == 'path' \
                    and escape_prefix is not None \
                    and value.startswith(escape_prefix):
                value = value.replace(escape_prefix, root_escape, 1)
            lines.append('%s=%s' % (option, value))
        lines.append('')

    other_ffs = [
        name
        for name in section_names
        if not name[-1].isdigit()
    ]
    for sectionname in other_ffs:
        section = config[sectionname]
        lines.append('[%s]' % sectionname)
        for option, value in section.items():
            lines.append('%s=%s' % (option, value))
        lines.append('')

    return deepcopy(lines)


def makedir(path, mode=0o777):
    try:
        makedirs(path, mode)
    except OSError as e:
        accepted_errors = [
            errno.EEXIST,
            errno.EPERM,
            errno.EACCES,
            errno.ENOENT,
            ]
        if e.errno not in accepted_errors:
            raise


def get_args():
    '''Parse command-line arguments

    Uses the API compatibility between the legacy
    argparse.OptionParser and its replacement argparse.ArgumentParser
    for functional equivelancy and nearly identical help prompt.
    '''

    description = 'Convert Moses.ini v2 file to standard INI format'
    usage = '%s [arguments]' % basename(__file__)

    try:
        from argparse import ArgumentParser
    except ImportError:
        from optparse import OptionParser
        argparser = False
        escape_help = (
            "Optional. Path of SMT model. If provided, "
            "escapes \"escape-prefix\" with \"%(escape-prefix)s\"")
        parser = OptionParser(usage=usage, description=description)
        add_argument = parser.add_option
    else:
        argparser = True
        escape_help = (
            "Optional. Path of SMT model. If provided, "
            "escape \"escape-prefix\" with \"%%(escape-prefix)s\"")
        parser = ArgumentParser(usage=usage, description=description)
        add_argument = parser.add_argument

    add_argument(
        '-i', '--inp', action='store',
        help="moses.ini v2 file to convert (required)")

    add_argument(
        '-o', '--out', action='store', default='-',
        help="standard INI file (default: '-' outputs to stdout)")

    add_argument('-r', '--escape-prefix', action='store', help=escape_help)

    if argparser:
        args = vars(parser.parse_args())
    else:
        opts = parser.parse_args()
        args = vars(opts[0])

    if args['inp'] is None:
        parser.error('argument -i/--inp required')

    args['inp'] = realpath(args['inp'])

    if not exists(args['inp']):
        parser.error(
            "argument -i/--inp invalid.\n"
            "reference: %s" % args['inp'])

    if args['out'] != '-':
        args['out'] = realpath(args['out'])

    return args


if __name__ == '__main__':
    args = get_args()
    converter = moses2_to_ini(**args)
    config = converter.parse()
    converter.render(config)
