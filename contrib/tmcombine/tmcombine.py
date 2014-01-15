#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Rico Sennrich <sennrich [AT] cl.uzh.ch>

# This program handles the combination of Moses phrase tables, either through
# linear interpolation of the phrase translation probabilities/lexical weights,
# or through a recomputation based on the (weighted) combined counts.
#
# It also supports an automatic search for weights that minimize the cross-entropy
# between the model and a tuning set of word/phrase alignments.

# for usage information, run
# python tmcombine.py -h
# you can also check the docstrings of Combine_TMs() for more information and find some example commands in the function test()


# Some general things to note:
#  - Different combination algorithms require different statistics. To be on the safe side, use the option `-write-lexical-counts` when training models.
#  - The script assumes that phrase tables are sorted (to allow incremental, more memory-friendly processing). sort with LC_ALL=C.
#  - Some configurations require additional statistics that are loaded in memory (lexical tables; complete list of target phrases). If memory consumption is a problem, use the option --lowmem (slightly slower and writes temporary files to disk), or consider pruning your phrase table before combining (e.g. using Johnson et al. 2007).
#  - The script can read/write gzipped files, but the Python implementation is slow. You're better off unzipping the files on the command line and working with the unzipped files.
#  - The cross-entropy estimation assumes that phrase tables contain true probability distributions (i.e. a probability mass of 1 for each conditional probability distribution). If this is not true, the results are skewed.
#  - Unknown phrase pairs are not considered for the cross-entropy estimation. A comparison of models with different vocabularies may be misleading.
#  - Don't directly compare cross-entropies obtained from a combination with different modes. Depending on how some corner cases are treated, linear interpolation does not distribute full probability mass and thus shows higher (i.e. worse) cross-entropies.


from __future__ import division, unicode_literals
import sys
import os
import gzip
import argparse
import copy
import re
from math import log, exp
from collections import defaultdict
from operator import mul
from tempfile import NamedTemporaryFile
from subprocess import Popen
try:
    from itertools import izip
except:
    izip = zip

try:
    from lxml import etree as ET
except:
    import xml.etree.cElementTree as ET

try:
    from scipy.optimize.lbfgsb import fmin_l_bfgs_b
    optimizer = 'l-bfgs'
except:
    optimizer = 'hillclimb'

