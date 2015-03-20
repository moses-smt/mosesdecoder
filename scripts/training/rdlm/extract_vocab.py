#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Rico Sennrich

# extract 5 vocabulary files from parsed corpus in moses XML format

from __future__ import print_function, unicode_literals, division
import sys
import codecs
import argparse
from collections import Counter

# hack for python2/3 compatibility
from io import open
argparse.open = open

try:
    from lxml import etree as ET
except ImportError:
    from xml.etree import cElementTree as ET

def create_parser():

    help_text =  "generate 5 vocabulary files from parsed corpus in moses XML format\n"
    help_text += "  [PREFIX].special: around 40 symbols reserved for RDLM\n";
    help_text += "  [PREFIX].preterminals: preterminal symbols\n";
    help_text += "  [PREFIX].nonterminals: nonterminal symbols (which are not preterminal)\n";
    help_text += "  [PREFIX].terminals: terminal symbols\n";
    help_text += "  [PREFIX].all: all of the above\n"

    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description=help_text)

    parser.add_argument('--input', '-i', type=argparse.FileType('r'), default=sys.stdin, metavar='PATH',
                        help='input text (default: standard input).')
    parser.add_argument('--output', '-o', type=str, default='vocab', metavar='PREFIX',
                        help='output prefix (default: "vocab")')
    parser.add_argument('--ptkvz', action="store_true",
                    help='special rule for German dependency trees: attach separable verb prefixes to verb')

    return parser

def escape_text(s):

    s = s.replace('|','&#124;') # factor separator
    s = s.replace('[','&#91;') # syntax non-terminal
    s = s.replace(']','&#93;') # syntax non-terminal
    s = s.replace('\'','&apos;') # xml special character
    s = s.replace('"','&quot;') # xml special character
    return s

# deterministic heuristic to get head of subtree
def get_head(xml, args):
    head = None
    preterminal = None
    for child in xml:
        if not len(child):
            if head is not None:
                continue
            preterminal = child.get('label')
            head = escape_text(child.text.strip())

        # hack for split compounds
        elif child[-1].get('label') == 'SEGMENT':
            return escape_text(child[-1].text.strip()), 'SEGMENT'

        elif args.ptkvz and head and child.get('label') == 'avz':
            for grandchild in child:
                if grandchild.get('label') == 'PTKVZ':
                    head = escape_text(grandchild.text.strip()) + head
                    break

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

    special_tokens = ['<unk>', '<null>', '<null_label>', '<null_head>', '<head_label>', '<root_label>', '<start_label>', '<stop_label>', '<head_head>', '<root_head>', '<start_head>', '<dummy_head>', '<stop_head>']

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
