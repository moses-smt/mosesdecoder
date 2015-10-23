#! /usr/bin/env python
# -*- coding: utf8 -*-

#===============================================================================
# Author: Walapa Muangjeen
#===============================================================================


__version__ = '2.0'

import sys
import os
import codecs
import ConfigParser
from optparse import OptionParser
from copy import deepcopy


class makemteval:

	def __init__(self, config=None):

		if isinstance(config,dict):
			self.config = deepcopy(config)
		else:
			self.config = {
				'filein': None,
				'fileout': None,
				'settype': None,
				'srclang': None,
				'tstlang': None,
				'setid': 'SetID',
				'refid': 'RefID',
				'sysid': 'SysID',
				'docid': 'DocID',
				'genre': 'Genre',
				}
 
 
	def parseini(self, config=None, inifile=None, section='set'):

		if inifile is None:
			inifile = os.path.abspath(os.path.dirname(sys.argv[0])) + os.sep + os.path.splitext(os.path.basename(sys.argv[0]))[0] + '.ini'

		if config is None:
			config = self.config

		cfgparser = ConfigParser.RawConfigParser()

		if not cfgparser.has_section(section):
			cfgparser.add_section(section)

		for option in config:
			cfgparser.set(section, option, config[option])

		cfgparser.read(inifile)

		for option in cfgparser.options(section):
			config[option] = cfgparser.get(section, option)

		return deepcopy(config)


	def writesgm( self, config ):

		try:
			filein = codecs.open(os.path.abspath(os.path.expanduser(config['filein'])), "r", 'utf-8-sig')
		except IOError, ErrorMessage:
			sys.stderr.write("\n: %s\n"%(ErrorMessage))
			sys.stderr.write(": End Program\n")
			return True

		if __name__ == "__main__":
			sys.stderr.write( ": opened \"%s\" for reading\n"%(os.path.basename( config['filein'] )))

		lines = [l.replace('&quot;','\"').replace('&apos;','\'').replace('&gt;','>').replace('&lt;','<').replace('&amp;','&') for l in filein.read().splitlines()]
		filein.close()
		lines = [l.replace('&','&amp;').replace('<','&lt;').replace('>','&gt;').replace('\'','&apos;').replace('\"','&quot;') for l in lines]

		if __name__ == "__main__":
			sys.stderr.write(": closed \"%s\"\n"%(os.path.basename( config['filein'] )))

		try:
			fileout = codecs.open(os.path.abspath(os.path.expanduser(config['fileout'])), "w", 'utf8')
		except IOError, ErrorMessage:
			sys.stderr.write("\n: %s\n"%(ErrorMessage))
			sys.stderr.write(": End Program\n")
			return True

		if __name__ == "__main__":
			sys.stderr.write(": opened \"%s\" for writing\n"%(os.path.basename( config['fileout'] )))

		contents = []
		contents.append('<?xml version=\"1.0\" encoding=\"UTF-8\"?>')
		contents.append('<!DOCTYPE mteval SYSTEM \"ftp://jaguar.ncsl.nist.gov/mt/resources/mteval-xml-v1.3.dtd\">')
		contents.append('<mteval>')

		if config['settype'] == "srcset":
			contents.append("<%(settype)s setid=\"%(setid)s\" srclang=\"%(srclang)s\">"%(config))

		elif config['settype'] == "refset":
			contents.append('<%(settype)s setid=\"%(setid)s\" srclang=\"%(srclang)s\" trglang=\"%(tstlang)s\" refid=\"%(refid)s\">'%(config))

		elif config['settype'] == "tstset":
			contents.append('<%(settype)s setid=\"%(setid)s\" srclang=\"%(srclang)s\" trglang=\"%(tstlang)s\" sysid=\"%(sysid)s\" sysbleu=\"%(sysbleu)s\" language=\"%(language)s\">'%(config))

		else:
			fileout.close()
			os.unlink(os.path.abspath(os.path.expanduser(config['fileout'])))
			sys.stderr.write("\n: Invalid \"settype\" value %s\n"%(config['settype']))
			sys.stderr.write(": End Program\n")
			return True

		contents.append('<DOC %sdocid=\"%s\" genre=\"%s\">'%('' if config['settype'] == "srcset" else 'sysid=\"%s\" '%(config['sysid']),config['docid'],config['genre']))

		if __name__ == "__main__":
			sys.stderr.write(": added header\n")

		for i in range(len(lines)):
			contents.append('<seg id=\"%d\"> %s </seg>'%(i+1,lines[i]))

		if __name__ == "__main__":
			sys.stderr.write(": added %d lines\n"%(i+1))

		contents.append('</DOC>')
		contents.append('</%s>'%(config['settype']))
		contents.append('</mteval>')

		if __name__ == "__main__":
			sys.stderr.write(": added footer\n")

		fileout.write('%s\n'%('\n'.join(contents)))
		ferror = fileout.close()

		if __name__ == "__main__":
			sys.stderr.write(": closed \"" +  os.path.basename( config['fileout'] ) + "\"\n")

		return ferror