class Moses():
    """Moses interface for loading/writing models
    to support other phrase table formats, subclass this and overwrite the relevant functions
    """
    
    def __init__(self,models,number_of_features):

        self.number_of_features = number_of_features
        self.models = models

        #example item (assuming mode=='counts' and one feature): phrase_pairs['the house']['das haus'] = [[[10,100]],['0-0 1-1']]
        self.phrase_pairs = defaultdict(lambda: defaultdict(lambda: [[[0]*len(self.models) for i in range(self.number_of_features)],[]]))
        self.phrase_source = defaultdict(lambda: [0]*len(self.models))
        self.phrase_target =  defaultdict(lambda: [0]*len(self.models))
        
        self.reordering_pairs = defaultdict(lambda: defaultdict(lambda: [[0]*len(self.models) for i in range(self.number_of_features)]))
        
        self.word_pairs_e2f = defaultdict(lambda: defaultdict(lambda: [0]*len(self.models)))
        self.word_pairs_f2e = defaultdict(lambda: defaultdict(lambda: [0]*len(self.models)))
        self.word_source = defaultdict(lambda: [0]*len(self.models))
        self.word_target = defaultdict(lambda: [0]*len(self.models))
        
        self.require_alignment = False
        

    def open_table(self,model,table,mode='r'):
        """define which paths to open for lexical tables and phrase tables.
            we assume canonical Moses structure, but feel free to overwrite this
        """
        
        if table == 'reordering-table':
            table = 'reordering-table.wbe-msd-bidirectional-fe'
        
        filename = os.path.join(model,'model',table)
        fileobj = handle_file(filename,'open',mode)
        return fileobj
        
        
    def load_phrase_features(self,line,priority,i,mode='interpolate',store='pairs',filter_by=None,filter_by_src=None,filter_by_target=None,inverted=False,flags=None):
        """take single phrase table line and store probablities in internal data structure"""
        
        src = line[0]
        target = line[1]
        
        if inverted:
            src,target = target,src
        
        if (store == 'all' or store == 'pairs') and (priority < 10 or (src in self.phrase_pairs and target in self.phrase_pairs[src])) and not (filter_by and not (src in filter_by and target in filter_by[src])):
            
                self.store_info(src,target,line)
                
                scores = line[2].split()
                if len(scores) <self.number_of_features:
                    sys.stderr.write('Error: model only has {0} features. Expected {1}.\n'.format(len(scores),self.number_of_features))
                    exit()
                    
                scores = scores[:self.number_of_features]
                model_probabilities = map(float,scores)
                phrase_probabilities = self.phrase_pairs[src][target][0]
                
                if mode == 'counts' and not priority == 2: #priority 2 is MAP
                    try:
                        counts = map(float,line[-1].split())
                        try:
                            target_count,src_count,joint_count = counts
                            joint_count_e2f = joint_count
                            joint_count_f2e = joint_count
                        except ValueError:
                            # possibly old-style phrase table with 2 counts in last column, or phrase table produced by tmcombine
                            # note: since each feature has different weight vector, we may have two different phrase pair frequencies
                            target_count,src_count = counts
                            i_e2f = flags['i_e2f']
                            i_f2e = flags['i_f2e']
                            joint_count_e2f = model_probabilities[i_e2f] * target_count
                            joint_count_f2e = model_probabilities[i_f2e] * src_count
                    except:
                        sys.stderr.write(str(b" ||| ".join(line))+b'\n')
                        sys.stderr.write('ERROR: counts are missing or misformatted. Maybe your phrase table is from an older Moses version that doesn\'t store counts or word alignment?\n')
                        raise
                    
                    i_e2f = flags['i_e2f']
                    i_f2e = flags['i_f2e']
                    model_probabilities[i_e2f] = joint_count_e2f
                    model_probabilities[i_f2e] = joint_count_f2e
                        
                for j,p in enumerate(model_probabilities):
                    phrase_probabilities[j][i] = p
                
        # mark that the src/target phrase has been seen.
        # needed for re-normalization during linear interpolation
        if (store == 'all' or store == 'source') and not (filter_by_src and not src in filter_by_src):
            if mode == 'counts' and not priority == 2: #priority 2 is MAP
                try:
                    self.phrase_source[src][i] = float(line[-1].split()[1])
                except:
                    sys.stderr.write(str(line)+'\n')
                    sys.stderr.write('ERROR: Counts are missing or misformatted. Maybe your phrase table is from an older Moses version that doesn\'t store counts or word alignment?\n')
                    raise
            else:
                self.phrase_source[src][i] = 1
                
        if (store == 'all' or store == 'target') and not (filter_by_target and not target in filter_by_target):
            if mode == 'counts' and not priority == 2: #priority 2 is MAP
                try:
                    self.phrase_target[target][i] = float(line[-1].split()[0])
                except:
                    sys.stderr.write(str(line)+'\n')
                    sys.stderr.write('ERROR: Counts are missing or misformatted. Maybe your phrase table is from an older Moses version that doesn\'t store counts or word alignment?\n')
                    raise
            else:
                self.phrase_target[target][i] = 1


    def load_reordering_probabilities(self,line,priority,i,**unused):
        """take single reordering table line and store probablities in internal data structure"""
        
        src = line[0]
        target = line[1]

        model_probabilities = map(float,line[2].split())
        reordering_probabilities = self.reordering_pairs[src][target]
        
        try:
            for j,p in enumerate(model_probabilities):
                reordering_probabilities[j][i] = p
        except IndexError:
            sys.stderr.write('\nIndexError: Did you correctly specify the number of reordering features? (--number_of_features N in command line)\n')
            exit()

    def traverse_incrementally(self,table,models,load_lines,store_flag,mode='interpolate',inverted=False,lowmem=False,flags=None):
        """hack-ish way to find common phrase pairs in multiple models in one traversal without storing it all in memory
            relies on alphabetical sorting of phrase table.
        """
        
        increment = -1
        stack = ['']*len(self.models)
        
        while increment:
            
            self.phrase_pairs = defaultdict(lambda: defaultdict(lambda: [[[0]*len(self.models) for i in range(self.number_of_features)],[]]))
            self.reordering_pairs = defaultdict(lambda: defaultdict(lambda: [[0]*len(self.models) for i in range(self.number_of_features)]))
            self.phrase_source = defaultdict(lambda: [0]*len(self.models))
            
            if lowmem:
                self.phrase_target = defaultdict(lambda: [0]*len(self.models))
    
            for model,priority,i in models:
                
                if stack[i]:
                    if increment != stack[i][0]:
                        continue
                    else:
                        load_lines(stack[i],priority,i,mode=mode,store=store_flag,inverted=inverted,flags=flags)
                        stack[i] = ''
                    
                for line in model:

                    line = line.rstrip().split(b' ||| ')
                
                    if increment != line[0]:
                        stack[i] = line
                        break
                        
                    load_lines(line,priority,i,mode=mode,store=store_flag,inverted=inverted,flags=flags)
                
            yield 1

            #calculate which source phrase to process next
            lines = [line[0] + b' |' for line in stack if line]
            if lines:
                increment = min(lines)[:-2]
            else:
                increment = None
    
    
    def load_word_probabilities(self,line,side,i,priority,e2f_filter=None,f2e_filter=None):
        """process single line of lexical table"""
        
        a, b, prob = line.split(b' ')
        
        if side == 'e2f' and (not e2f_filter or a in e2f_filter and b in e2f_filter[a]):
            
            self.word_pairs_e2f[a][b][i] = float(prob)
            
        elif side == 'f2e' and (not f2e_filter or a in f2e_filter and b in f2e_filter[a]):
            
            self.word_pairs_f2e[a][b][i] = float(prob)
    

    def load_word_counts(self,line,side,i,priority,e2f_filter=None,f2e_filter=None,flags=None):
        """process single line of lexical table"""
        
        a, b, ab_count, b_count = line.split(b' ')
        
        if side == 'e2f':
            
            if priority == 2: #MAP
                if not e2f_filter or a in e2f_filter:
                    if not e2f_filter or b in e2f_filter[a]:
                        self.word_pairs_e2f[a][b][i] = float(ab_count)/float(b_count)
                    self.word_target[b][i] = 1
            else:
                if not e2f_filter or a in e2f_filter:
                    if not e2f_filter or b in e2f_filter[a]:
                        self.word_pairs_e2f[a][b][i] = float(ab_count)
                    self.word_target[b][i] = float(b_count)

        elif side == 'f2e':
            
            if priority == 2: #MAP
                if not f2e_filter or a in f2e_filter and b in f2e_filter[a]:
                    if not f2e_filter or b in f2e_filter[a]:
                        self.word_pairs_f2e[a][b][i] = float(ab_count)/float(b_count)
                    self.word_source[b][i] = 1
            else:
                if not f2e_filter or a in f2e_filter and b in f2e_filter[a]:
                    if not f2e_filter or b in f2e_filter[a]:
                        self.word_pairs_f2e[a][b][i] = float(ab_count)
                    self.word_source[b][i] = float(b_count)


    def load_lexical_tables(self,models,mode,e2f_filter=None,f2e_filter=None):
        """open and load lexical tables into data structure"""
        
        if mode == 'counts':
            files = ['lex.counts.e2f','lex.counts.f2e']
            load_lines = self.load_word_counts
            
        else:
            files = ['lex.e2f','lex.f2e']
            load_lines = self.load_word_probabilities
        
        j = 0
        
        for f in files:
            models_prioritized = [(self.open_table(model,f),priority,i) for (model,priority,i) in priority_sort_models(models)]
        
            for model,priority,i in models_prioritized:
                for line in model:
                    if not j % 100000:
                        sys.stderr.write('.')
                    j += 1
                    load_lines(line,f[-3:],i,priority,e2f_filter=e2f_filter,f2e_filter=f2e_filter)
                

    def store_info(self,src,target,line):
        """store alignment info and comment section for re-use in output"""
        
        if len(line) == 5:
            self.phrase_pairs[src][target][1] = line[3:5]
        
        # assuming that alignment is empty
        elif len(line) == 4:
            if self.require_alignment:
                sys.stderr.write('Error: unexpected phrase table format. Your current configuration requires alignment information. Make sure you trained your model with -phrase-word-alignment (default in newer Moses versions)\n')
                exit()
            
            self.phrase_pairs[src][target][1] = [b'',line[3].lstrip(b'| ')]
   
        else:
            sys.stderr.write('Error: unexpected phrase table format. Are you using a very old/new version of Moses with different formatting?\n')
            exit()
   
   
    def get_word_alignments(self,src,target,cache=False,mycache={}):
        """from the Moses phrase table alignment info in the form "0-0 1-0",
           get the aligned word pairs / NULL alignments
        """
        
        if cache:
            if (src,target) in mycache:
                return mycache[(src,target)]
        
        try:
            alignment = self.phrase_pairs[src][target][1][0]
        except:
            return None,None
        
        src_list = src.split(b' ')
        target_list = target.split(b' ')
        
        textual_e2f = [[s,[]] for s in src_list]
        textual_f2e = [[t,[]] for t in target_list]
        
        for pair in alignment.split(b' '):
            s,t = pair.split('-')
            s,t = int(s),int(t)
            
            textual_e2f[s][1].append(target_list[t])
            textual_f2e[t][1].append(src_list[s])

        for s,t in textual_e2f:
            if not t:
                t.append('NULL')
                
        for s,t in textual_f2e:
            if not t:
                t.append('NULL')
         
        #tupelize so we can use the value as dictionary keys
        for i in range(len(textual_e2f)):
            textual_e2f[i][1] = tuple(textual_e2f[i][1])

        for i in range(len(textual_f2e)):
            textual_f2e[i][1] = tuple(textual_f2e[i][1])

        if cache:
            mycache[(src,target)] = textual_e2f,textual_f2e
            
        return textual_e2f,textual_f2e
   
   
    def write_phrase_table(self,src,target,weights,features,mode,flags):
        """convert data to string in Moses phrase table format"""
        
        # if one feature value is 0 (either because of loglinear interpolation or rounding to 0), don't write it to phrasetable
        # (phrase pair will end up with probability zero in log-linear model anyway)
        if 0 in features:
            return ''
        
        # information specific to Moses model: alignment info and comment section with target and source counts
        alignment,comments = self.phrase_pairs[src][target][1]
        if alignment:
            extra_space = b' '
        else:
            extra_space = b''

        if mode == 'counts':
            i_e2f = flags['i_e2f']
            i_f2e = flags['i_f2e']
            srccount =  dot_product(self.phrase_source[src],weights[i_f2e])
            targetcount = dot_product(self.phrase_target[target],weights[i_e2f])
            comments = b"%s %s" %(targetcount,srccount)
            
        features = b' '.join([b'%.6g' %(f) for f in features])
        
        if flags['add_origin_features']:
            origin_features = map(lambda x: 2.718**bool(x),self.phrase_pairs[src][target][0][0]) # 1 if phrase pair doesn't occur in model, 2.718 if it does
            origin_features = b' '.join([b'%.4f' %(f) for f in origin_features]) + ' '
        else:
            origin_features = b''
        if flags['write_phrase_penalty']:
          phrase_penalty = b' 2.718'
        else:
          phrase_penalty = b''
        line = b"%s ||| %s ||| %s%s %s||| %s%s||| %s\n" %(src,target,features,origin_features,phrase_penalty,alignment,extra_space,comments)
        return line
        
        

    def write_lexical_file(self,direction, path, weights,mode):
        
        if mode == 'counts':
            bridge = '.counts'
        else:
            bridge = ''
        
        fobj = handle_file("{0}{1}.{2}".format(path,bridge,direction),'open',mode='w')
        sys.stderr.write('Writing {0}{1}.{2}\n'.format(path,bridge,direction))
    
        if direction == 'e2f':
            word_pairs = self.word_pairs_e2f
            marginal = self.word_target
        
        elif direction == 'f2e':
            word_pairs = self.word_pairs_f2e
            marginal = self.word_source
    
        for x in sorted(word_pairs):
            for y in sorted(word_pairs[x]):
                xy = dot_product(word_pairs[x][y],weights)
                fobj.write(b"%s %s %s" %(x,y,xy))
    
                if mode == 'counts':
                    fobj.write(b" %s\n" %(dot_product(marginal[y],weights)))
                else:
                    fobj.write(b'\n')

        handle_file("{0}{1}.{2}".format(path,bridge,direction),'close',fobj,mode='w')
        


    def write_reordering_table(self,src,target,features):
        """convert data to string in Moses reordering table format"""
        
        # if one feature value is 0 (either because of loglinear interpolation or rounding to 0), don't write it to reordering table
        # (phrase pair will end up with probability zero in log-linear model anyway)
        if 0 in features:
            return ''
        
        features = b' '.join([b'%.6g' %(f) for f in features])
        
        line = b"%s ||| %s ||| %s\n" %(src,target,features)
        return line


    def create_inverse(self,fobj,tempdir=None):
        """swap source and target phrase in the phrase table, and then sort (by target phrase)"""
        
        inverse = NamedTemporaryFile(prefix='inv_unsorted',delete=False,dir=tempdir)        
        swap = re.compile(b'(.+?) \|\|\| (.+?) \|\|\|')
        
        # just swap source and target phrase, and leave order of scores etc. intact. 
        # For better compatibility with existing codebase, we swap the order of the phrases back for processing
        for line in fobj:
            inverse.write(swap.sub(b'\\2 ||| \\1 |||',line,1))
        inverse.close()
        
        inverse_sorted = sort_file(inverse.name,tempdir=tempdir)
        os.remove(inverse.name)
        
        return inverse_sorted


    def merge(self,pt_normal, pt_inverse, pt_out, mode='interpolate'):
        """merge two phrasetables (the latter having been inverted to calculate p(s|t) and lex(s|t) in sorted order)
           Assumes that p(s|t) and lex(s|t) are in first table half, p(t|s) and lex(t|s) in second"""
        
        for line,line2 in izip(pt_normal,pt_inverse):
            
            line = line.split(b' ||| ')
            line2 = line2.split(b' ||| ')
            
            #scores
            mid = int(self.number_of_features/2)
            scores1 = line[2].split()
            scores2 = line2[2].split()
            line[2] = b' '.join(scores2[:mid]+scores1[mid:])
            
            # marginal counts
            if mode == 'counts':
                src_count = line[-1].split()[1]
                target_count = line2[-1].split()[0]
                line[-1] = b' '.join([target_count,src_count]) + b'\n'
            
            pt_out.write(b' ||| '.join(line))
            
        pt_normal.close()
        pt_inverse.close()
        pt_out.close()



