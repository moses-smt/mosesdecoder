#! /usr/bin/env python
# -*- coding: utf8 -*-


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
from os import (
	sep,
	makedirs,
	)

root_escape = '%(escape-prefix)s'


class moses2_to_ini(object):


	def __init__(self, inp, out, escape_prefix):
		self.inp = inp
		self.out = out
		self.escape_prefix = escape_prefix
		self._config = {}


	def parse(self):

		content = ''
		key = ''
		section = None
		self._config = {}
		counter = 0

		with open(self.inp, 'rb' ) as f:
			contents = f.read().decode('utf8')

		lines = contents.splitlines()

		# retrieve all values except feature/functions with attributes
		for i, line in [(i, line.strip()) for i, line in enumerate(lines)
						if line.strip() and not line.strip().startswith('#')]:

			if line.startswith('[') and line.endswith(']'):

				section = line.strip('] [')

				if section not in self._config.keys() + ['feature', 'weight']:
					# new section not in config and not a reserved section
					counter = 0
					key = section
					self._config[key] = {}

			elif section == 'feature' and line in ['UnknownWordPenalty',
								'WordPenalty', 'PhrasePenalty', 'Distortion']:
				# known feature/funcions without attributes
				key = '%s0' % line
				if key not in self._config:
					self._config[key] = {}
				self._config[key]['feature'] = line

			elif section == 'feature':
				# skip feature/funcions with artuments
				continue

			elif section == 'weight':
				# add weight value to feature sections
				for key, value in [(key.strip(), value.strip())
									for key, value in [line.split('=', 1)]]:
					if key not in self._config:
						self._config[key] = {}
					self._config[key]['weight'] = value

			else:
				self._config[key][counter] = line
				counter += 0

			lines[i] = ''

		# second, match feature/functions attributes to [weight] section values
		for i, line in [(i, line.strip()) for i, line in enumerate(lines)
						if line.strip() and not line.strip().startswith('#')]: 

			# add "feature" to assist creating tmpdict for feature/functions
			line = 'feature=%s' % line
			tmpdict = dict([key.split('=',1) for key in line.split()])

			# feature/functions 'name' attribute must match an entry in [weight]
			if tmpdict.get('name') not in self._config:
				raise RuntimeError('malformed moses.ini v2 file')

			for key, value in [(key.strip(), value.strip()) for key, value 
								in tmpdict.items() if key.strip() != 'name']:

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

	lines = []

	# group feature/functions first
	for sectionname in [sectionname for sectionname in sorted(config)
									if sectionname[-1] in '0123456789']:

		section = config[sectionname]

		lines.append('[%s]' % sectionname)

		for option, value in section.items():

			if option == 'path' \
					and escape_prefix is not None \
					and value.startswith(escape_prefix):

				value = value.replace(escape_prefix, root_escape, 1)

			lines.append('%s=%s' % (option, value))

		lines.append('')

	for sectionname in [sectionname for sectionname in sorted(config)
									if sectionname[-1] not in '0123456789']:

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
		if e.errno not in [errno.EEXIST,
							errno.EPERM, errno.EACCES, errno.ENOENT]:
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
		escape_help = ('Optional. Path of SMT model. If provided, '
							'escapes \"escape-prefix\" with \"%(escape-prefix)s\"')
		parser = OptionParser(usage=usage, description=description)
		add_argument = parser.add_option
	else:
		argparser = True
		escape_help = ('Optional. Path of SMT model. If provided, '
							'escape \"escape-prefix\" with \"%%(escape-prefix)s\"')
		parser = ArgumentParser(usage=usage, description=description)
		add_argument = parser.add_argument

	add_argument('-i','--inp', action='store',
			help='moses.ini v2 file to convert (required)')

	add_argument('-o','--out', action='store', default='-',
			help='standard INI file (default: "-" outputs to stdout)')

	add_argument('-r','--escape-prefix', action='store',
			help=escape_help)

	if argparser:

		args = vars(parser.parse_args())

	else:

		opts = parser.parse_args()
		args = vars(opts[0])

	if args['inp'] is None:
		parser.error('argument -i/--inp required')

	args['inp'] = realpath(args['inp'])

	if not exists(args['inp']):
		parser.error('argument -i/--inp invalid.\n'
										'reference: %s' % args['inp'])

	if args['out'] != '-':
		args['out'] = realpath(args['out'])

	return args


if __name__ == '__main__':

	args = get_args()

	converter = moses2_to_ini(**args)

	config = converter.parse()

	converter.render(config)