def parsecmd( config = {} ):

	optparser = OptionParser()

	optparser.add_option(
		"-i", "--filein", dest = "filein", default = config["filein"],
		help = "UNC path to tokenized input file (required)")

	optparser.add_option(
		"-o", "--fileout", dest = "fileout", default = config["fileout"],
		help = "UNC path of fileout file (required)")

	optparser.add_option(
		"-s", "--srclang", dest = "srclang", default = config["srclang"],
		help = "2-letter code for source language (required)")

	optparser.add_option(
		"-t", "--tstlang", dest = "tstlang", default = config["tstlang"],
		help = "2-letter code for test language (required)")

	optparser.add_option(
		"-T", "--settype", dest = "settype", default = config["settype"],
		help = "Use XML tag: srcset, tstset or refset (required)")

	optparser.add_option(
		"-e", "--setid", dest = "setid", default = config["setid"],
		help = "Test set ID (default \""+config["setid"]+"\")")

	optparser.add_option(
		"-d", "--docid", dest = "docid", default = config["docid"],
		help = "Document ID (default \""+config["docid"]+"\")")

	optparser.add_option(
		"-r", "--refid", dest = "refid", default = config["refid"],
		help = "Reference ID (default \""+config["refid"]+"\")")

	optparser.add_option(
		"-S", "--sysid", dest = "sysid", default = config["sysid"],
		help = "System ID used to make the test set (default \""+config["sysid"]+"\")")

	optparser.add_option(
		"-g", "--genre", dest = "genre", default = config["genre"],
		help = "Genre of the test set and system ID (default \""+config["genre"]+"\")")

	options, commands = optparser.parse_args()

	missing = []
	for k,v in {	"Error: missing --filein" : options.filein, 
			"Error: missing --fileout": options.fileout, 
			"Error: missing --settype": options.settype, 
			"Error: missing --srclang": options.srclang, 
			"Error: missing --tstlang": options.tstlang
								}.items():
		if not v:
			missing.append(k)

	if missing:
		for msg in missing:
			sys.stderr.write('%s\n'%(msg))
		optparser.print_help()
		exit(1)

	config['filein'] = options.filein
	config['fileout'] = options.fileout
	config['settype'] = options.settype
	config['setid'] = options.setid
	config['srclang'] = options.srclang
	config['tstlang'] = options.tstlang
	config['refid'] = options.refid
	config['sysid'] = options.sysid
	config['docid'] = options.docid
	config['genre'] = options.genre

	sys.stderr.write(": Configuration complete\n")

	return


licensetxt=u'''CorpusFiltergraph™
Copyright © 2010-2014 Precision Translation Tools Co., Ltd.

This module is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see http://www.gnu.org/licenses/.

For more information, please contact Precision Translation Tools Pte
at: http://www.precisiontranslationtools.com'''


def main():

	mksgm = makemteval()

	mksgm.parseini(mksgm.config)

	parsecmd(mksgm.config)

	mksgm.writesgm(mksgm.config)

	return 0


if __name__ == "__main__":
	sys.exit(main())