class TigerXML():
    """interface to load reference word alignments from TigerXML corpus.
       Tested on SMULTRON (http://kitt.cl.uzh.ch/kitt/smultron/)
    """
    
    def __init__(self,alignment_xml):
        """only argument is TigerXML file
        """
        
        self.treebanks = self._open_treebanks(alignment_xml)
        self.word_pairs = defaultdict(lambda: defaultdict(int))
        self.word_source = defaultdict(int)
        self.word_target = defaultdict(int)
        

    def load_word_pairs(self,src,target):
        """load word pairs. src and target are the itentifiers of the source and target language in the XML"""
        
        if not src or not target:
            sys.stderr.write('Error: Source and/or target language not specified. Required for TigerXML extraction.\n')
            exit()
        
        alignments = self._get_aligned_ids(src,target)
        self._textualize_alignments(src,target,alignments)
        

    def _open_treebanks(self,alignment_xml):
        """Parallel XML format references monolingual files. Open all."""
        
        alignment_path = os.path.dirname(alignment_xml)
        align_xml = ET.parse(alignment_xml)

        treebanks = {}
        treebanks['aligned'] = align_xml
        
        for treebank in align_xml.findall('//treebank'):
            treebank_id = treebank.get('id')
            filename = treebank.get('filename')
            
            if not os.path.isabs(filename):
                filename = os.path.join(alignment_path,filename)
        
            treebanks[treebank_id] = ET.parse(filename)  

        return treebanks
    
    
    def _get_aligned_ids(self,src,target):
        """first step: find which nodes are aligned."""
        
    
        alignments = []
        ids = defaultdict(dict)
        
        for alignment in self.treebanks['aligned'].findall('//align'):
            
            newpair = {}
            
            if len(alignment) != 2:
                sys.stderr.write('Error: alignment with ' + str(len(alignment)) + ' children. Expected 2. Skipping.\n')
                continue
                
            for node in alignment:
                lang = node.get('treebank_id')
                node_id = node.get('node_id')
                newpair[lang] = node_id

            if not (src in newpair and target in newpair):
                sys.stderr.write('Error: source and target languages don\'t match. Skipping.\n')
                continue
                
            # every token may only appear in one alignment pair;
            # if it occurs in multiple, we interpret them as one 1-to-many or many-to-1 alignment
            if newpair[src] in ids[src]:
                idx = ids[src][newpair[src]]
                alignments[idx][1].append(newpair[target])
                
            elif newpair[target] in ids[target]:
                idx = ids[target][newpair[target]]
                alignments[idx][0].append(newpair[src])
                
            else:
                idx = len(alignments)
                alignments.append(([newpair[src]],[newpair[target]]))
                ids[src][newpair[src]] = idx
                ids[target][newpair[target]] = idx
                
        alignments = self._discard_discontinuous(alignments)
                
        return alignments

    
    def _discard_discontinuous(self,alignments):
        """discard discontinuous word sequences (which we can't use for phrase-based SMT systems)
           and make sure that sequence is in correct order.
        """
        
        new_alignments = []
        
        for alignment in alignments:
            new_pair = []
            
            for sequence in alignment:
                
                sequence_split = [t_id.split('_') for t_id in sequence]
                
                #check if all words come from the same sentence
                sentences = [item[0] for item in sequence_split]
                if not len(set(sentences)) == 1:
                    #sys.stderr.write('Warning. Word sequence crossing sentence boundary. Discarding.\n')
                    #sys.stderr.write(str(sequence)+'\n')
                    continue
                
                
                #sort words and check for discontinuities.
                try:
                    tokens = sorted([int(item[1]) for item in sequence_split])
                except ValueError:
                    #sys.stderr.write('Warning. Not valid word IDs. Discarding.\n')
                    #sys.stderr.write(str(sequence)+'\n')
                    continue
                    
                if not tokens[-1]-tokens[0] == len(tokens)-1:
                    #sys.stderr.write('Warning. Discontinuous word sequence(?). Discarding.\n')
                    #sys.stderr.write(str(sequence)+'\n')
                    continue
                
                out_sequence = [sentences[0]+'_'+str(token) for token in tokens]
                new_pair.append(out_sequence)
                
            if len(new_pair) == 2:
                new_alignments.append(new_pair)
                
        return new_alignments

    
    def _textualize_alignments(self,src,target,alignments):
        """Knowing which nodes are aligned, get actual words that are aligned."""
        
        words = defaultdict(dict)
        
        for text in [text for text in self.treebanks if not text == 'aligned']:
        
            #TODO: Make lowercasing optional
            for terminal in self.treebanks[text].findall('//t'):
                words[text][terminal.get('id')] = terminal.get('word').lower()
        
        
        for (src_ids, target_ids) in alignments:
            
            try:
                src_text = ' '.join((words[src][src_id] for src_id in src_ids))
            except KeyError:
                #sys.stderr.write('Warning. ID not found: '+ str(src_ids) +'\n')
                continue
                
            try:
                target_text = ' '.join((words[target][target_id] for target_id in target_ids))
            except KeyError:
                #sys.stderr.write('Warning. ID not found: '+ str(target_ids) +'\n')
                continue
            
            self.word_pairs[src_text][target_text] += 1
            self.word_source[src_text] += 1
            self.word_target[target_text] += 1



class Moses_Alignment():
    """interface to load reference phrase alignment from corpus aligend with Giza++
       and with extraction heuristics as applied by the Moses toolkit.
    
    """
    
    def __init__(self,alignment_file):
        
        self.alignment_file = alignment_file
        self.word_pairs = defaultdict(lambda: defaultdict(int))
        self.word_source = defaultdict(int)
        self.word_target = defaultdict(int)
        
        
    def load_word_pairs(self,src_lang,target_lang):
        """main function. overwrite this to import data in different format."""
        
        fileobj = handle_file(self.alignment_file,'open','r')
        
        for line in fileobj:
            
            line = line.split(b' ||| ')
            
            src = line[0]
            target = line[1]
            
            self.word_pairs[src][target] += 1
            self.word_source[src] += 1
            self.word_target[target] += 1
        

def dot_product(a,b):
    """calculate dot product from two lists"""
    
    # optimized for PyPy (much faster than enumerate/map)
    s = 0
    i = 0
    for x in a:
        s += x * b[i]
        i += 1
        
    return s
    

def priority_sort_models(models):
    """primary models should have priority before supplementary models.
       zipped with index to know which weight model belongs to
    """

    return [(model,priority,i) for (i,(model,priority)) in sorted(zip(range(len(models)),models),key=lambda x: x[1][1])]


def cross_entropy(model_interface,reference_interface,weights,score,mode,flags):
    """calculate cross entropy given all necessary information.
       don't call this directly, but use one of the Combine_TMs methods.
    """

    weights = normalize_weights(weights,mode,flags)
    
    if 'compare_cross-entropies' in flags and flags['compare_cross-entropies']:
        num_results = len(model_interface.models)
    else:
        num_results = 1
    
    cross_entropies = [[0]*num_results for i in range(model_interface.number_of_features)]
    oov = [0]*num_results
    oov2 = 0
    other_translations = [0]*num_results
    ignored = [0]*num_results
    n = [0]*num_results
    total_pairs = 0
    
    for src in reference_interface.word_pairs:
        for target in reference_interface.word_pairs[src]:
            
            c = reference_interface.word_pairs[src][target]
            
            for i in range(num_results):
                if src in model_interface.phrase_pairs and target in model_interface.phrase_pairs[src]:

                    if ('compare_cross-entropies' in flags and flags['compare_cross-entropies']) or ('intersected_cross-entropies' in flags and flags['intersected_cross-entropies']):
                        
                        if 0 in model_interface.phrase_pairs[src][target][0][0]: #only use intersection of models for comparability
                        
                            # update unknown words statistics
                            if model_interface.phrase_pairs[src][target][0][0][i]:
                                ignored[i] += c
                            elif src in model_interface.phrase_source and model_interface.phrase_source[src][i]:
                                other_translations[i] += c
                            else:
                                oov[i] += c
                                
                            continue
                        
                        if ('compare_cross-entropies' in flags and flags['compare_cross-entropies']):
                            tmp_weights = [[0]*i+[1]+[0]*(num_results-i-1)]*model_interface.number_of_features
                        elif ('intersected_cross-entropies' in flags and flags['intersected_cross-entropies']):
                            tmp_weights = weights
                            
                        features = score(tmp_weights,src,target,model_interface,flags)
  
                    else:
                        features = score(weights,src,target,model_interface,flags)
                    
                    #if weight is so low that feature gets probability zero
                    if 0 in features:
                        #sys.stderr.write('Warning: 0 probability in model {0}: source phrase: {1!r}; target phrase: {2!r}\n'.format(i,src,target))
                        #sys.stderr.write('Possible reasons: 0 probability in phrase table; very low (or 0) weight; recompute lexweight and different alignments\n')
                        #sys.stderr.write('Phrase pair is ignored for cross_entropy calculation\n\n')
                        continue
                        
                    n[i] += c
                    for j in range(model_interface.number_of_features):
                        cross_entropies[j][i] -= log(features[j],2)*c

                elif src in model_interface.phrase_source and not ('compare_cross-entropies' in flags and flags['compare_cross-entropies']):
                    other_translations[i] += c
                        
                else:
                    oov2 += c

            total_pairs += c
            

    oov2 = int(oov2/num_results)
    
    for i in range(num_results):
        try:
            for j in range(model_interface.number_of_features):
                cross_entropies[j][i] /= n[i]
        except ZeroDivisionError:
            sys.stderr.write('Warning: no matching phrase pairs between reference set and model\n')
            for j in range(model_interface.number_of_features):
                cross_entropies[j][i] = 0


    if 'compare_cross-entropies' in flags and flags['compare_cross-entropies']:
        return [tuple([ce[i] for ce in cross_entropies]) + (other_translations[i],oov[i],ignored[i],n[i],total_pairs) for i in range(num_results)], (n[0],total_pairs,oov2)
    else:
        return tuple([ce[0] for ce in cross_entropies]) + (other_translations[0],oov2,total_pairs)


def cross_entropy_light(model_interface,reference_interface,weights,score,mode,flags,cache):
    """calculate cross entropy given all necessary information.
       don't call this directly, but use one of the Combine_TMs methods.
       Same as cross_entropy, but optimized for speed: it doesn't generate all of the statistics,
       doesn't normalize, and uses caching.
    """
    weights = normalize_weights(weights,mode,flags)
    cross_entropies = [0]*model_interface.number_of_features

    for (src,target,c) in cache:
        features = score(weights,src,target,model_interface,flags,cache=True)

        if 0 in features:
            #sys.stderr.write('Warning: 0 probability in model {0}: source phrase: {1!r}; target phrase: {2!r}\n'.format(i,src,target))
            #sys.stderr.write('Possible reasons: 0 probability in phrase table; very low (or 0) weight; recompute lexweight and different alignments\n')
            #sys.stderr.write('Phrase pair is ignored for cross_entropy calculation\n\n')
            continue

        for i in range(model_interface.number_of_features):
            cross_entropies[i] -= log(features[i],2)*c

    return cross_entropies


def _get_reference_cache(reference_interface,model_interface):
    """creates a data structure that allows for a quick access 
       to all relevant reference set phrase/word pairs and their frequencies.
    """
    cache = []
    n = 0

    for src in reference_interface.word_pairs:
        for target in reference_interface.word_pairs[src]:
            if src in model_interface.phrase_pairs and target in model_interface.phrase_pairs[src]:
                c = reference_interface.word_pairs[src][target]
                cache.append((src,target,c))
                n += c

    return cache,n


