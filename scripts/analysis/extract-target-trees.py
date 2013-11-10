#!/usr/bin/env python

# Usage: extract-target-trees.py [FILE]
#
# Reads moses-chart's -T output from FILE or standard input and writes trees to
# standard output in Moses' XML tree format.

import re
import sys


class Tree:
    def __init__(self, label, children):
        self.label = label
        self.children = children

    def is_leaf(self):
        return len(self.children) == 0


class Derivation(list):
    def find_root(self):
        assert len(self) > 0
        root = None
        for hypothesis in self:
            if hypothesis.span[0] != 0:
                continue
            if root == None or hypothesis.span[1] > root.span[1]:
                root = hypothesis
        assert root
        return root

    def construct_target_tree(self):
        hypo_map = {}
        for hypothesis in self:
            hypo_map[hypothesis.span] = hypothesis
        root = self.find_root()
        return self._build_tree(root, hypo_map)

    def _build_tree(self, root, hypo_map):
        def escape_label(label):
            s = label.replace("&", "&amp;")
            s = s.replace("<", "&lt;")
            s = s.replace(">", "&gt;")
            return s

        # Build list of NT spans in source order...
        non_term_spans = []
        for item in root.source_symbol_info:
            span = item[0]
            if span != root.span and span in hypo_map: # In hypo_map iff symbol is NT
                non_term_spans.append(span)
        non_term_spans.sort()

        # ... then convert to target order.
        alignment_pairs = root.nt_alignments[:]
        alignment_pairs.sort()
        target_order_non_term_spans = {}
        for i, pair in enumerate(alignment_pairs):
            target_order_non_term_spans[pair[1]] = non_term_spans[i]

        children = []
        num_non_terms = 0

        for i, symbol in enumerate(root.target_rhs):
            if i in target_order_non_term_spans:
                hyp = hypo_map[target_order_non_term_spans[i]]
                children.append(self._build_tree(hyp, hypo_map))
                num_non_terms += 1
            else:
                children.append(Tree(escape_label(symbol), []))

        assert num_non_terms == len(root.nt_alignments)

        return Tree(root.target_lhs, children)


class Hypothesis:
    def __init__(self):
        self.sentence_num = None
        self.span = None
        self.source_symbol_info = None
        self.target_lhs = None
        self.target_rhs = None
        self.nt_alignments = None


def read_derivations(input):
    line_num = 0
    start_line_num = None
    prev_sentence_num = None
    derivation = Derivation()
    for line in input:
        line_num += 1
        hypothesis = parse_line(line)
        if hypothesis.sentence_num != prev_sentence_num:
            # We've started reading the next derivation...
            prev_sentence_num = hypothesis.sentence_num
            if len(derivation):
                yield derivation, start_line_num
                derivation = Derivation()
            start_line_num = line_num
        derivation.append(hypothesis)
    if len(derivation):
        yield derivation, start_line_num


# Extract the hypothesis components and return a Hypothesis object.
def parse_line(s):
    pattern = r"Trans Opt (\d+) " + \
              r"\[(\d+)\.\.(\d+)\]:" + \
              r"((?: \[\d+\.\.\d+\]=\S+  )+):" + \
              r" (\S+) ->\S+  -> " + \
              r"((?:\S+ )+):" + \
              r"((?:\d+-\d+ )*): c="
    regexp = re.compile(pattern)
    match = regexp.match(s)
    if not match:
        sys.stderr.write("%s\n" % s)
    assert match
    group = match.groups()
    hypothesis = Hypothesis()
    hypothesis.sentence_num = int(group[0]) + 1
    hypothesis.span = (int(group[1]), int(group[2]))
    hypothesis.source_symbol_info = []
    for item in group[3].split():
        pattern = "\[(\d+)\.\.(\d+)\]=(\S+)"
        regexp = re.compile(pattern)
        match = regexp.match(item)
        assert(match)
        start, end, symbol = match.groups()
        span = (int(start), int(end))
        hypothesis.source_symbol_info.append((span, symbol))
    hypothesis.target_lhs = group[4]
    hypothesis.target_rhs = group[5].split()
    hypothesis.nt_alignments = []
    for pair in group[6].split():
        match = re.match(r'(\d+)-(\d+)', pair)
        assert match
        ai = (int(match.group(1)), int(match.group(2)))
        hypothesis.nt_alignments.append(ai)
    return hypothesis


def tree_to_xml(tree):
    if tree.is_leaf():
        return tree.label
    else:
        s = '<tree label="%s"> ' % tree.label
        for child in tree.children:
            s += tree_to_xml(child)
            s += " "
        s += '</tree>'
        return s


def main():
    if len(sys.argv) > 2:
        sys.stderr.write("usage: %s [FILE]\n" % sys.argv[0])
        sys.exit(1)
    if len(sys.argv) == 1 or sys.argv[1] == "-":
        input = sys.stdin
    else:
        input = open(sys.argv[1])
    for derivation, line_num in read_derivations(input):
        try:
            tree = derivation.construct_target_tree()
        except:
            msg = "error processing derivation starting at line %d\n" % line_num
            sys.stderr.write(msg)
            raise
        print tree_to_xml(tree)


if __name__ == '__main__':
    main()
