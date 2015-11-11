#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Rico Sennrich
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""
Takes a file in the CoNLL dependency format (from the CoNLL-X shared task on
dependency parsing; http://ilk.uvt.nl/conll/#dataformat ) and produces
Moses XML format.

Note that the structure is built based on fields 9 and 10 (projective HEAD
and RELATION), which not all parsers produce.

Usage: conll2mosesxml.py [--brackets] < input_file > output_file
"""

from __future__ import print_function, unicode_literals
import sys
import re
import codecs
from collections import (
    namedtuple,
    defaultdict,
    )
from lxml import etree as ET


Word = namedtuple(
    'Word',
    ['pos', 'word', 'lemma', 'tag', 'head', 'func', 'proj_head', 'proj_func'])


def main(output_format='xml'):
    sentence = []

    for line in sys.stdin:

        # Process sentence.
        if line == "\n":
            sentence.insert(0, [])
            if is_projective(sentence):
                write(sentence, output_format)
            else:
                sys.stderr.write(
                    ' '.join(w.word for w in sentence[1:]) + '\n')
                sys.stdout.write('\n')
            sentence = []
            continue

        try:
            (
                pos,
                word,
                lemma,
                tag,
                tag2,
                morph,
                head,
                func,
                proj_head,
                proj_func,
            ) = line.split()
        except ValueError:  # Word may be unicode whitespace.
            (
                pos,
                word,
                lemma,
                tag,
                tag2,
                morph,
                head,
                func,
                proj_head,
                proj_func,
            ) = re.split(' *\t*', line.strip())

        word = escape_special_chars(word)
        lemma = escape_special_chars(lemma)

        if proj_head == '_':
            proj_head = head
            proj_func = func

        sentence.append(
            Word(
                int(pos), word, lemma, tag2, int(head), func, int(proj_head),
                proj_func))


# This script performs the same escaping as escape-special-chars.perl in
# Moses.  Most of it is done in function write(), but quotation marks need
# to be processed first.
def escape_special_chars(line):
    line = line.replace('\'', '&apos;')  # xml
    line = line.replace('"', '&quot;')  # xml
    line = line.replace('[', '&#91;')  # syntax non-terminal
    line = line.replace(']', '&#93;')  # syntax non-terminal

    return line


# make a check if structure is projective
def is_projective(sentence):
    dominates = defaultdict(set)
    for i, w in enumerate(sentence):
        dominates[i].add(i)
        if not i:
            continue
        head = int(w.proj_head)
        while head != 0:
            if i in dominates[head]:
                break
            dominates[head].add(i)
            head = int(sentence[head].proj_head)

    for i in dominates:
        dependents = dominates[i]
        if max(dependents) - min(dependents) != len(dependents) - 1:
            sys.stderr.write("error: non-projective structure.\n")
            return False
    return True


def write(sentence, output_format='xml'):

    if output_format == 'xml':
        tree = create_subtree(0, sentence)
        out = ET.tostring(tree, encoding='UTF-8').decode('UTF-8')

    if output_format == 'brackets':
        out = create_brackets(0, sentence)

    out = out.replace('|', '&#124;')  # factor separator

    # lxml is buggy if input is escaped:
    out = out.replace('&amp;apos;', '&apos;')
    # lxml is buggy if input is escaped:
    out = out.replace('&amp;quot;', '&quot;')
    # lxml is buggy if input is escaped:
    out = out.replace('&amp;#91;', '&#91;')
    # lxml is buggy if input is escaped:
    out = out.replace('&amp;#93;', '&#93;')

    print(out)


def create_subtree(position, sentence):
    """"Write node in Moses XML format."""
    element = ET.Element('tree')

    if position:
        element.set('label', sentence[position].proj_func)
    else:
        element.set('label', 'sent')

    for i in range(1, position):
        if sentence[i].proj_head == position:
            element.append(create_subtree(i, sentence))

    if position:

        if preterminals:
            head = ET.Element('tree')
            head.set('label', sentence[position].tag)
            head.text = sentence[position].word
            element.append(head)

        else:
            if len(element):
                element[-1].tail = sentence[position].word
            else:
                element.text = sentence[position].word

    for i in range(position, len(sentence)):
        if i and sentence[i].proj_head == position:
            element.append(create_subtree(i, sentence))

    return element


# write node in bracket format (Penn treebank style)
def create_brackets(position, sentence):

    if position:
        element = "[ " + sentence[position].proj_func + ' '
    else:
        element = "[ sent "

    for i in range(1, position):
        if sentence[i].proj_head == position:
            element += create_brackets(i, sentence)

    if position:
        word = sentence[position].word
        tag = sentence[position].tag

        if preterminals:
            element += '[ ' + tag + ' ' + word + ' ] '
        else:
            element += word + ' ] '

    for i in range(position, len(sentence)):
        if i and sentence[i].proj_head == position:
            element += create_brackets(i, sentence)

    if preterminals or not position:
        element += '] '

    return element

if __name__ == '__main__':
    if sys.version_info < (3, 0, 0):
        sys.stdin = codecs.getreader('UTF-8')(sys.stdin)
        sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)
        sys.stderr = codecs.getwriter('UTF-8')(sys.stderr)

    if '--no_preterminals' in sys.argv:
        preterminals = False
    else:
        preterminals = True

    if '--brackets' in sys.argv:
        main('brackets')
    else:
        main('xml')