def _get_lexical_filter(reference_interface,model_interface):
    """returns dictionaries that store the words and word pairs needed
       for perplexity optimization. We can use these dicts to load fewer data into memory for optimization."""
    
    e2f_filter = defaultdict(set)
    f2e_filter = defaultdict(set)
    
    for src in reference_interface.word_pairs:
        for target in reference_interface.word_pairs[src]:
            if src in model_interface.phrase_pairs and target in model_interface.phrase_pairs[src]:
                e2f_alignment,f2e_alignment = model_interface.get_word_alignments(src,target)
                
                for s,t_list in e2f_alignment:
                    for t in t_list:
                        e2f_filter[s].add(t)
                
                for t,s_list in f2e_alignment:
                    for s in s_list:
                        f2e_filter[t].add(s)
                        
    return e2f_filter,f2e_filter


def _hillclimb_move(weights,stepsize,mode,flags):
    """Move function for hillclimb algorithm. Updates each weight by stepsize."""

    for i,w in enumerate(weights):
        yield normalize_weights(weights[:i]+[w+stepsize]+weights[i+1:],mode,flags)

    for i,w in enumerate(weights):
        new = w-stepsize
        if new >= 1e-10:
            yield normalize_weights(weights[:i]+[new]+weights[i+1:],mode,flags)

def _hillclimb(scores,best_weights,objective,model_interface,reference_interface,score_function,mode,flags,precision,cache,n):
    """first (deprecated) implementation of iterative weight optimization."""
    
    best = objective(best_weights)
    
    i = 0 #counts number of iterations with same stepsize: if greater than 10, it is doubled
    stepsize = 512 # initial stepsize
    move = 1 #whether we found a better set of weights in the current iteration. if not, it is halfed
    sys.stderr.write('Hillclimb: step size: ' + str(stepsize))
    while stepsize > 0.0078:
        
        if not move:
            stepsize /= 2
            sys.stderr.write(' ' + str(stepsize))
            i = 0
            move = 1
            continue
            
        move = 0
        
        for w in _hillclimb_move(list(best_weights),stepsize,mode,flags):
            weights_tuple = tuple(w)
            
            if weights_tuple in scores:
                continue

            scores[weights_tuple] = cross_entropy_light(model_interface,reference_interface,[w for m in range(model_interface.number_of_features)],score_function,mode,flags,cache)
            
            if objective(weights_tuple)+precision < best:
                best = objective(weights_tuple)
                best_weights = weights_tuple
                move = 1
                
        if i and not i % 10:
            sys.stderr.write('\nIteration '+ str(i) + ' with stepsize ' + str(stepsize) + '. current cross-entropy: ' + str(best) + '- weights: ' + str(best_weights) + ' ')
            stepsize *= 2
            sys.stderr.write('\nIncreasing stepsize: '+ str(stepsize))
            i = 0
            
        i += 1
    
    return best_weights


def optimize_cross_entropy_hillclimb(model_interface,reference_interface,initial_weights,score_function,mode,flags,precision=0.000001):
    """find weights that minimize cross-entropy on a tuning set
       deprecated (default is now L-BFGS (optimize_cross_entropy)), but left in for people without SciPy
    """
    
    scores = {}

    best_weights = tuple(initial_weights[0])
    
    cache,n = _get_reference_cache(reference_interface,model_interface)
    
    # each objective is a triple: which score to minimize from cross_entropy(), which weights to update accordingly, and a comment that is printed
    objectives = [(lambda x: scores[x][i]/n,[i],'minimize cross-entropy for feature {0}'.format(i)) for i in range(model_interface.number_of_features)]
    
    scores[best_weights] = cross_entropy_light(model_interface,reference_interface,initial_weights,score_function,mode,flags,cache)
    final_weights = initial_weights[:]
    final_cross_entropy = [0]*model_interface.number_of_features
    
    for i,(objective, features, comment) in enumerate(objectives):
        best_weights = min(scores,key=objective)
        sys.stderr.write('Optimizing objective "' + comment +'"\n')
        best_weights = _hillclimb(scores,best_weights,objective,model_interface,reference_interface,score_function,feature_specific_mode(mode,i,flags),flags,precision,cache,n)
        
        sys.stderr.write('\nCross-entropy:' + str(objective(best_weights)) + ' - weights: ' + str(best_weights)+'\n\n')
        
        for j in features:
            final_weights[j] = list(best_weights)
            final_cross_entropy[j] = objective(best_weights)

    return final_weights,final_cross_entropy


def optimize_cross_entropy(model_interface,reference_interface,initial_weights,score_function,mode,flags):
    """find weights that minimize cross-entropy on a tuning set
       Uses L-BFGS optimization and requires SciPy
    """
    
    if not optimizer == 'l-bfgs':
        sys.stderr.write('SciPy is not installed. Falling back to naive hillclimb optimization (instead of L-BFGS)\n')
        return optimize_cross_entropy_hillclimb(model_interface,reference_interface,initial_weights,score_function,mode,flags)
    
    cache,n = _get_reference_cache(reference_interface,model_interface)
    
    # each objective is a triple: which score to minimize from cross_entropy(), which weights to update accordingly, and a comment that is printed
    objectives = [(lambda w: cross_entropy_light(model_interface,reference_interface,[[1]+list(w) for m in range(model_interface.number_of_features)],score_function,feature_specific_mode(mode,i,flags),flags,cache)[i],[i],'minimize cross-entropy for feature {0}'.format(i)) for i in range(model_interface.number_of_features)] #optimize cross-entropy for p(s|t)

    final_weights = initial_weights[:]
    final_cross_entropy = [0]*model_interface.number_of_features
    
    for i,(objective, features, comment) in enumerate(objectives):
        sys.stderr.write('Optimizing objective "' + comment +'"\n')
        initial_values = [1]*(len(model_interface.models)-1) # we leave value of first model at 1 and optimize all others (normalized of course)
        best_weights, best_point, data = fmin_l_bfgs_b(objective,initial_values,approx_grad=True,bounds=[(0.000000001,None)]*len(initial_values))
        best_weights = normalize_weights([1]+list(best_weights),feature_specific_mode(mode,i,flags),flags)
        sys.stderr.write('Cross-entropy after L-BFGS optimization: ' + str(best_point/n) + ' - weights: ' + str(best_weights)+'\n')
    
        for j in features:
            final_weights[j] = list(best_weights)
            final_cross_entropy[j] = best_point/n

    return final_weights,final_cross_entropy


def feature_specific_mode(mode,i,flags):
    """in mode 'counts', only the default Moses features can be recomputed from raw frequencies;
       all other features are interpolated by default. 
       This fucntion mostly serves optical purposes (i.e. normalizing a single weight vector for logging),
       since normalize_weights also handles a mix of interpolated and recomputed features.
    """
    
    if mode == 'counts' and i not in [flags['i_e2f'],flags['i_e2f_lex'],flags['i_f2e'],flags['i_f2e_lex']]:
        return 'interpolate'
    else:
        return mode


def redistribute_probability_mass(weights,src,target,interface,flags,mode='interpolate'):
    """the conditional probability p(x|y) is undefined for cases where p(y) = 0
       this function redistributes the probability mass to only consider models for which p(y) > 0
    """

    i_e2f = flags['i_e2f']
    i_e2f_lex = flags['i_e2f_lex']
    i_f2e = flags['i_f2e']
    i_f2e_lex = flags['i_f2e_lex']

    new_weights = weights[:]
    
    if flags['normalize_s_given_t'] == 's':
    
        # set weight to 0 for all models where target phrase is unseen (p(s|t)   
        new_weights[i_e2f] = map(mul,interface.phrase_source[src],weights[i_e2f])
        if flags['normalize-lexical_weights']:
            new_weights[i_e2f_lex] = map(mul,interface.phrase_source[src],weights[i_e2f_lex])
        
    elif flags['normalize_s_given_t'] == 't':
        
        # set weight to 0 for all models where target phrase is unseen (p(s|t)   
        new_weights[i_e2f] = map(mul,interface.phrase_target[target],weights[i_e2f])
        if flags['normalize-lexical_weights']:
            new_weights[i_e2f_lex] = map(mul,interface.phrase_target[target],weights[i_e2f_lex])

    # set weight to 0 for all models where source phrase is unseen (p(t|s)
    new_weights[i_f2e] = map(mul,interface.phrase_source[src],weights[i_f2e])
    if flags['normalize-lexical_weights']:
        new_weights[i_f2e_lex] = map(mul,interface.phrase_source[src],weights[i_f2e_lex])
        
    
    return normalize_weights(new_weights,mode,flags)


def score_interpolate(weights,src,target,interface,flags,cache=False):
    """linear interpolation of probabilites (and other feature values)
       if normalized is True, the probability mass for p(x|y) is redistributed to models with p(y) > 0
    """

    model_values = interface.phrase_pairs[src][target][0]
    
    scores = [0]*len(model_values)
    
    if 'normalized' in flags and flags['normalized']:
        normalized_weights = redistribute_probability_mass(weights,src,target,interface,flags)
    else:
        normalized_weights = weights

    if 'recompute_lexweights' in flags and flags['recompute_lexweights']:
        e2f_alignment,f2e_alignment = interface.get_word_alignments(src,target,cache=cache)
        
        if not e2f_alignment or not f2e_alignment:
            sys.stderr.write('Error: no word alignments found, but necessary for lexical weight computation.\n')
            lst = 0
            lts = 0
            
        else:
            scores[flags['i_e2f_lex']] = compute_lexicalweight(normalized_weights[flags['i_e2f_lex']],e2f_alignment,interface.word_pairs_e2f,None,mode='interpolate')
            scores[flags['i_f2e_lex']] = compute_lexicalweight(normalized_weights[flags['i_f2e_lex']],f2e_alignment,interface.word_pairs_f2e,None,mode='interpolate')

    
    for idx,prob in enumerate(model_values):
        if not ('recompute_lexweights' in flags and flags['recompute_lexweights'] and (idx == flags['i_e2f_lex'] or idx == flags['i_f2e_lex'])):
            scores[idx] = dot_product(prob,normalized_weights[idx])
    
    return scores


def score_loglinear(weights,src,target,interface,flags,cache=False):
    """loglinear interpolation of probabilites
       warning: if phrase pair does not occur in all models, resulting probability is 0
       this is usually not what you want - loglinear scoring is only included for completeness' sake
    """
    
    scores = []
    model_values = interface.phrase_pairs[src][target][0]

    for idx,prob in enumerate(model_values):
        try:
            scores.append(exp(dot_product(map(log,prob),weights[idx])))
        except ValueError:
            scores.append(0)
    
    return scores


