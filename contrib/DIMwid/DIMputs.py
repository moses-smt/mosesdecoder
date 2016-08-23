# -*- coding: utf-8 -*-

import collections
import re


class DataInput():
    def __init__(self, file_name):
        self.file = open(file_name, "r")
        self.sentences = None

        
    def read_phrase(self):
        self.sentences = []
        sentence = None
        span_reg = re.compile("\|[0-9]+-[0-9]+\|")
        previous = ""
        for line in self.file:
            sentence = Single()
            for word in line.split():
                if span_reg.match(word):
                    sentence.spans[tuple([int(i) for i in word.strip("|").split("-")])] = previous.strip()
                    previous = " "
                else:
                    previous += word + " "
            sentence.set_length()
            self.sentences.append(sentence)
            sentence.number = len(self.sentences)

    def read_syntax(self):
        self.sentences = []
        sentence = None
        number = -1
        for line in self.file:
            if int(line.split()[2]) != number:
                if sentence is not None:
                    sentence.set_length()
                    self.sentences.append(sentence)
                sentence = Single()
                sentence.number = int(line.split()[2])
                number = sentence.number
            sentence.spans[tuple([int(i) for i in line.split()[3].strip(":[]").split("..")])] \
 = line.strip()

        if sentence is not None:
            sentence.set_length()
            self.sentences.append(sentence)
                # = tuple([line.split(":")[1], line.split(":")[2], line.split(":")[3]])
                
    
    def read_syntax_cubes(self, cell_limit):
        self.sentences = []
        sentence = None
        number = -1
        new_item = False
        for line in self.file:
            if  line.startswith("Chart Cell"):
                pass  # we dont care for those lines
            elif line.startswith("---------"):
                new_item = True
            elif line.startswith("Trans Opt") and new_item is True:
                new_item = False
                if int(line.split()[2]) != number:
                    if sentence is not None:
                        sentence.set_length()
                        self.sentences.append(sentence)
                    sentence = Multiple()
                    sentence.number = int(line.split()[2])
                    number = sentence.number
                span = tuple([int(i) for i in line.split()[3].strip(":[]").split("..")])                
                if len(sentence.spans[span]) < cell_limit:
                    sentence.spans[span].append(line.strip())
        if sentence is not None:
            sentence.set_length()
            self.sentences.append(sentence)
    
    def read_phrase_stack_flag(self, cell_limit):
        self.sentences = []
        sentence = None
        number = -1
        for line in self.file:
            if len(line.split()) < 6:
                pass
#            elif re.match("recombined=[0-9]+", line.split()[6]):
#                pass
            else:
                if int(line.split()[0]) != number:
                    if sentence is not None:
                        sentence.set_length()
                        self.sentences.append(sentence)
                    sentence = Multiple()
                    sentence.number = int(line.split()[0])
                    number = sentence.number
#                span = tuple([int(i) for i in line.split()[8].split("=")[1].split("-")])
                span = re.search(r"covered=([0-9]+\-[0-9]+)", line).expand("\g<1>")
                # print span.expand("\g<1>")
                span = tuple([int(i) for i in span.split("-")])
                if len(sentence.spans[span]) < cell_limit:
                    sentence.spans[span].append(line.strip())
        if sentence is not None:
            sentence.set_length()
            self.sentences.append(sentence)
            
    def read_phrase_stack_verbose(self, cell_limit):
        self.sentences = []
        sentence = None
        number = -1
        span_input = False
        for line in self.file:
            if line.startswith("Translating: "):
                if sentence is not None:
                    sentence.set_length()
                    self.sentences.append(sentence)
                    
                number += 1
                sentence = Multiple()
                sentence.number = number
            else:
                if re.match("\[[A-Z,a-z,\ ]+;\ [0-9]+-[0-9]+\]", line):
                    span = tuple([int(i) for i in line.split(";")[1].strip().strip("]").split("-")])
                    sentence.spans[span].append(line.strip())
                    span_input = True
#                    print line,
                elif span_input is True:
                    if line.strip() == "":
                        span_input = False
#                        print "X"
                    else:
                        if len(sentence.spans[span]) < cell_limit:
                            sentence.spans[span].append(line.strip())
