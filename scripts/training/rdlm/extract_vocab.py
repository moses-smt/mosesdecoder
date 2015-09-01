#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Rico Sennrich
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# extract 5 vocabulary files from parsed corpus in moses XML format

from __future__ import print_function, unicode_literals, division
import sys
import codecs
import argparse
from collections import Counter
from textwrap import dedent

# hack for python2/3 compatibility
from io import open
argparse.open = open

try:
    from lxml import etree as ET
except ImportError:
    from xml.etree import cElementTree as ET


HELP_TEXT = dedent("""\
    generate 5 vocabulary files from parsed corpus in moses XML format
      [PREFIX].special: around 40 symbols reserved for RDLM
      [PREFIX].preterminals: preterminal symbols
      [PREFIX].nonterminals: nonterminal symbols (which are not preterminal)
      [PREFIX].terminals: terminal symbols
      [PREFIX].all: all of the above
""")


def create_parser():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=HELP_TEXT)

    parser.add_argument(
        '--input', '-i', type=argparse.FileType('r'), default=sys.stdin,
        metavar='PATH',
        help="Input text (default: standard input).")
    parser.add_argument(
        '--output', '-o', type=str, default='vocab', metavar='PREFIX',
        help="Output prefix (default: 'vocab')")

    return parser


def escape_text(s):
    s = s.replace('|', '&#124;')  # factor separator
    s = s.replace('[', '&#91;')  # syntax non-terminal
    s = s.replace(']', '&#93;')  # syntax non-terminal
    s = s.replace('\'', '&apos;')  # xml special character
    s = s.replace('"', '&quot;')  # xml special character
    return s


def get_head(xml, args):
    """Deterministic heuristic to get head of subtree."""
    head = None
    preterminal = None
    for child in xml:
        if not len(child):
            preterminal = child.get('label')
            head = escape_text(child.text.strip())
            return head, preterminal

    return head, preterminal


def get_vocab(xml, args):

    if len(xml):

        head, preterminal = get_head(xml, args)
        if not head:
            head = '<null>'
            preterminal = '<null>'

        heads[head] += 1
        preterminals[preterminal] += 1

        label = xml.get('label')

        nonterminals[label] += 1

        for child in xml:
            if not len(child):
                continue
            get_vocab(child, args)


def main(args):

    global heads
    global preterminals
    global nonterminals

    heads = Counter()
    preterminals = Counter()
    nonterminals = Counter()

    i = 0
    for line in args.input:
        if i and not i % 50000:
            sys.stderr.write('.')
        if i and not i % 1000000:
            sys.stderr.write('{0}\n'.format(i))
        if line == '\n':
            continue

        xml = ET.fromstring(line)
        get_vocab(xml, args)
        i += 1

    special_tokens = [
        '<unk>',
        '<null>',
        '<null_label>',
        '<null_head>',
        '<head_label>',
        '<root_label>',
        '<start_label>',
        '<stop_label>',
        '<head_head>',
        '<root_head>',
        '<start_head>',
        '<dummy_head>',
        '<stop_head>',
    ]

    for i in range(30):
        special_tokens.append('<null_{0}>'.format(i))

    f = open(args.output + '.special', 'w', encoding='UTF-8')
    for item in special_tokens:
        f.write(item + '\n')
    f.close()

    f = open(args.output + '.preterminals', 'w', encoding='UTF-8')
    for item in sorted(preterminals, key=preterminals.get, reverse=True):
        f.write(item + '\n')
    f.close()

    f = open(args.output + '.nonterminals', 'w', encoding='UTF-8')
    for item in sorted(nonterminals, key=nonterminals.get, reverse=True):
        f.write(item + '\n')
    f.close()

    f = open(args.output + '.terminals', 'w', encoding='UTF-8')
    for item in sorted(heads, key=heads.get, reverse=True):
        f.write(item + '\n')
    f.close()

    f = open(args.output + '.all', 'w', encoding='UTF-8')
    special_tokens_set = set(special_tokens)
    for item in sorted(nonterminals, key=nonterminals.get, reverse=True):
        if item not in special_tokens:
            special_tokens.append(item)
            special_tokens_set.add(item)
    for item in sorted(preterminals, key=preterminals.get, reverse=True):
        if item not in special_tokens:
            special_tokens.append(item)
            special_tokens_set.add(item)
    for item in special_tokens:
        f.write(item + '\n')
    i = len(special_tokens)

    for item in sorted(heads, key=heads.get, reverse=True):
        if item in special_tokens_set:
            continue
        i += 1
        f.write(item + '\n')
    f.close()


if __name__ == '__main__':

    if sys.version_info < (3, 0):
        sys.stderr = codecs.getwriter('UTF-8')(sys.stderr)
        sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)
        sys.stdin = codecs.getreader('UTF-8')(sys.stdin)

    parser = create_parser()
    args = parser.parse_args()
    main(args)