def score_counts(weights,src,target,interface,flags,cache=False):
    """count-based re-estimation of probabilites and lexical weights
       each count is multiplied by its weight; trivial case is weight 1 for each model, which corresponds to a concatentation
    """
    
    i_e2f = flags['i_e2f']
    i_e2f_lex = flags['i_e2f_lex']
    i_f2e = flags['i_f2e']
    i_f2e_lex = flags['i_f2e_lex']
    
    # if we have non-default number of weights, assume that we might have to do a mix of count-based and interpolated scores.
    if len(weights) == 4:
        scores = [0]*len(weights)
    else:
        scores = score_interpolate(weights,src,target,interface,flags,cache=cache)
    
    try:
        joined_count = dot_product(interface.phrase_pairs[src][target][0][i_e2f],weights[i_e2f])
        target_count = dot_product(interface.phrase_target[target],weights[i_e2f])
        scores[i_e2f] = joined_count / target_count
    except ZeroDivisionError:
        scores[i_e2f] = 0

    try:
        joined_count = dot_product(interface.phrase_pairs[src][target][0][i_f2e],weights[i_f2e])
        source_count = dot_product(interface.phrase_source[src],weights[i_f2e])
        scores[i_f2e] = joined_count / source_count
    except ZeroDivisionError:
        scores[i_f2e] = 0
    
    e2f_alignment,f2e_alignment = interface.get_word_alignments(src,target,cache=cache)
    
    if not e2f_alignment or not f2e_alignment:
        sys.stderr.write('Error: no word alignments found, but necessary for lexical weight computation.\n')
        scores[i_e2f_lex] = 0
        scores[i_f2e_lex] = 0
    
    else:
        scores[i_e2f_lex] = compute_lexicalweight(weights[i_e2f_lex],e2f_alignment,interface.word_pairs_e2f,interface.word_target,mode='counts',cache=cache)
        scores[i_f2e_lex] = compute_lexicalweight(weights[i_f2e_lex],f2e_alignment,interface.word_pairs_f2e,interface.word_source,mode='counts',cache=cache)
    
    return scores


def score_interpolate_reordering(weights,src,target,interface):
    """linear interpolation of reordering model probabilities
       also normalizes model so that 
    """
    
    model_values = interface.reordering_pairs[src][target]
    
    scores = [0]*len(model_values)
    
    for idx,prob in enumerate(model_values):
            scores[idx] = dot_product(prob,weights[idx])
    
    #normalizes first half and last half probabilities (so that each half sums to one). 
    #only makes sense for bidirectional configuration in Moses. Remove/change this if you want a different (or no) normalization
    scores = normalize_weights(scores[:int(interface.number_of_features/2)],'interpolate') + normalize_weights(scores[int(interface.number_of_features/2):],'interpolate')
    
    return scores


def compute_lexicalweight(weights,alignment,word_pairs,marginal,mode='counts',cache=False,mycache=[0,defaultdict(dict)]):
    """compute the lexical weights as implemented in Moses toolkit"""
    
    lex = 1
    
    # new weights: empty cache
    if cache and mycache[0] != weights:
        mycache[0] = weights
        mycache[1] = defaultdict(dict)
    
    for x,translations in alignment:
        
        if cache and translations in mycache[1][x]:
            lex_step = mycache[1][x][translations]
        
        else:
            lex_step = 0
            for y in translations:
                
                if mode == 'counts':
                    lex_step += dot_product(word_pairs[x][y],weights) / dot_product(marginal[y],weights)
                elif mode == 'interpolate':
                    lex_step += dot_product(word_pairs[x][y],weights)
            
            lex_step /= len(translations)
            
            if cache:
                mycache[1][x][translations] = lex_step
            
        lex *= lex_step
        
    return lex


def normalize_weights(weights,mode,flags=None):
    """make sure that probability mass in linear interpolation is 1
       for weighted counts, weight of first model is set to 1
    """

    if mode == 'interpolate' or mode == 'loglinear':

        if type(weights[0]) == list:
                
            new_weights = []
            
            for weight_list in weights:
                total = sum(weight_list)
                
                try:
                    weight_list = [weight/total for weight in weight_list]
                except ZeroDivisionError:
                    sys.stderr.write('Error: Zero division in weight normalization. Are some of your weights zero? This might lead to undefined behaviour if a phrase pair is only seen in model with weight 0\n')
                    
                new_weights.append(weight_list)

        else:
            total = sum(weights)
                
            try:
                new_weights = [weight/total for weight in weights]
            except ZeroDivisionError:
                sys.stderr.write('Error: Zero division in weight normalization. Are some of your weights zero? This might lead to undefined behaviour if a phrase pair is only seen in model with weight 0\n')

    elif mode == 'counts_pure':
        
        if type(weights[0]) == list:
                
            new_weights = []
            
            for weight_list in weights:
                ratio = 1/weight_list[0]
                new_weights.append([weight * ratio for weight in weight_list])
       
        else:
            ratio = 1/weights[0]
            new_weights = [weight * ratio for weight in weights]

    # make sure that features other than the standard Moses features are always interpolated (since no count-based computation is defined)
    elif mode == 'counts': 
        
        if type(weights[0]) == list:
                norm_counts = normalize_weights(weights,'counts_pure')
                new_weights = normalize_weights(weights,'interpolate')
                for i in [flags['i_e2f'],flags['i_e2f_lex'],flags['i_f2e'],flags['i_f2e_lex']]:
                    new_weights[i] = norm_counts[i]
                return new_weights
        
        else:
            return normalize_weights(weights,'counts_pure')

    return new_weights


def handle_file(filename,action,fileobj=None,mode='r'):
    """support reading/writing either from/to file, stdout or gzipped file"""

    if action == 'open':

        if mode == 'r':
            mode = 'rb'

        if mode == 'rb' and not filename == '-' and not os.path.exists(filename):
            if os.path.exists(filename+'.gz'):
                filename = filename+'.gz'
            else:
                sys.stderr.write('Error: unable to open file. ' + filename + ' - aborting.\n')
                
                if 'counts' in filename and os.path.exists(os.path.dirname(filename)):
                    sys.stderr.write('For a weighted counts combination, we need statistics that Moses doesn\'t write to disk by default.\n')
                    sys.stderr.write('Repeat step 4 of Moses training for all models with the option -write-lexical-counts.\n')
                
                exit()

        if filename.endswith('.gz'):
            fileobj = gzip.open(filename,mode)
            
        elif filename == '-' and mode == 'w':
            fileobj = sys.stdout
                    
        else:
                fileobj = open(filename,mode)
        
        return fileobj
    
    elif action == 'close' and filename != '-':
        fileobj.close()   


def sort_file(filename,tempdir=None):
    """Sort a file and return temporary file"""

    cmd = ['sort', filename]
    env = {}
    env['LC_ALL'] = 'C'
    if tempdir:
        cmd.extend(['-T',tempdir])
    
    outfile = NamedTemporaryFile(delete=False,dir=tempdir)
    sys.stderr.write('LC_ALL=C ' + ' '.join(cmd) + ' > ' + outfile.name + '\n')
    p = Popen(cmd,env=env,stdout=outfile.file)
    p.wait()
    
    outfile.seek(0)

    return outfile
        

