#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Rico Sennrich
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

"""
Extract syntactic n-grams from dependency treebank in Moses XML format for
training RDLM.

Expected format can be produced with
mosesdecoder/scripts/training/wrapper/conll2mosesxml.py

OOV terminal symbols are mapped to preterminal; OOV nonterminals are mapped
to 0 (<unk>)
"""

from __future__ import print_function, unicode_literals, division
import sys
import codecs
import argparse

# Hack for python2/3 compatibility
from io import open

argparse.open = open

try:
    from lxml import etree as ET
except ImportError:
    from xml.etree import cElementTree as ET


def create_parser():
    parser = argparse.ArgumentParser(
        description=(
            "Extract syntactic n-grams from parsed corpus in "
            "Moses XML format for training RDLM"))

    parser.add_argument(
        '--input', '-i', type=argparse.FileType('r'), default=sys.stdin,
        metavar='PATH',
        help='Input file (default: standard input).')
    parser.add_argument(
        '--output', '-o', type=argparse.FileType('w'), default=sys.stdout,
        metavar='PATH',
        help='Output file (default: standard output).')
    parser.add_argument(
        '--mode', type=str, choices=['label', 'head'], required=True,
        help='Predict terminals (head) or dependency labels (label).')
    parser.add_argument(
        '--vocab', metavar='PATH', type=str, required=True,
        help=(
            "Input layer vocabulary file (one item per line; "
            "first line '<unk>')"))
    parser.add_argument(
        '--output_vocab', metavar='PATH', type=str,
        help=(
            "Output layer vocabulary file "
            "(default: use input layer vocabulary)"))
    parser.add_argument(
        '--left_context', metavar='INT', type=int, default=3,
        help=(
            "Size of context vector for left siblings "
            "(default: %(default)s)"))
    parser.add_argument(
        '--right_context', metavar='INT', type=int, default=0,
        help=(
            "Size of context vector for right siblings "
            "(default: %(default)s)"))
    parser.add_argument(
        '--up_context', metavar='INT', type=int, default=2,
        help=(
            "Size of context vector for ancestors "
            "(default: %(default)s)"))
    parser.add_argument(
        '--glue_symbol', metavar='STR', type=str, default='Q',
        help=(
            "Glue symbol. Will be skipped during extraction "
            "(default: %(default)s)"))
    parser.add_argument(
        '--start_symbol', metavar='STR', type=str, default='SSTART',
        help=(
            "Sentence start symbol. Will be skipped during extraction "
            "(default: %(default)s)"))
    parser.add_argument(
        '--end_symbol', metavar='STR', type=str, default='SEND',
        help=(
            "Sentence end symbol. Will be skipped during extraction "
            "(default: %(default)s)"))
    return parser


def escape_text(s):

    s = s.replace('|', '&#124;')  # factor separator
    s = s.replace('[', '&#91;')  # syntax non-terminal
    s = s.replace(']', '&#93;')  # syntax non-terminal
    s = s.replace('\'', '&apos;')  # xml special character
    s = s.replace('"', '&quot;')  # xml special character
    return s


def get_head(xml):
    """Deterministic heuristic to get head of subtree."""
    head = None
    preterminal = None
    for child in xml:
        if not len(child):
            preterminal = child.get('label')
            head = escape_text(child.text.strip())
            return head, preterminal

    return head, preterminal


