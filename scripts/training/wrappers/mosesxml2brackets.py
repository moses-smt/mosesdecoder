#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Rico Sennrich
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""Convert trees in moses XML format to PTB-style bracketed format."""

from __future__ import print_function, unicode_literals
import sys
import codecs

from lxml import etree as ET


def escape(word):
    # Factor separator:
    word = word.replace('|', '&#124;')
    # Syntax non-terminal:
    word = word.replace('[', '&#91;')
    # Syntax non-terminal:
    word = word.replace(']', '&#93;')
    word = word.replace('\'', '&apos;')
    word = word.replace('\"', '&quot;')

    return word


def make_brackets(xml):
    out = ' [' + xml.get('label')

    if xml.text and xml.text.strip():
        word = escape(xml.text.strip())
        out += ' ' + word + ']'

    else:
        for child in xml:
            out += make_brackets(child)

        out += ']'

    return out


if __name__ == '__main__':

    if sys.version_info < (3, 0):
        sys.stderr = codecs.getwriter('UTF-8')(sys.stderr)
        sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)
        sys.stdin = codecs.getreader('UTF-8')(sys.stdin)

    for line in sys.stdin:
        if line == '\n':
            sys.stdout.write(line)
            continue
        out = make_brackets(ET.fromstring(line)).strip()
        sys.stdout.write(out + '\n')