class Combine_TMs():
    
    """This class handles the various options, checks them for sanity and has methods that define what models to load and what functions to call for the different tasks.
       Typically, you only need to interact with this class and its attributes.
    
    """
    
    #some flags that change the behaviour during scoring. See init docstring for more info
    flags = {'normalized':False, 
            'recompute_lexweights':False, 
            'intersected_cross-entropies':False, 
            'normalize_s_given_t':None, 
            'normalize-lexical_weights':True, 
            'add_origin_features':False,
            'write_phrase_penalty':False,
            'lowmem': False,
            'i_e2f':0,
            'i_e2f_lex':1,
            'i_f2e':2,
            'i_f2e_lex':3
            }

    # each model needs a priority. See init docstring for more info
    _priorities = {'primary':1,
                    'map':2,
                    'supplementary':10}
    
    def __init__(self,models,weights=None,
                      output_file=None,
                      mode='interpolate',
                      number_of_features=4,
                      model_interface=Moses,
                      reference_interface=Moses_Alignment,
                      reference_file=None,
                      lang_src=None,
                      lang_target=None,
                      output_lexical=None,
                      **flags):
        """The whole configuration of the task is done during intialization. Afterwards, you only need to call your intended method(s).
           You can change some of the class attributes afterwards (such as the weights, or the output file), but you should never change the models or mode after initialization.
           See unit_test function for example configurations
           
           models: list of tuples (path,priority) that defines which models to process. Path is usually the top directory of a Moses model. There are three priorities:
                    'primary': phrase pairs with this priority will always be included in output model. For most purposes, you'll want to define all models as primary.
                    'map': for maximum a-posteriori combination (Bacchiani et al. 2004; Foster et al. 2010). for use with mode 'counts'. stores c(t) = 1 and c(s,t) = p(s|t)
                    'supplementary': phrase pairs are considered for probability computation, but not included in output model (unless they also occur in at least one primary model)
                                     useful for rescoring a model without changing its vocabulary.

           weights: accept two types of weight declarations: one weight per model, and one weight per model and feature
                    type one is internally converted to type two. For 2 models with four features, this looks like: [0.1,0.9] -> [[0.1,0.9],[0.1,0.9],[0.1,0.9],[0.1,0.9]]
                    default: uniform weights (None)
                    
           output_file: filepath of output phrase table. If it ends with .gz, file is automatically zipped.
           
           output_lexical: If defined, also writes combined lexical tables. Writes to output_lexical.e2f and output_lexical.f2e, or output_lexical.counts.e2f in mode 'counts'.

           mode: declares the basic mixture-model algorithm. there are currently three options:
                 'counts': weighted counts (requires some statistics that Moses doesn't produce. Repeat step 4 of Moses training with the option -write-lexical-counts to obtain them.)
                           Only the standard Moses features are recomputed from weighted counts; additional features are linearly interpolated 
                           (see number_of_features to allow more features, and i_e2f etc. if the standard features are in a non-standard position)
                 'interpolate': linear interpolation
                 'loglinear': loglinear interpolation (careful: this creates the intersection of phrase tables and is often of little use)
                 
           number_of_features: could be used to interpolate models with non-default Moses features. 4 features is currently still hardcoded in various places 
                               (e.g. cross_entropy calculations, mode 'counts')
           
           i_e2f,i_e2f_lex,i_f2e,i_f2e_lex: Index of the (Moses) phrase table features p(s|t), lex(s|t), p(t|s) and lex(t|s). 
                Relevant for mode 'counts', and if 'recompute_lexweights' is True in mode 'interpolate'. In mode 'counts', any additional features are combined through linear interpolation.
           
           model_interface: class that handles reading phrase tables and lexical tables, and writing phrase tables. Currently only Moses is implemented.
                 default: Moses
           
           reference_interace: class that deals with reading in reference phrase pairs for cross-entropy computation
                Moses_Alignment: Word/phrase pairs as computed by Giza++ and extracted through Moses heuristics. This corresponds to the file model/extract.gz if you train a Moses model on your tuning set.
                TigerXML: TigerXML data format
                
                default: Moses_Alignment
           
           reference_file: path to reference file. Required for every operation except combination of models with given weights.
           
           lang_src: source language. Only required if reference_interface is TigerXML. Identifies which language in XML file we should treat as source language.
           
           lang_target: target language.  Only required if reference_interface is TigerXML. Identifies which language in XML file we should treat as target language.

           intersected_cross-entropies: compute cross-entropies of intersection of phrase pairs, ignoring phrase pairs that do not occur in all models.
               If False, algorithm operates on union of phrase pairs
               default: False
               
           add_origin_features: For each model that is being combined, add a binary feature to the final phrase table, with values of 1 (phrase pair doesn't occur in model) and 2.718 (it does).
                                This indicates which model(s) a phrase pair comes from and can be used during MERT to additionally reward/penalize translation models

           lowmem: low memory mode: instead of loading target phrase counts / probability (when required), process the original table and its inversion (source and target swapped) incrementally, then merge the two halves.

           tempdir: temporary directory (for low memory mode).

           there are a number of further configuration options that you can define, which modify the algorithm for linear interpolation. They have no effect in mode 'counts'

                recompute_lexweights: don't directly interpolate lexical weights, but interpolate word translation probabilities instead and recompute the lexical weights.
                            default: False

                normalized: for interpolation of p(x|y): if True, models with p(y)=0 will be ignored, and probability mass will be distributed among models with p(y)>0. 
                            If False, missing entries (x,y) are always interpreted as p(x|y)=0.
                            default: False
                
                normalize_s_given_t: How to we normalize p(s|t) if 'normalized' is True? Three options:
                            None: don't normalize p(s|t) and lex(s|t) (only p(t|s) and lex(t|s))
                            t: check if p(t)==0 : advantage: theoretically sound; disadvantage: slower (we need to know if t occcurs in model); favours rare target phrases (relative to default choice)
                            s: check if p(s)==0 : advantage: relevant for task; disadvantage: no true probability distributions
                            
                            default: None
                            
                normalize-lexical_weights: also normalize lex(s|t) and lex(t|s) if 'normalized' ist True: 
                            reason why you might want to disable this: lexical weights suffer less from data sparseness than probabilities.
                            default: True
           
        """
        
        
        self.mode = mode
        self.output_file = output_file
        self.lang_src = lang_src
        self.lang_target = lang_target
        self.loaded = defaultdict(int)
        self.output_lexical = output_lexical

        self.flags = copy.copy(self.flags)
        self.flags.update(flags)
        
        self.flags['i_e2f'] = int(self.flags['i_e2f'])
        self.flags['i_e2f_lex'] = int(self.flags['i_e2f_lex'])
        self.flags['i_f2e'] = int(self.flags['i_f2e'])
        self.flags['i_f2e_lex'] = int(self.flags['i_f2e_lex'])

        if reference_interface:
            self.reference_interface = reference_interface(reference_file)

        if mode not in ['interpolate','loglinear','counts']:
            sys.stderr.write('Error: mode must be either "interpolate", "loglinear" or "counts"\n')
            sys.exit()

        models,number_of_features,weights = self._sanity_checks(models,number_of_features,weights)
        
        self.weights = weights
        self.models = models

        self.model_interface = model_interface(models,number_of_features)

        if mode == 'interpolate':
            self.score = score_interpolate
        elif mode == 'loglinear':
            self.score = score_loglinear
        elif mode == 'counts':
            self.score = score_counts
            

    def _sanity_checks(self,models,number_of_features,weights):
        """check if input arguments make sense (correct number of weights, valid model priorities etc.)
           is only called on initialization. If you change weights afterwards, better know what you're doing.
        """
        
        number_of_features = int(number_of_features)
              
        for (model,priority) in models:
            assert(priority in self._priorities)
        models = [(model,self._priorities[p]) for (model,p) in models]
            

        # accept two types of weight declarations: one weight per model, and one weight per model and feature
        # type one is internally converted to type two: [0.1,0.9] -> [[0.1,0.9],[0.1,0.9],[0.1,0.9],[0.1,0.9]]
        if weights:
            if type(weights[0]) == list:
                assert(len(weights)==number_of_features)
                for sublist in weights:
                    assert(len(sublist)==len(models))
                
            else:
                assert(len(models) == len(weights))
                weights = [weights for i in range(number_of_features)]

        else:
            if self.mode == 'loglinear' or self.mode == 'interpolate':
                weights = [[1/len(models)]*len(models) for i in range(number_of_features)]
            elif self.mode == 'counts':
                weights = [[1]*len(models) for i in range(number_of_features)]
            sys.stderr.write('Warning: No weights defined: initializing with uniform weights\n')


        new_weights = normalize_weights(weights,self.mode,self.flags)
        if weights != new_weights:
            if self.mode == 'interpolate' or self.mode == 'loglinear':
                sys.stderr.write('Warning: weights should sum to 1 - ')
            elif self.mode == 'counts':
                sys.stderr.write('Warning: normalizing weights so that first model has weight 1 (for features that are recomputed from counts) - ')
            sys.stderr.write('normalizing to: '+ str(new_weights) +'\n')
            weights = new_weights
            
        return models,number_of_features,weights


    def _ensure_loaded(self,data):
        """load data (lexical tables; reference alignment; phrase table), if it isn't already in memory"""

        if 'lexical' in data:
            self.model_interface.require_alignment = True

        if 'reference' in data and not self.loaded['reference']:
            
            sys.stderr.write('Loading word pairs from reference set...')
            self.reference_interface.load_word_pairs(self.lang_src,self.lang_target)
            sys.stderr.write('done\n')
            self.loaded['reference'] = 1

        if 'lexical' in data and not self.loaded['lexical']:
            
            sys.stderr.write('Loading lexical tables...')
            self.model_interface.load_lexical_tables(self.models,self.mode)
            sys.stderr.write('done\n')
            self.loaded['lexical'] = 1
            
        if 'pt-filtered' in data and not self.loaded['pt-filtered']:
            
            models_prioritized = [(self.model_interface.open_table(model,'phrase-table'),priority,i) for (model,priority,i) in priority_sort_models(self.models)]
            
            for model,priority,i in models_prioritized:
                sys.stderr.write('Loading phrase table ' + str(i) + ' (only data relevant for reference set)')
                j = 0
                for line in model:
                    if not j % 1000000:
                        sys.stderr.write('...'+str(j))
                    j += 1
                    line = line.rstrip().split(b' ||| ')
                    self.model_interface.load_phrase_features(line,priority,i,store='all',mode=self.mode,filter_by=self.reference_interface.word_pairs,filter_by_src=self.reference_interface.word_source,filter_by_target=self.reference_interface.word_target,flags=self.flags)
                sys.stderr.write(' done\n')

            self.loaded['pt-filtered'] = 1

        if 'lexical-filtered' in data and not self.loaded['lexical-filtered']:
            e2f_filter, f2e_filter = _get_lexical_filter(self.reference_interface,self.model_interface)
            
            sys.stderr.write('Loading lexical tables (only data relevant for reference set)...')
            self.model_interface.load_lexical_tables(self.models,self.mode,e2f_filter=e2f_filter,f2e_filter=f2e_filter)
            sys.stderr.write('done\n')
            self.loaded['lexical-filtered'] = 1

        if 'pt-target' in data and not self.loaded['pt-target']:
            
            models_prioritized = [(self.model_interface.open_table(model,'phrase-table'),priority,i) for (model,priority,i) in priority_sort_models(self.models)]
            
            for model,priority,i in models_prioritized:
                sys.stderr.write('Loading target information from phrase table ' + str(i))
                j = 0
                for line in model:
                    if not j % 1000000:
                        sys.stderr.write('...'+str(j))
                    j += 1
                    line = line.rstrip().split(b' ||| ')
                    self.model_interface.load_phrase_features(line,priority,i,mode=self.mode,store='target',flags=self.flags)
                sys.stderr.write(' done\n')

            self.loaded['pt-target'] = 1


    def _inverse_wrapper(self,weights,tempdir=None):
        """if we want to invert the phrase table to better calcualte p(s|t) and lex(s|t), manage creation, sorting and merging of inverted phrase tables"""

        sys.stderr.write('Processing first table half\n')
        models = [(self.model_interface.open_table(model,'phrase-table'),priority,i) for (model,priority,i) in priority_sort_models(self.model_interface.models)]
        pt_half1 = NamedTemporaryFile(prefix='half1',delete=False,dir=tempdir)
        self._write_phrasetable(models,pt_half1,weights)
        pt_half1.seek(0)

        sys.stderr.write('Inverting tables\n')
        models = [(self.model_interface.create_inverse(self.model_interface.open_table(model,'phrase-table'),tempdir=tempdir),priority,i) for (model,priority,i) in priority_sort_models(self.model_interface.models)]
        sys.stderr.write('Processing second table half\n')
        pt_half2_inverted = NamedTemporaryFile(prefix='half2',delete=False,dir=tempdir)
        self._write_phrasetable(models,pt_half2_inverted,weights,inverted=True)
        pt_half2_inverted.close()
        for model,priority,i in models:
            model.close()
            os.remove(model.name)
        pt_half2 = sort_file(pt_half2_inverted.name,tempdir=tempdir)
        os.remove(pt_half2_inverted.name)

        sys.stderr.write('Merging tables: first half: {0} ; second half: {1} ; final table: {2}\n'.format(pt_half1.name,pt_half2.name,self.output_file))
        output_object = handle_file(self.output_file,'open',mode='w')
        self.model_interface.merge(pt_half1,pt_half2,output_object,self.mode)
        os.remove(pt_half1.name)
        os.remove(pt_half2.name)
        
        handle_file(self.output_file,'close',output_object,mode='w')
        

    def _write_phrasetable(self,models,output_object,weights,inverted=False):
        """Incrementally load phrase tables, calculate score for increment and write it to output_object"""

        # define which information we need to store from the phrase table
        # possible flags: 'all', 'target', 'source' and 'pairs'
        # interpolated models without re-normalization only need 'pairs', otherwise 'all' is the correct choice
        store_flag = 'all'
        if self.mode == 'interpolate' and not self.flags['normalized']:
            store_flag = 'pairs'

        i = 0
        sys.stderr.write('Incrementally loading and processing phrase tables...')

        for block in self.model_interface.traverse_incrementally('phrase-table',models,self.model_interface.load_phrase_features,store_flag,mode=self.mode,inverted=inverted,lowmem=self.flags['lowmem'],flags=self.flags):
            for src in sorted(self.model_interface.phrase_pairs, key = lambda x: x + b' |'):
                for target in sorted(self.model_interface.phrase_pairs[src], key = lambda x: x + b' |'):
                    
                    if not i % 1000000:
                        sys.stderr.write(str(i) + '...')
                    i += 1
                    
                    features = self.score(weights,src,target,self.model_interface,self.flags)
                    outline = self.model_interface.write_phrase_table(src,target,weights,features,self.mode, self.flags)
                    output_object.write(outline)
        sys.stderr.write('done\n')


    def combine_given_weights(self,weights=None):
        """write a new phrase table, based on existing weights"""
        
        if not weights:
            weights = self.weights
        
        data = []
    
        if self.mode == 'counts':
            data.append('lexical')
            if not self.flags['lowmem']:
                data.append('pt-target')
            
        elif self.mode == 'interpolate':
            if self.flags['recompute_lexweights']:
                data.append('lexical')
            if self.flags['normalized'] and self.flags['normalize_s_given_t'] == 't' and not self.flags['lowmem']:
                data.append('pt-target')
    
        self._ensure_loaded(data)

        if self.flags['lowmem'] and (self.mode == 'counts' or self.flags['normalized'] and self.flags['normalize_s_given_t'] == 't'):
            self._inverse_wrapper(weights,tempdir=self.flags['tempdir'])
        else:
            models = [(self.model_interface.open_table(model,'phrase-table'),priority,i) for (model,priority,i) in priority_sort_models(self.model_interface.models)]
            output_object = handle_file(self.output_file,'open',mode='w')
            self._write_phrasetable(models,output_object,weights)
            handle_file(self.output_file,'close',output_object,mode='w')

        if self.output_lexical:
            sys.stderr.write('Writing lexical tables\n')
            self._ensure_loaded(['lexical'])
            self.model_interface.write_lexical_file('e2f',self.output_lexical,weights[1],self.mode)
            self.model_interface.write_lexical_file('f2e',self.output_lexical,weights[3],self.mode)

    
    def combine_given_tuning_set(self):
        """write a new phrase table, using the weights that minimize cross-entropy on a tuning set"""
        
        data = ['reference','pt-filtered']
    
        if self.mode == 'counts' or (self.mode == 'interpolate' and self.flags['recompute_lexweights']):
            data.append('lexical-filtered')
    
        self._ensure_loaded(data)
        
        best_weights,best_cross_entropy = optimize_cross_entropy(self.model_interface,self.reference_interface,self.weights,self.score,self.mode,self.flags)
        sys.stderr.write('Best weights: ' + str(best_weights) + '\n')
        sys.stderr.write('Cross entropies: ' + str(best_cross_entropy) + '\n')
        sys.stderr.write('Executing action combine_given_weights with -w "{0}"\n'.format('; '.join([', '.join(str(w) for w in item) for item in best_weights])))
        
        self.loaded['pt-filtered'] = False # phrase table will be overwritten
        self.combine_given_weights(weights=best_weights)



    def combine_reordering_tables(self,weights=None):
        """write a new reordering table, based on existing weights."""
        
        if not weights:
            weights = self.weights
        
        data = []
    
        if self.mode != 'interpolate':
            sys.stderr.write('Error: only linear interpolation is supported for reordering model combination')
            
        output_object = handle_file(self.output_file,'open',mode='w')
        models = [(self.model_interface.open_table(model,'reordering-table'),priority,i) for (model,priority,i) in priority_sort_models(self.models)]

        i = 0
        
        sys.stderr.write('Incrementally loading and processing phrase tables...')

        for block in self.model_interface.traverse_incrementally('reordering-table',models,self.model_interface.load_reordering_probabilities,'pairs',mode=self.mode,lowmem=self.flags['lowmem'],flags=self.flags):
            for src in sorted(self.model_interface.reordering_pairs):
                for target in sorted(self.model_interface.reordering_pairs[src]):
                    if not i % 1000000:
                        sys.stderr.write(str(i) + '...')
                    i += 1
                    
                    features = score_interpolate_reordering(weights,src,target,self.model_interface)
                    outline = self.model_interface.write_reordering_table(src,target,features)
                    output_object.write(outline)
        sys.stderr.write('done\n')


        handle_file(self.output_file,'close',output_object,mode='w')


    def compare_cross_entropies(self):
        """print cross-entropies for each model/feature, using the intersection of phrase pairs.
           analysis tool.
        """
        
        self.flags['compare_cross-entropies'] = True
        
        data = ['reference','pt-filtered']

        if self.mode == 'counts' or (self.mode == 'interpolate' and self.flags['recompute_lexweights']):
            data.append('lexical-filtered')

        self._ensure_loaded(data)
        
        results, (intersection,total_pairs,oov2) = cross_entropy(self.model_interface,self.reference_interface,self.weights,self.score,self.mode,self.flags)
        
        padding = 90
        num_features = self.model_interface.number_of_features
        
        print('\nResults of model comparison\n')
        print('{0:<{padding}}: {1}'.format('phrase pairs in reference (tokens)',total_pairs, padding=padding))
        print('{0:<{padding}}: {1}'.format('phrase pairs in model intersection (tokens)',intersection, padding=padding))
        print('{0:<{padding}}: {1}\n'.format('phrase pairs in model union (tokens)',total_pairs-oov2, padding=padding))
        
        for i,data in enumerate(results):
            
            cross_entropies = data[:num_features]
            (other_translations,oov,ignored,n,total_pairs) = data[num_features:]
            
            print('model ' +str(i))
            for j in range(num_features):
                print('{0:<{padding}}: {1}'.format('cross-entropy for feature {0}'.format(j), cross_entropies[j], padding=padding))
            print('{0:<{padding}}: {1}'.format('phrase pairs in model (tokens)', n+ignored, padding=padding))
            print('{0:<{padding}}: {1}'.format('phrase pairs in model, but not in intersection (tokens)', ignored, padding=padding))
            print('{0:<{padding}}: {1}'.format('phrase pairs in union, but not in model (but source phrase is) (tokens)', other_translations, padding=padding))
            print('{0:<{padding}}: {1}\n'.format('phrase pairs in union, but source phrase not in model (tokens)', oov, padding=padding))

        self.flags['compare_cross-entropies'] = False
        
        return results, (intersection,total_pairs,oov2)


    def compute_cross_entropy(self):
        """return cross-entropy for a tuning set, a set of models and a set of weights.
           analysis tool.
        """
        
        data = ['reference','pt-filtered']
    
        if self.mode == 'counts' or (self.mode == 'interpolate' and self.flags['recompute_lexweights']):
            data.append('lexical-filtered')
    
        self._ensure_loaded(data)
        
        current_cross_entropy = cross_entropy(self.model_interface,self.reference_interface,self.weights,self.score,self.mode,self.flags)
        sys.stderr.write('Cross entropy: ' + str(current_cross_entropy) + '\n')
        return current_cross_entropy
        
        
    def return_best_cross_entropy(self):
        """return the set of weights and cross-entropy that is optimal for a tuning set and a set of models."""
        
        data = ['reference','pt-filtered']
    
        if self.mode == 'counts' or (self.mode == 'interpolate' and self.flags['recompute_lexweights']):
            data.append('lexical-filtered')
    
        self._ensure_loaded(data)
        
        best_weights,best_cross_entropy = optimize_cross_entropy(self.model_interface,self.reference_interface,self.weights,self.score,self.mode,self.flags)

        sys.stderr.write('Best weights: ' + str(best_weights) + '\n')
        sys.stderr.write('Cross entropies: ' + str(best_cross_entropy) + '\n')
        sys.stderr.write('You can apply these weights with the action combine_given_weights and the option -w "{0}"\n'.format('; '.join([', '.join(str(w) for w in item) for item in best_weights])))
        return best_weights,best_cross_entropy

        