def get_syntactic_ngrams(xml, options, vocab, output_vocab,
                         parent_heads=None, parent_labels=None):

    if len(xml):

        # Skip glue rules.
        skip_glue_labels = [
            options.glue_symbol,
            options.start_symbol,
            options.end_symbol,
            ]
        if xml.get('label') in skip_glue_labels:
            for child in xml:
                get_syntactic_ngrams(
                    child, options, vocab, output_vocab, parent_heads,
                    parent_labels)
            return

        # Skip virtual nodes.
        skip_virtual_labels = [
            '<stop_label>',
            '<start_label>',
            ]
        if xml.get('label') in skip_virtual_labels:
            return

        if not parent_heads:
            parent_heads = (
                [vocab.get('<root_head>', 0)] * options.up_context)
            parent_labels = (
                [vocab.get('<root_label>', 0)] * options.up_context)

            head, preterminal = get_head(xml)
            if not head:
                head = '<dummy_head>'
                preterminal = head
            elif head not in vocab:
                head = preterminal

            label = xml.get('label')

            # syntactic n-gram for root node
            int_list = []
            int_list.extend([start_head_idx] * options.left_context)
            int_list.extend([start_label_idx] * options.left_context)
            int_list.extend([stop_head_idx] * options.right_context)
            int_list.extend([stop_label_idx] * options.right_context)
            int_list.extend(parent_heads)
            int_list.extend(parent_labels)

            # write root of tree
            if options.mode == 'label':
                int_list.append(output_vocab.get(label, 0))
                options.output.write(' '.join(map(str, int_list)) + '\n')
            elif options.mode == 'head' and not head == '<dummy_head>':
                int_list.append(vocab.get(label, 0))
                int_list.append(
                    output_vocab.get(head, output_vocab.get(preterminal, 0)))
                options.output.write(' '.join(map(str, int_list)) + '\n')

            parent_heads.append(vocab.get(head, 0))
            parent_labels.append(vocab.get(label, 0))

        # virtual start/end-of-subtree tag
        if len(xml) > 0:
            if options.right_context:
                start = ET.Element('tree')
                start2 = ET.Element('tree')
                start.set('label', '<start_label>')
                start2.set('label', 'XY')
                start2.text = '<start_head>'
                start.append(start2)
                xml.insert(0, start)
            if options.left_context:
                end = ET.Element('tree')
                end2 = ET.Element('tree')
                end.set('label', '<stop_label>')
                end2.set('label', 'XY')
                end2.text = '<stop_head>'
                end.append(end2)
                xml.append(end)

        heads = []
        preterminals = []
        labels = []

        for child in xml:
            if not len(child):
                # Mark that the previous sibling is the head of the
                # structure (the head/label are not repeated because they're
                # also head/label of the parent).
                head_child = '<head_head>'
                preterminal_child = head_child
                child_label = '<head_label>'
            else:
                head_child, preterminal_child = get_head(child)
                child_label = child.get('label')

            if head_child is None:
                head_child = '<dummy_head>'

            heads.append(head_child)
            preterminals.append(preterminal_child)
            labels.append(child_label)

            heads_idx = [
                vocab.get(heads[i], vocab.get(preterminals[i], 0))
                for i in range(len(heads))]
            labels_idx = [
                vocab.get(labels[i], 0)
                for i in range(len(labels))]

        # Ancestor context is the same for all children.
        up_heads = parent_heads[-options.up_context:]
        up_labels = parent_labels[-options.up_context:]

        skip_special_heads = [
            '<dummy_head>',
            '<head_head>',
            '<stop_head>',
            '<start_head>',
            ]
        for i, child in enumerate(xml):

            # Skip some special symbols, but recursively extract n-grams
            # for its children.
            if options.mode == 'head' and heads[i] in skip_special_heads:
                parent_heads.append(vocab.get(heads[i], 0))
                parent_labels.append(vocab.get(labels[i], 0))
                get_syntactic_ngrams(
                    child, options, vocab, output_vocab, parent_heads,
                    parent_labels)
                parent_heads.pop()
                parent_labels.pop()
                continue

            previous_heads = heads_idx[max(0, i - options.left_context):i]
            previous_labels = labels_idx[max(0, i - options.left_context):i]

            subsequent_heads = heads_idx[i + 1:i + options.right_context + 1]
            subsequent_labels = labels_idx[i + 1:i + options.right_context + 1]

            if len(previous_heads) < options.left_context:
                previous_heads = (
                    [start_head_idx] *
                    (options.left_context - len(previous_heads)) +
                    previous_heads)
                previous_labels = (
                    [start_label_idx] *
                    (options.left_context - len(previous_labels)) +
                    previous_labels)

            if len(subsequent_heads) < options.right_context:
                subsequent_heads += (
                    [stop_head_idx] *
                    (options.right_context - len(subsequent_heads)))
                subsequent_labels += (
                    [stop_label_idx] *
                    (options.right_context - len(subsequent_labels)))

            int_list = []
            int_list.extend(previous_heads)
            int_list.extend(previous_labels)
            int_list.extend(subsequent_heads)
            int_list.extend(subsequent_labels)
            int_list.extend(up_heads)
            int_list.extend(up_labels)
            if options.mode == 'label':
                int_list.append(output_vocab.get(labels[i], 0))
            elif options.mode == 'head':
                int_list.append(vocab.get(labels[i], 0))
                int_list.append(
                    output_vocab.get(
                        heads[i], output_vocab.get(preterminals[i], 0)))

            options.output.write(' '.join(map(str, int_list)) + '\n')

            parent_heads.append(
                vocab.get(heads[i], vocab.get(preterminals[i], 0)))
            parent_labels.append(vocab.get(labels[i], 0))

            get_syntactic_ngrams(
                child, options, vocab, output_vocab, parent_heads,
                parent_labels)

            parent_heads.pop()
            parent_labels.pop()


def load_vocab(path):
    v = {}
    for i, line in enumerate(open(path, encoding="UTF-8")):
        v[line.strip()] = i
    return v


def main(options):
    vocab = load_vocab(options.vocab)

    if options.output_vocab is None:
        sys.stderr.write(
            "No output vocabulary specified; using input vocabulary.\n")
        output_vocab = vocab
    else:
        output_vocab = load_vocab(options.output_vocab)

    global start_head_idx
    global start_label_idx
    global stop_head_idx
    global stop_label_idx
    start_head_idx = vocab.get("<start_head>", 0)
    start_label_idx = vocab.get("<start_label>", 0)
    stop_head_idx = vocab.get("<stop_head>", 0)
    stop_label_idx = vocab.get("<stop_label>", 0)

    i = 0
    for line in options.input:
        if i and not i % 50000:
            sys.stderr.write('.')
        if i and not i % 1000000:
            sys.stderr.write('{0}\n'.format(i))
        if sys.version_info < (3, 0):
            if line == b'\n':
                continue
            # hack for older moses versions with inconsistent encoding of "|"
            line = line.replace(b'&bar;', b'&#124;')
        else:
            if line == '\n':
                continue
            # hack for older moses versions with inconsistent encoding of "|"
            line = line.replace('&bar;', '&#124;')
        xml = ET.fromstring(line)
        get_syntactic_ngrams(xml, options, vocab, output_vocab)
        i += 1

if __name__ == '__main__':

    if sys.version_info < (3, 0):
        sys.stderr = codecs.getwriter('UTF-8')(sys.stderr)
        sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)

    parser = create_parser()
    options = parser.parse_args()

    main(options)