#                        print line,
        if sentence is not None:
            sentence.set_length()
            self.sentences.append(sentence)
            
            

    def read_syntax_cube_flag(self, cell_limit):
        self.sentences = []
        sentence = None
        number = -1
        for line in self.file:
            if len(line.split()) < 6:
                pass
            else:
                if int(line.split()[0]) != number:
                    if sentence is not None:
                        sentence.set_length()
                        self.sentences.append(sentence)
                    sentence = Multiple()  # 
                    sentence.number = int(line.split()[0])
                    number = sentence.number
                span = re.search(r"\[([0-9]+)\.\.([0-9]+)\]", line).expand("\g<1> \g<2>")
                span = tuple([int(i) for i in span.split()])
                if len(sentence.spans[span]) < cell_limit:
                    sentence.spans[span].append(line.strip())
        if sentence is not None:
            sentence.set_length()
            self.sentences.append(sentence)
        
        
    def read_mbot(self, cell_limit):
        self.sentences = []
        sentence = None
        number = -1
        hypo = False
        rule = False
        popping = False
        target = ""
        source = ""
        source_parent = ""
        target_parent = ""
        alignment = ""
        for line in self.file:
            if line.startswith("Translating:"):
                if sentence is not None:
                    sentence.set_length()
                    self.sentences.append(sentence)
                sentence = Multiple()
                sentence.number = number + 1
                number = sentence.number
            elif line.startswith("POPPING"):
                popping = True 
            elif popping is True:
                popping = False
                span = tuple([int(i) for i in line.split()[1].strip("[").split("]")[0].split("..")])
                hypo = True
            elif hypo is True:
                if line.startswith("Target Phrases"):
                    target = line.split(":", 1)[1].strip()
                
                elif line.startswith("Alignment Info"):
                    alignment = line.split(":", 1)[1].strip()
                    if alignment == "":
                        alignment = "(1)"
                    
                elif line.startswith("Source Phrase"):
                    source = line.split(":", 1)[1].strip()
    
                elif line.startswith("Source Left-hand-side"):
                    source_parent = line.split(":", 1)[1].strip()
    
                elif line.startswith("Target Left-hand-side"):
                    target_parent = line.split(":", 1)[1].strip()
    
                    # Input stored: now begin translation into rule-format
                    alignment = re.sub(r"\([0-9]+\)", "||", alignment)
                    align_blocks = alignment.split("||")[:-1]
                    target = re.sub(r"\([0-9]+\)", "||", target)
                    target = [x.split() for x in target.split("||")][:-1]
                    source = source.split()
    
                    for i in range(len(source)):
                        if source[i].isupper():
                            source[i] = "[" + source[i] + "]"
                            for k in range(len(align_blocks)):
                                align_pairs = [tuple([int(y) for y in x.split("-")]) for x in align_blocks[k].split()]
                                for j in filter(lambda x: x[0] == i, align_pairs):
                                    source[i] = source[i] + "[" + target[k][j[1]] + "]"
    
                    for i in range(len(target)):
                        for j in range(len(target[i])):
                            align_pairs = [tuple([int(y) for y in x.split("-")]) for x in align_blocks[i].split()]
                            for k in filter(lambda x: x[1] == j, align_pairs):
                                target[i][j] = source[k[0]].split("]")[0] + "][" + target[i][j] + "]"
                
                
                
                    target = " || ".join([" ".join(x) for x in target]) + " ||"
    
                    source = " ".join(source)
                    source = source + "  [" + source_parent + "]"
    
                    tp = re.sub(r"\([0-9]+\)", "", target_parent).split()
                    for i in tp:
                        target = target.replace("||", " [" + i + "] !!", 1)
                    target = target.replace("!!", "||")
                        
                    rule = False
                    search_pattern = "|||  " + source + " ||| " + target + "| ---  ||| " + alignment + "|" 
    
                    sentence.spans[span].append(search_pattern)
#                    print search_pattern, span
                    if len(sentence.spans[span]) < cell_limit:
                        sentence.spans[span].append(search_pattern)
            else:
                pass
        if sentence is not None:
            sentence.set_length()
            self.sentences.append(sentence)




class Single():
    def __init__(self):
        self.number = None
        self.spans = {}
        self.length = None

    def set_length(self):
        self.length = max([x[1] for x in self.spans.keys()])
    
    def __str__(self):
        number = str(self.number)
        length = str(self.length)
        spans = "\n"
        for i in self.spans.keys():
            spans += str(i) + " - " + str(self.spans[i]) + "\n"
        return str((number, length, spans))

class Multiple():
    def __init__(self):
        self.number = None
        self.spans = collections.defaultdict(list)
        self.length = None

    def set_length(self):
        self.length = max([x[1] for x in self.spans.keys()])
    
    def __str__(self):
        number = str(self.number)
        length = str(self.length)
        spans = "\n"
        for i in self.spans.keys():
            spans += str(i) + " - " + str(self.spans[i]) + "\n"
        return str((number, length, spans))