def test():
    """test (and illustrate) the functionality of the program based on two test phrase tables and a small reference set,"""
    
    # linear interpolation of two models, with fixed weights. Output uses vocabulary of model1 (since model2 is supplementary)
    # command line: (currently not possible to define supplementary models through command line)
    sys.stderr.write('Regression test 1\n')
    Combiner = Combine_TMs([[os.path.join('test','model1'),'primary'],[os.path.join('test','model2'),'supplementary']],[0.5,0.5],os.path.join('test','phrase-table_test1'))
    Combiner.combine_given_weights()
    
    # linear interpolation of two models, with fixed weights (but different for each feature).
    # command line: python tmcombine.py combine_given_weights test/model1 test/model2 -w "0.1,0.9;0.1,1;0.2,0.8;0.5,0.5" -o test/phrase-table_test2
    sys.stderr.write('Regression test 2\n')
    Combiner = Combine_TMs([[os.path.join('test','model1'),'primary'],[os.path.join('test','model2'),'primary']],[[0.1,0.9],[0.1,1],[0.2,0.8],[0.5,0.5]],os.path.join('test','phrase-table_test2'))
    Combiner.combine_given_weights()

    # count-based combination of two models, with fixed weights
    # command line: python tmcombine.py combine_given_weights test/model1 test/model2 -w "0.1,0.9;0.1,1;0.2,0.8;0.5,0.5" -o test/phrase-table_test3 -m counts
    sys.stderr.write('Regression test 3\n')
    Combiner = Combine_TMs([[os.path.join('test','model1'),'primary'],[os.path.join('test','model2'),'primary']],[[0.1,0.9],[0.1,1],[0.2,0.8],[0.5,0.5]],os.path.join('test','phrase-table_test3'),mode='counts')
    Combiner.combine_given_weights()

    # output phrase table should be identical to model1
    # command line: python tmcombine.py combine_given_weights test/model1 -w 1 -o test/phrase-table_test4 -m counts
    sys.stderr.write('Regression test 4\n')
    Combiner = Combine_TMs([[os.path.join('test','model1'),'primary']],[1],os.path.join('test','phrase-table_test4'),mode='counts')
    Combiner.combine_given_weights()

    # count-based combination of two models with weights set through perplexity minimization
    # command line: python tmcombine.py combine_given_tuning_set test/model1 test/model2 -o test/phrase-table_test5 -m counts -r test/extract
    sys.stderr.write('Regression test 5\n')
    Combiner = Combine_TMs([[os.path.join('test','model1'),'primary'],[os.path.join('test','model2'),'primary']],output_file=os.path.join('test','phrase-table_test5'),mode='counts',reference_file='test/extract')
    Combiner.combine_given_tuning_set()

    # loglinear combination of two models with fixed weights
    # command line: python tmcombine.py combine_given_weights test/model1 test/model2 -w 0.1,0.9 -o test/phrase-table_test6 -m loglinear
    sys.stderr.write('Regression test 6\n')
    Combiner = Combine_TMs([[os.path.join('test','model1'),'primary'],[os.path.join('test','model2'),'primary']],weights=[0.1,0.9],output_file=os.path.join('test','phrase-table_test6'),mode='loglinear')
    Combiner.combine_given_weights()

    # cross-entropy analysis of two models through a reference set
    # command line: python tmcombine.py compare_cross_entropies test/model1 test/model2 -m counts -r test/extract
    sys.stderr.write('Regression test 7\n')
    Combiner = Combine_TMs([[os.path.join('test','model1'),'primary'],[os.path.join('test','model2'),'primary']],mode='counts',reference_file='test/extract')
    f = open(os.path.join('test','phrase-table_test7'),'w')
    f.write(str(Combiner.compare_cross_entropies()))
    f.close()
    
    # maximum a posteriori combination of two models (Bacchiani et al. 2004; Foster et al. 2010) with weights set through cross-entropy minimization
    # command line: (currently not possible through command line)
    sys.stderr.write('Regression test 8\n')
    Combiner = Combine_TMs([[os.path.join('test','model1'),'primary'],[os.path.join('test','model2'),'map']],output_file=os.path.join('test','phrase-table_test8'),mode='counts',reference_file='test/extract')
    Combiner.combine_given_tuning_set()

    # count-based combination of two non-default models, with fixed weights. Same as test 3, but with the standard features moved back
    # command line: python tmcombine.py combine_given_weights test/model3 test/model4 -w "0.5,0.5;0.5,0.5;0.5,0.5;0.5,0.5;0.1,0.9;0.1,1;0.2,0.8;0.5,0.5" -o test/phrase-table_test9 -m counts --number_of_features 8 --i_e2f 4 --i_e2f_lex 5 --i_f2e 6 --i_f2e_lex 7 -r test/extract
    sys.stderr.write('Regression test 9\n')
    Combiner = Combine_TMs([[os.path.join('test','model3'),'primary'],[os.path.join('test','model4'),'primary']],[[0.5,0.5],[0.5,0.5],[0.5,0.5],[0.5,0.5],[0.1,0.9],[0.1,1],[0.2,0.8],[0.5,0.5]],os.path.join('test','phrase-table_test9'),mode='counts',number_of_features=8,i_e2f=4,i_e2f_lex=5,i_f2e=6,i_f2e_lex=7)
    Combiner.combine_given_weights()

    # count-based combination of two non-default models, with fixed weights. Same as test 5, but with the standard features moved back
    # command line: python tmcombine.py combine_given_tuning_set test/model3 test/model4 -o test/phrase-table_test10 -m counts --number_of_features 8 --i_e2f 4 --i_e2f_lex 5 --i_f2e 6 --i_f2e_lex 7 -r test/extract
    sys.stderr.write('Regression test 10\n')
    Combiner = Combine_TMs([[os.path.join('test','model3'),'primary'],[os.path.join('test','model4'),'primary']],output_file=os.path.join('test','phrase-table_test10'),mode='counts',number_of_features=8,i_e2f=4,i_e2f_lex=5,i_f2e=6,i_f2e_lex=7,reference_file='test/extract')
    Combiner.combine_given_tuning_set()
    

#convert weight vector passed as a command line argument
class to_list(argparse.Action):
     def __call__(self, parser, namespace, weights, option_string=None):
         if ';' in weights:
             values = [[float(x) for x in vector.split(',')] for vector in weights.split(';')]
         else:
             values = [float(x) for x in weights.split(',')]
         setattr(namespace, self.dest, values)


def parse_command_line():
    parser = argparse.ArgumentParser(description='Combine translation models. Check DOCSTRING of the class Combine_TMs() and its methods for a more in-depth documentation and additional configuration options not available through the command line. The function test() shows examples.')
    
    group1 = parser.add_argument_group('Main options')
    group2 = parser.add_argument_group('More model combination options')
    
    group1.add_argument('action', metavar='ACTION', choices=["combine_given_weights","combine_given_tuning_set","combine_reordering_tables","compute_cross_entropy","return_best_cross_entropy","compare_cross_entropies"],
                    help='What you want to do with the models. One of %(choices)s.')
    
    group1.add_argument('model', metavar='DIRECTORY', nargs='+',
                    help='Model directory. Assumes default Moses structure (i.e. path to phrase table and lexical tables).')
                    
    group1.add_argument('-w', '--weights', dest='weights', action=to_list,
                    default=None,
                    help='weight vector. Format 1: single vector, one weight per model. Example: \"0.1,0.9\" ; format 2: one vector per feature, one weight per model: \"0.1,0.9;0.5,0.5;0.4,0.6;0.2,0.8\"')

    group1.add_argument('-m', '--mode', type=str,
                    default="interpolate",
                    choices=["counts","interpolate","loglinear"],
                    help='basic mixture-model algorithm. Default: %(default)s. Note: depending on mode and additional configuration, additional statistics are needed. Check docstring documentation of Combine_TMs() for more info.')

    group1.add_argument('-r', '--reference', type=str,
                    default=None,
                    help='File containing reference phrase pairs for cross-entropy calculation. Default interface expects \'path/model/extract.gz\' that is produced by training a model on the reference (i.e. development) corpus.')

    group1.add_argument('-o', '--output', type=str,
                    default="-",
                    help='Output file (phrase table). If not specified, model is written to standard output.')

    group1.add_argument('--output-lexical', type=str,
                    default=None,
                    help=('Not only create a combined phrase table, but also combined lexical tables. Writes to OUTPUT_LEXICAL.e2f and OUTPUT_LEXICAL.f2e, or OUTPUT_LEXICAL.counts.e2f in mode \'counts\'.'))

    group1.add_argument('--lowmem', action="store_true",
                    help=('Low memory mode: requires two passes (and sorting in between) to combine a phrase table, but loads less data into memory. Only relevant for mode "counts" and some configurations of mode "interpolate".'))

    group1.add_argument('--tempdir', type=str,
                    default=None,
                    help=('Temporary directory in --lowmem mode.'))

    group2.add_argument('--i_e2f', type=int,
                    default=0, metavar='N',
                    help=('Index of p(f|e) (relevant for mode counts if phrase table has custom feature order). (default: %(default)s)'))

    group2.add_argument('--i_e2f_lex', type=int,
                    default=1, metavar='N',
                    help=('Index of lex(f|e) (relevant for mode counts or with option recompute_lexweights if phrase table has custom feature order). (default: %(default)s)'))

    group2.add_argument('--i_f2e', type=int,
                    default=2, metavar='N',
                    help=('Index of p(e|f) (relevant for mode counts if phrase table has custom feature order). (default: %(default)s)'))

    group2.add_argument('--i_f2e_lex', type=int,
                    default=3, metavar='N',
                    help=('Index of lex(e|f) (relevant for mode counts or with option recompute_lexweights if phrase table has custom feature order). (default: %(default)s)'))

    group2.add_argument('--number_of_features', type=int,
                    default=4, metavar='N',
                    help=('Combine models with N + 1 features (last feature is constant phrase penalty). (default: %(default)s)'))

    group2.add_argument('--normalized', action="store_true",
                    help=('for each phrase pair x,y: ignore models with p(y)=0, and distribute probability mass among models with p(y)>0. (default: missing entries (x,y) are always interpreted as p(x|y)=0). Only relevant in mode "interpolate".'))
    
    group2.add_argument('--write-phrase-penalty', action="store_true",
      help=("Include phrase penalty in phrase table"))

    group2.add_argument('--recompute_lexweights', action="store_true",
                    help=('don\'t directly interpolate lexical weights, but interpolate word translation probabilities instead and recompute the lexical weights. Only relevant in mode "interpolate".'))

    return parser.parse_args()

if __name__ == "__main__":
    
    if len(sys.argv) < 2:
        sys.stderr.write("no command specified. use option -h for usage instructions\n")
    
    elif sys.argv[1] == "test":
        test()
        
    else:
        args = parse_command_line()
        #initialize
        combiner = Combine_TMs([(m,'primary') for m in args.model],
                               weights=args.weights,
                               mode=args.mode,
                               output_file=args.output,
                               reference_file=args.reference,
                               output_lexical=args.output_lexical,
                               lowmem=args.lowmem,
                               normalized=args.normalized,
                               recompute_lexweights=args.recompute_lexweights,
                               tempdir=args.tempdir,
                               number_of_features=args.number_of_features,
                               i_e2f=args.i_e2f,
                               i_e2f_lex=args.i_e2f_lex,
                               i_f2e=args.i_f2e,
                               i_f2e_lex=args.i_f2e_lex,
                               write_phrase_penalty=args.write_phrase_penalty)
        # execute right method
        f_string = "combiner."+args.action+'()'
        exec(f_string)
