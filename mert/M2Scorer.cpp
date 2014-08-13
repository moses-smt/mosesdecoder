#include "M2Scorer.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <sstream>

#include <boost/lexical_cast.hpp>


using namespace std;
using namespace boost::python;

namespace MosesTuning
{

M2Scorer::M2Scorer(const string& config)
  : StatisticsBasedScorer("M2Scorer", config),
    beta_(Scan<float>(getConfig("beta", "0.5"))),
    max_unchanged_words_(Scan<int>(getConfig("max_unchanged_words", "2"))),
    ignore_whitespace_casing_(Scan<bool>(getConfig("ignore_whitespace_casing", "false")))
{
  Py_Initialize();
  object main_module = import("__main__");
  main_namespace_ = main_module.attr("__dict__");
  exec(code(), main_namespace_);
}

void M2Scorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  for(size_t i = 0; i < referenceFiles.size(); ++i) {
    
    m2_ = main_namespace_["M2Obj"](
        referenceFiles[i],
        beta_,
        max_unchanged_words_,
        ignore_whitespace_casing_
    );
    
    break;
  }
}

void M2Scorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  string sentence = trimStr(this->preprocessSentence(text));
  
  if(seen_.count(sentence) != 0) {
    entry.set(seen_[sentence]);
    return;
  }
  
  std::vector<int> stats;
  
  boost::python::object list = m2_.attr("sufstats")(sentence, sid);
  
  int correct = extract<int>(list[0]);
  int proposed = extract<int>(list[1]);
  int gold = extract<int>(list[2]);
  
  stats.push_back(correct);
  stats.push_back(proposed);
  stats.push_back(gold);
  
  seen_[sentence] = stats;
  entry.set(stats);
}

float M2Scorer::calculateScore(const vector<int>& comps) const
{
  if (comps.size() != NumberOfScores()) {
    throw runtime_error("Size of stat vector for M2Scorer is not " + NumberOfScores());
  }
  
  float beta = boost::lexical_cast<float>(beta_);
  
  float p = 0.0;
  float r = 0.0;
  float f = 0.0;
    
  if(comps[1] != 0)
    p = comps[0] / (double)comps[1];
  else
    p = 1.0;
    
  if(comps[2] != 0)
    r = comps[0] / (double)comps[2];
  else
    r = 1.0;
  
  float denom = beta * beta * p + r;
  if(denom != 0)
    f = (1.0 + beta * beta) * p * r / denom;
  else
    f = 0.0;
  
  return f;
}

float sentenceBackgroundM2(const std::vector<float>& stats, const std::vector<float>& bg)
{
  float beta = 0.5;
  
  float p = 0.0;
  float r = 0.0;
  float f = 0.0;
    
  if(stats[1] + bg[1] != 0)
    p = (stats[0] + bg[0]) / (stats[1] + bg[1]);
  else
    p = 1.0;
    
  if(stats[2] + bg[2] != 0)
    r = (stats[0] + bg[0]) / (stats[2] + bg[2]);
  else
    r = 1.0;
  
  float denom = beta * beta * p + r;
  if(denom != 0)
    f = (1.0 + beta * beta) * p * r / denom;
  else
    f = 0.0;
  
  return f;
}

float sentenceScaledM2(const std::vector<float>& stats)
{
  float beta = 0.5;
  
  float p = 0.0;
  float r = 0.0;
  float f = 0.0;
    
  float smoothing = 0.0;  
  
  if(stats[1] + smoothing != 0)
    p = (stats[0] + smoothing) / (stats[1] + smoothing);
  else
    p = 1.0;
    
  if(stats[2] + smoothing != 0)
    r = (stats[0] + smoothing) / (stats[2] + smoothing);
  else
    r = 1.0;
  
  float denom = beta * beta * p + r;
  if(denom != 0)
    f = (1.0 + beta * beta) * p * r / denom;
  else
    f = 0.0;
  
  return f;
}

float sentenceM2(const std::vector<float>& stats)
{
  float beta = 0.5;
  
  float p = 0.0;
  float r = 0.0;
  float f = 0.0;
    
  if(stats[1] != 0)
    p = stats[0] / stats[1];
  else
    p = 1.0;
    
  if(stats[2] != 0)
    r = stats[0] / stats[2];
  else
    r = 1.0;
  
  float denom = beta * beta * p + r;
  if(denom != 0)
    f = (1.0 + beta * beta) * p * r / denom;
  else
    f = 0.0;
  
  return f;
}

const char* M2Scorer::code() {
  return
    "import sys\n"
    "from getopt import getopt\n"
    "from itertools import izip\n"
    "import re\n"
    "from copy import deepcopy\n"
    "\n"
    "\n"
    "def load_annotation(gold_file):\n"
    "    source_sentences = []\n"
    "    gold_edits = []\n"
    "    fgold = smart_open(gold_file, 'r')\n"
    "    puffer = fgold.read()\n"
    "    fgold.close()\n"
    "    puffer = puffer.decode('utf8')\n"
    "    for item in paragraphs(puffer.splitlines(True)):\n"
    "        item = item.splitlines(False)\n"
    "        sentence = [line[2:].strip() for line in item if line.startswith('S ')]\n"
    "        assert sentence != []\n"
    "        annotations = {}\n"
    "        for line in item[1:]:\n"
    "            if line.startswith('I ') or line.startswith('S '):\n"
    "                continue\n"
    "            assert line.startswith('A ')\n"
    "            line = line[2:]\n"
    "            fields = line.split('|||')\n"
    "            start_offset = int(fields[0].split()[0])\n"
    "            end_offset = int(fields[0].split()[1])\n"
    "            etype = fields[1]\n"
    "            if etype == 'noop':\n"
    "                start_offset = -1\n"
    "                end_offset = -1\n"
    "            corrections =  [c.strip() if c != '-NONE-' else '' for c in fields[2].split('||')]\n"
    "            # NOTE: start and end are *token* offsets\n"
    "            original = ' '.join(' '.join(sentence).split()[start_offset:end_offset])\n"
    "            annotator = int(fields[5])\n"
    "            if annotator not in annotations.keys():\n"
    "                annotations[annotator] = []\n"
    "            annotations[annotator].append((start_offset, end_offset, original, corrections))\n"
    "        tok_offset = 0\n"
    "        for this_sentence in sentence:\n"
    "            tok_offset += len(this_sentence.split())\n"
    "            source_sentences.append(this_sentence)\n"
    "            this_edits = {}\n"
    "            for annotator, annotation in annotations.iteritems():\n"
    "                this_edits[annotator] = [edit for edit in annotation if edit[0] <= tok_offset and edit[1] <= tok_offset and edit[0] >= 0 and edit[1] >= 0]\n"
    "            if len(this_edits) == 0:\n"
    "                this_edits[0] = []\n"
    "            gold_edits.append(this_edits)\n"
    "    return (source_sentences, gold_edits)\n"
    "\n"
    "\n"
    "def f1_suffstats(candidate, source, gold_edits, max_unchanged_words=2, beta = 0.5, ignore_whitespace_casing= False, verbose=False, very_verbose=False):\n"
    "    stat_correct = 0.0\n"
    "    stat_proposed = 0.0\n"
    "    stat_gold = 0.0\n"
    "\n"
    "    candidate_tok = candidate.split()\n"
    "    source_tok = source.split()\n"
    "\n"
    // Prevent losing time on stupid candidates that have more than 10 spurious tokens
    "    if len(candidate_tok) > len(source_tok) + 10:\n"
    "        return (0,0,1);\n"
    "\n"
    "    lmatrix1, backpointers1 = levenshtein_matrix(source_tok, candidate_tok, 1, 1, 1)\n"
    "    lmatrix2, backpointers2 = levenshtein_matrix(source_tok, candidate_tok, 1, 1, 2)\n"
    "\n"
    "    #V, E, dist, edits = edit_graph(lmatrix, backpointers)\n"
    "    V1, E1, dist1, edits1 = edit_graph(lmatrix1, backpointers1)\n"
    "    V2, E2, dist2, edits2 = edit_graph(lmatrix2, backpointers2)\n"
    "\n"
    "    V, E, dist, edits = merge_graph(V1, V2, E1, E2, dist1, dist2, edits1, edits2)\n"
    "    if very_verbose:\n"
    "        print \"edit matrix 1:\", lmatrix1\n"
    "        print \"edit matrix 2:\", lmatrix2\n"
    "        print \"backpointers 1:\", backpointers1\n"
    "        print \"backpointers 2:\", backpointers2\n"
    "        print \"edits (w/o transitive arcs):\", edits\n"
    "    V, E, dist, edits = transitive_arcs(V, E, dist, edits, max_unchanged_words, very_verbose)\n"
    "\n"
    "    # Find measures maximizing current cumulative F1; local: curent annotator only\n"
    "    sqbeta = beta * beta\n"
    "    chosen_ann = -1\n"
    "    f1_max = -1.0\n"
    "\n"
    "    argmax_correct = 0.0\n"
    "    argmax_proposed = 0.0\n"
    "    argmax_gold = 0.0\n"
    "    max_stat_correct = -1.0\n"
    "    min_stat_proposed = float(\"inf\")\n"
    "    min_stat_gold = float(\"inf\")\n"
    "    for annotator, gold in gold_edits.iteritems():\n"
    "        localdist = set_weights(E, dist, edits, gold, verbose, very_verbose)\n"
    "        editSeq = best_edit_seq_bf(V, E, localdist, edits, very_verbose)\n"
    "        if verbose:\n"
    "            print \">> Annotator:\", annotator\n"
    "        if very_verbose:\n"
    "            print \"Graph(V,E) = \"\n"
    "            print \"V =\", V\n"
    "            print \"E =\", E\n"
    "            print \"edits (with transitive arcs):\", edits\n"
    "            print \"dist() =\", localdist\n"
    "            print \"viterbi path =\", editSeq\n"
    "        if ignore_whitespace_casing:\n"
    "            editSeq = filter(lambda x : not equals_ignore_whitespace_casing(x[2], x[3]), editSeq)\n"
    "        correct = matchSeq(editSeq, gold, ignore_whitespace_casing, verbose)\n"
    "\n"
    "        # local cumulative counts, P, R and F1\n"
    "        stat_correct_local = stat_correct + len(correct)\n"
    "        stat_proposed_local = stat_proposed + len(editSeq)\n"
    "        stat_gold_local = stat_gold + len(gold)\n"
    "        p_local = comp_p(stat_correct_local, stat_proposed_local)\n"
    "        r_local = comp_r(stat_correct_local, stat_gold_local)\n"
    "        f1_local = comp_f1(stat_correct_local, stat_proposed_local, stat_gold_local, beta)\n"
    "\n"
    "        if f1_max < f1_local or \\\n"
    "          (f1_max == f1_local and max_stat_correct < stat_correct_local) or \\\n"
    "          (f1_max == f1_local and max_stat_correct == stat_correct_local and min_stat_proposed + sqbeta * min_stat_gold > stat_proposed_local + sqbeta * stat_gold_local):\n"
    "            chosen_ann = annotator\n"
    "            f1_max = f1_local\n"
    "            max_stat_correct = stat_correct_local\n"
    "            min_stat_proposed = stat_proposed_local\n"
    "            min_stat_gold = stat_gold_local\n"
    "            argmax_correct = len(correct)\n"
    "            argmax_proposed = len(editSeq)\n"
    "            argmax_gold = len(gold)\n"
    "\n"
    "        if verbose:\n"
    "            print \"SOURCE        :\", source.encode(\"utf8\")\n"
    "            print \"HYPOTHESIS    :\", candidate.encode(\"utf8\")\n"
    "            print \"EDIT SEQ      :\", [shrinkEdit(ed) for ed in list(reversed(editSeq))]\n"
    "            print \"GOLD EDITS    :\", gold\n"
    "            print \"CORRECT EDITS :\", correct\n"
    "            print \"# correct     :\", int(stat_correct_local)\n"
    "            print \"# proposed    :\", int(stat_proposed_local)\n"
    "            print \"# gold        :\", int(stat_gold_local)\n"
    "            print \"precision     :\", p_local\n"
    "            print \"recall        :\", r_local\n"
    "            print \"f_%.1f         :\" % beta, f1_local\n"
    "            print \"-------------------------------------------\"\n"
    "    if verbose:\n"
    "        print \">> Chosen Annotator for line\", i, \":\", chosen_ann\n"
    "        print \"\"\n"
    "    stat_correct += argmax_correct\n"
    "    stat_proposed += argmax_proposed\n"
    "    stat_gold += argmax_gold\n"
    "    return (stat_correct, stat_proposed, stat_gold)\n"
    "\n"
    "def comp_p(a, b):\n"
    "    try:\n"
    "        p  = a / b\n"
    "    except ZeroDivisionError:\n"
    "        p = 1.0\n"
    "    return p\n"
    "\n"
    "def comp_r(c, g):\n"
    "    try:\n"
    "        r  = c / g\n"
    "    except ZeroDivisionError:\n"
    "        r = 1.0\n"
    "    return r\n"
    "\n"
    "def comp_f1(c, e, g, b):\n"
    "    try:\n"
    "        f = (1+b*b) * c / (b*b*g+e)\n"
    "        #f = 2 * c / (g+e)\n"
    "    except ZeroDivisionError:\n"
    "        if c == 0.0:\n"
    "            f = 1.0\n"
    "        else:\n"
    "            f = 0.0\n"
    "    return f\n"
    "\n"
    "def equals_ignore_whitespace_casing(a,b):\n"
    "    return a.replace(\" \", \"\").lower() == b.replace(\" \", \"\").lower()\n"
    "\n"
    "# distance function\n"
    "def get_distance(dist, v1, v2):\n"
    "    try:\n"
    "        return dist[(v1, v2)]\n"
    "    except KeyError:\n"
    "        return float('inf')\n"
    "\n"
    "\n"
    "# find maximally matching edit squence through the graph using bellman-ford\n"
    "def best_edit_seq_bf(V, E, dist, edits, verby_verbose=False):\n"
    "    thisdist = {}\n"
    "    path = {}\n"
    "    for v in V:\n"
    "        thisdist[v] = float('inf')\n"
    "    thisdist[(0,0)] = 0\n"
    "    for i in range(len(V)-1):\n"
    "        for edge in E:\n"
    "            v = edge[0]\n"
    "            w = edge[1]\n"
    "            if thisdist[v] + dist[edge] < thisdist[w]:\n"
    "                thisdist[w] = thisdist[v] + dist[edge]\n"
    "                path[w] = v\n"
    "    # backtrack\n"
    "    v = sorted(V)[-1]\n"
    "    editSeq = []\n"
    "    while True:\n"
    "        try:\n"
    "            w = path[v]\n"
    "        except KeyError:\n"
    "            break\n"
    "        edit = edits[(w,v)]\n"
    "        if edit[0] != 'noop':\n"
    "            editSeq.append((edit[1], edit[2], edit[3], edit[4]))\n"
    "        v = w\n"
    "    return editSeq\n"
    "\n"
    "\n"
    "# set weights on the graph, gold edits edges get negative weight\n"
    "# other edges get an epsilon weight added\n"
    "# gold_edits = (start, end, original, correction)\n"
    "def set_weights(E, dist, edits, gold_edits, verbose=False, very_verbose=False):\n"
    "    EPSILON = 0.001\n"
    "    if very_verbose:\n"
    "        print \"set weights of edges()\",\n"
    "        print \"gold edits :\", gold_edits\n"
    "\n"
    "    gold_set = deepcopy(gold_edits)\n"
    "    retdist = deepcopy(dist)\n"
    "\n"
    "    M = {}\n"
    "    G = {}\n"
    "    for edge in E:\n"
    "        tE = edits[edge]\n"
    "        s, e = tE[1], tE[2]\n"
    "        if (s, e) not in M:\n"
    "            M[(s,e)] = []\n"
    "        M[(s,e)].append(edge)\n"
    "        if (s, e) not in G:\n"
    "            G[(s,e)] = []\n"
    "\n"
    "    for gold in gold_set:\n"
    "        s, e = gold[0], gold[1]\n"
    "        if (s, e) not in G:\n"
    "            G[(s,e)] = []\n"
    "        G[(s,e)].append(gold)\n"
    "\n"
    "    for k in sorted(M.keys()):\n"
    "        M[k] = sorted(M[k])\n"
    "\n"
    "        if k[0] == k[1]: # insertion case\n"
    "            lptr = 0\n"
    "            rptr = len(M[k])-1\n"
    "            cur = lptr\n"
    "\n"
    "            g_lptr = 0\n"
    "            g_rptr = len(G[k])-1\n"
    "\n"
    "            while lptr <= rptr:\n"
    "                hasGoldMatch = False\n"
    "                edge = M[k][cur]\n"
    "                thisEdit = edits[edge]\n"
    "                # only check start offset, end offset, original string, corrections\n"
    "                if very_verbose:\n"
    "                    print \"set weights of edge\", edge\n"
    "                    print \"edit  =\", thisEdit\n"
    "\n"
    "                cur_gold = []\n"
    "                if cur == lptr:\n"
    "                    cur_gold = range(g_lptr, g_rptr+1)\n"
    "                else:\n"
    "                    cur_gold = reversed(range(g_lptr, g_rptr+1))\n"
    "\n"
    "                for i in cur_gold:\n"
    "                    gold = G[k][i]\n"
    "                    if thisEdit[1] == gold[0] and \\\n"
    "                        thisEdit[2] == gold[1] and \\\n"
    "                        thisEdit[3] == gold[2] and \\\n"
    "                        thisEdit[4] in gold[3]:\n"
    "                        hasGoldMatch = True\n"
    "                        retdist[edge] = - len(E)\n"
    "                        if very_verbose:\n"
    "                            print \"matched gold edit :\", gold\n"
    "                            print \"set weight to :\", retdist[edge]\n"
    "                        if cur == lptr:\n"
    "                            #g_lptr += 1 # why?\n"
    "                            g_lptr = i + 1\n"
    "                        else:\n"
    "                            #g_rptr -= 1 # why?\n"
    "                            g_rptr = i - 1\n"
    "                        break\n"
    "\n"
    "                if not hasGoldMatch and thisEdit[0] != 'noop':\n"
    "                    retdist[edge] += EPSILON\n"
    "                if hasGoldMatch:\n"
    "                    if cur == lptr:\n"
    "                        lptr += 1\n"
    "                        while lptr < len(M[k]) and M[k][lptr][0] != M[k][cur][1]:\n"
    "                            if edits[M[k][lptr]] != 'noop':\n"
    "                                retdist[M[k][lptr]] += EPSILON\n"
    "                            lptr += 1\n"
    "                        cur = lptr\n"
    "                    else:\n"
    "                        rptr -= 1\n"
    "                        while rptr >= 0 and M[k][rptr][1] != M[k][cur][0]:\n"
    "                            if edits[M[k][rptr]] != 'noop':\n"
    "                                retdist[M[k][rptr]] += EPSILON\n"
    "                            rptr -= 1\n"
    "                        cur = rptr\n"
    "                else:\n"
    "                    if cur == lptr:\n"
    "                        lptr += 1\n"
    "                        cur = rptr\n"
    "                    else:\n"
    "                        rptr -= 1\n"
    "                        cur = lptr\n"
    "        else: #deletion or substitution, don't care about order, no harm if setting parallel edges weight < 0\n"
    "            for edge in M[k]:\n"
    "                hasGoldMatch = False\n"
    "                thisEdit = edits[edge]\n"
    "                if very_verbose:\n"
    "                    print \"set weights of edge\", edge\n"
    "                    print \"edit  =\", thisEdit\n"
    "                for gold in G[k]:\n"
    "                    if thisEdit[1] == gold[0] and \\\n"
    "                        thisEdit[2] == gold[1] and \\\n"
    "                        thisEdit[3] == gold[2] and \\\n"
    "                        thisEdit[4] in gold[3]:\n"
    "                        hasGoldMatch = True\n"
    "                        retdist[edge] = - len(E)\n"
    "                        if very_verbose:\n"
    "                            print \"matched gold edit :\", gold\n"
    "                            print \"set weight to :\", retdist[edge]\n"
    "                        break\n"
    "                if not hasGoldMatch and thisEdit[0] != 'noop':\n"
    "                    retdist[edge] += EPSILON\n"
    "    return retdist\n"
    "\n"
    "\n"
    "def merge_graph(V1, V2, E1, E2, dist1, dist2, edits1, edits2):\n"
    "    # vertices\n"
    "    V = deepcopy(V1)\n"
    "    for v in V2:\n"
    "        if v not in V:\n"
    "            V.append(v)\n"
    "    V = sorted(V)\n"
    "\n"
    "    # edges\n"
    "    E = E1\n"
    "    for e in E2:\n"
    "        if e not in V:\n"
    "            E.append(e)\n"
    "    E = sorted(E)\n"
    "\n"
    "    # distances\n"
    "    dist = deepcopy(dist1)\n"
    "    for k in dist2.keys():\n"
    "        if k not in dist.keys():\n"
    "            dist[k] = dist2[k]\n"
    "        else:\n"
    "            if dist[k] != dist2[k]:\n"
    "                print >> sys.stderr, \"WARNING: merge_graph: distance does not match!\"\n"
    "                dist[k] = min(dist[k], dist2[k])\n"
    "\n"
    "    # edit contents\n"
    "    edits = deepcopy(edits1)\n"
    "    for e in edits2.keys():\n"
    "        if e not in edits.keys():\n"
    "            edits[e] = edits2[e]\n"
    "        else:\n"
    "            if edits[e] != edits2[e]:\n"
    "                print >> sys.stderr, \"WARNING: merge_graph: edit does not match!\"\n"
    "    return (V, E, dist, edits)\n"
    "\n"
    "# add transitive arcs\n"
    "def transitive_arcs(V, E, dist, edits, max_unchanged_words=2, very_verbose=False):\n"
    "    if very_verbose:\n"
    "        print \"-- Add transitive arcs --\"\n"
    "    for k in range(len(V)):\n"
    "        vk = V[k]\n"
    "        if very_verbose:\n"
    "            print \"v _k :\", vk\n"
    "\n"
    "        for i in range(len(V)):\n"
    "            vi = V[i]\n"
    "            if very_verbose:\n"
    "                print \"v _i :\", vi\n"
    "            try:\n"
    "                eik = edits[(vi, vk)]\n"
    "            except KeyError:\n"
    "                continue\n"
    "            for j in range(len(V)):\n"
    "                vj = V[j]\n"
    "                if very_verbose:\n"
    "                    print \"v _j :\", vj\n"
    "                try:\n"
    "                    ekj = edits[(vk, vj)]\n"
    "                except KeyError:\n"
    "                    continue\n"
    "                dik = get_distance(dist, vi, vk)\n"
    "                dkj = get_distance(dist, vk, vj)\n"
    "                if dik + dkj < get_distance(dist, vi, vj):\n"
    "                    eij = merge_edits(eik, ekj)\n"
    "                    if eij[-1] <= max_unchanged_words:\n"
    "                        if very_verbose:\n"
    "                            print \" add new arcs v_i -> v_j:\", eij\n"
    "                        E.append((vi, vj))\n"
    "                        dist[(vi, vj)] = dik + dkj\n"
    "                        edits[(vi, vj)] = eij\n"
    "    # remove noop transitive arcs\n"
    "    if very_verbose:\n"
    "        print \"-- Remove transitive noop arcs --\"\n"
    "    for edge in E:\n"
    "        e = edits[edge]\n"
    "        if e[0] == 'noop' and dist[edge] > 1:\n"
    "            if very_verbose:\n"
    "                print \" remove noop arc v_i -> vj:\", edge\n"
    "            E.remove(edge)\n"
    "            dist[edge] = float('inf')\n"
    "            del edits[edge]\n"
    "    return(V, E, dist, edits)\n"
    "\n"
    "def shrinkEdit(edit):\n"
    "    shrunkEdit = deepcopy(edit)\n"
    "    origtok = edit[2].split()\n"
    "    corrtok = edit[3].split()\n"
    "    i = 0\n"
    "    cstart = 0\n"
    "    cend = len(corrtok)\n"
    "    found = False\n"
    "    while i < min(len(origtok), len(corrtok)) and not found:\n"
    "        if origtok[i] != corrtok[i]:\n"
    "            found = True\n"
    "        else:\n"
    "            cstart += 1\n"
    "            i += 1\n"
    "    j = 1\n"
    "    found = False\n"
    "    while j <= min(len(origtok), len(corrtok)) - cstart and not found:\n"
    "        if origtok[len(origtok) - j] != corrtok[len(corrtok) - j]:\n"
    "            found = True\n"
    "        else:\n"
    "            cend -= 1\n"
    "            j += 1\n"
    "    shrunkEdit = (edit[0] + i, edit[1] - (j-1), ' '.join(origtok[i : len(origtok)-(j-1)]), ' '.join(corrtok[i : len(corrtok)-(j-1)]))\n"
    "    return shrunkEdit\n"
    "\n"
    "def matchSeq(editSeq, gold_edits, ignore_whitespace_casing= False, verbose=False):\n"
    "    m = []\n"
    "    goldSeq = deepcopy(gold_edits)\n"
    "    last_index = 0\n"
    "    CInsCDel = False\n"
    "    CInsWDel = False\n"
    "    CDelWIns = False\n"
    "    for e in reversed(editSeq):\n"
    "        for i in range(last_index, len(goldSeq)):\n"
    "            g = goldSeq[i]\n"
    "            if matchEdit(e,g, ignore_whitespace_casing):\n"
    "                m.append(e)\n"
    "                last_index = i+1\n"
    "                if verbose:\n"
    "                    nextEditList = [shrinkEdit(edit) for edit in editSeq if e[1] == edit[0]]\n"
    "                    prevEditList = [shrinkEdit(edit) for edit in editSeq if e[0] == edit[1]]\n"
    "\n"
    "                    if e[0] != e[1]:\n"
    "                        nextEditList = [edit for edit in nextEditList if edit[0] == edit[1]]\n"
    "                        prevEditList = [edit for edit in prevEditList if edit[0] == edit[1]]\n"
    "                    else:\n"
    "                        nextEditList = [edit for edit in nextEditList if edit[0] < edit[1] and edit[3] == '']\n"
    "                        prevEditList = [edit for edit in prevEditList if edit[0] < edit[1] and edit[3] == '']\n"
    "\n"
    "                    matchAdj = any(any(matchEdit(edit, gold, ignore_whitespace_casing) for gold in goldSeq) for edit in nextEditList) or \\\n"
    "                        any(any(matchEdit(edit, gold, ignore_whitespace_casing) for gold in goldSeq) for edit in prevEditList)\n"
    "                    if e[0] < e[1] and len(e[3].strip()) == 0 and \\\n"
    "                        (len(nextEditList) > 0 or len(prevEditList) > 0):\n"
    "                        if matchAdj:\n"
    "                            print \"!\", e\n"
    "                        else:\n"
    "                            print \"&\", e\n"
    "                    elif e[0] == e[1] and \\\n"
    "                        (len(nextEditList) > 0 or len(prevEditList) > 0):\n"
    "                        if matchAdj:\n"
    "                            print \"!\", e\n"
    "                        else:\n"
    "                            print \"*\", e\n"
    "    return m\n"
    "\n"
    "\n"
    "def matchEdit(e, g, ignore_whitespace_casing= False):\n"
    "    # start offset\n"
    "    if e[0] != g[0]:\n"
    "        return False\n"
    "    # end offset\n"
    "    if e[1] != g[1]:\n"
    "        return False\n"
    "    # original string\n"
    "    if e[2] != g[2]:\n"
    "        return False\n"
    "    # correction string\n"
    "    if not e[3] in g[3]:\n"
    "        return False\n"
    "    # all matches\n"
    "    return True\n"
    "\n"
    "\n"
    "# combine two edits into one\n"
    "# edit = (type, start, end, orig, correction, #unchanged_words)\n"
    "def merge_edits(e1, e2, joiner = ' '):\n"
    "    if e1[0] == 'ins':\n"
    "        if e2[0] == 'ins':\n"
    "            e = ('ins', e1[1], e2[2], '', e1[4] + joiner + e2[4], e1[5] + e2[5])\n"
    "        elif e2[0] == 'del':\n"
    "            e = ('sub', e1[1], e2[2], e2[3], e1[4], e1[5] + e2[5])\n"
    "        elif e2[0] == 'sub':\n"
    "            e = ('sub', e1[1], e2[2], e2[3], e1[4] + joiner + e2[4], e1[5] + e2[5])\n"
    "        elif e2[0] == 'noop':\n"
    "            e = ('sub', e1[1], e2[2], e2[3], e1[4] + joiner + e2[4], e1[5] + e2[5])\n"
    "    elif e1[0] == 'del':\n"
    "        if e2[0] == 'ins':\n"
    "            e = ('sub', e1[1], e2[2], e1[3], e2[4], e1[5] + e2[5])\n"
    "        elif e2[0] == 'del':\n"
    "            e = ('del', e1[1], e2[2], e1[3] + joiner + e2[3], '', e1[5] + e2[5])\n"
    "        elif e2[0] == 'sub':\n"
    "            e = ('sub', e1[1], e2[2], e1[3] + joiner + e2[3], e2[4], e1[5] + e2[5])\n"
    "        elif e2[0] == 'noop':\n"
    "            e = ('sub', e1[1], e2[2], e1[3] + joiner +  e2[3], e2[4], e1[5] + e2[5])\n"
    "    elif e1[0] == 'sub':\n"
    "        if e2[0] == 'ins':\n"
    "            e = ('sub', e1[1], e2[2], e1[3], e1[4] + joiner + e2[4], e1[5] + e2[5])\n"
    "        elif e2[0] == 'del':\n"
    "            e = ('sub', e1[1], e2[2], e1[3] + joiner + e2[3], e1[4], e1[5] + e2[5])\n"
    "        elif e2[0] == 'sub':\n"
    "            e = ('sub', e1[1], e2[2], e1[3] + joiner + e2[3], e1[4] + joiner + e2[4], e1[5] + e2[5])\n"
    "        elif e2[0] == 'noop':\n"
    "            e = ('sub', e1[1], e2[2], e1[3] + joiner + e2[3], e1[4] + joiner + e2[4], e1[5] + e2[5])\n"
    "    elif e1[0] == 'noop':\n"
    "        if e2[0] == 'ins':\n"
    "            e = ('sub', e1[1], e2[2], e1[3], e1[4] + joiner + e2[4], e1[5] + e2[5])\n"
    "        elif e2[0] == 'del':\n"
    "            e = ('sub', e1[1], e2[2], e1[3] + joiner + e2[3], e1[4], e1[5] + e2[5])\n"
    "        elif e2[0] == 'sub':\n"
    "            e = ('sub', e1[1], e2[2], e1[3] + joiner + e2[3], e1[4] + joiner + e2[4], e1[5] + e2[5])\n"
    "        elif e2[0] == 'noop':\n"
    "            e = ('noop', e1[1], e2[2], e1[3] + joiner + e2[3], e1[4] + joiner + e2[4], e1[5] + e2[5])\n"
    "    else:\n"
    "        assert False\n"
    "    return e\n"
    "\n"
    "\n"
    "# build edit graph\n"
    "def edit_graph(levi_matrix, backpointers):\n"
    "    V = []\n"
    "    E = []\n"
    "    dist = {}\n"
    "    edits = {}\n"
    "    # breath-first search through the matrix\n"
    "    v_start = (len(levi_matrix)-1, len(levi_matrix[0])-1)\n"
    "    queue = [v_start]\n"
    "    while len(queue) > 0:\n"
    "        v = queue[0]\n"
    "        queue = queue[1:]\n"
    "        if v in V:\n"
    "            continue\n"
    "        V.append(v)\n"
    "        try:\n"
    "            for vnext_edits in backpointers[v]:\n"
    "                vnext = vnext_edits[0]\n"
    "                edit_next = vnext_edits[1]\n"
    "                E.append((vnext, v))\n"
    "                dist[(vnext, v)] = 1\n"
    "                edits[(vnext, v)] = edit_next\n"
    "                if not vnext in queue:\n"
    "                    queue.append(vnext)\n"
    "        except KeyError:\n"
    "            pass\n"
    "    return (V, E, dist, edits)\n"
    "\n"
    "\n"
    "# convenience method for levenshtein distance\n"
    "def levenshtein_distance(first, second):\n"
    "    lmatrix, backpointers = levenshtein_matrix(first, second)\n"
    "    return lmatrix[-1][-1]\n"
    "\n"
    "\n"
    "# levenshtein matrix\n"
    "def levenshtein_matrix(first, second, cost_ins=1, cost_del=1, cost_sub=1):\n"
    "    #if len(second) == 0 or len(second) == 0:\n"
    "    #    return len(first) + len(second)\n"
    "    first_length = len(first) + 1\n"
    "    second_length = len(second) + 1\n"
    "\n"
    "    # init\n"
    "    distance_matrix = [[None] * second_length for x in range(first_length)]\n"
    "    backpointers = {}\n"
    "    distance_matrix[0][0] = 0\n"
    "    for i in range(1, first_length):\n"
    "        distance_matrix[i][0] = i\n"
    "        edit = (\"del\", i-1, i, first[i-1], '', 0)\n"
    "        backpointers[(i, 0)] = [((i-1,0), edit)]\n"
    "    for j in range(1, second_length):\n"
    "        distance_matrix[0][j]=j\n"
    "        edit = (\"ins\", j-1, j-1, '', second[j-1], 0)\n"
    "        backpointers[(0, j)] = [((0,j-1), edit)]\n"
    "\n"
    "    # fill the matrix\n"
    "    for i in xrange(1, first_length):\n"
    "        for j in range(1, second_length):\n"
    "            deletion = distance_matrix[i-1][j] + cost_del\n"
    "            insertion = distance_matrix[i][j-1] + cost_ins\n"
    "            if first[i-1] == second[j-1]:\n"
    "                substitution = distance_matrix[i-1][j-1]\n"
    "            else:\n"
    "                substitution = distance_matrix[i-1][j-1] + cost_sub\n"
    "            if substitution == min(substitution, deletion, insertion):\n"
    "                distance_matrix[i][j] = substitution\n"
    "                if first[i-1] != second[j-1]:\n"
    "                    edit = (\"sub\", i-1, i, first[i-1], second[j-1], 0)\n"
    "                else:\n"
    "                    edit = (\"noop\", i-1, i, first[i-1], second[j-1], 1)\n"
    "                try:\n"
    "                    backpointers[(i, j)].append(((i-1,j-1), edit))\n"
    "                except KeyError:\n"
    "                    backpointers[(i, j)] = [((i-1,j-1), edit)]\n"
    "            if deletion == min(substitution, deletion, insertion):\n"
    "                distance_matrix[i][j] = deletion\n"
    "                edit = (\"del\", i-1, i, first[i-1], '', 0)\n"
    "                try:\n"
    "                    backpointers[(i, j)].append(((i-1,j), edit))\n"
    "                except KeyError:\n"
    "                    backpointers[(i, j)] = [((i-1,j), edit)]\n"
    "            if insertion == min(substitution, deletion, insertion):\n"
    "                distance_matrix[i][j] = insertion\n"
    "                edit = (\"ins\", i, i, '', second[j-1], 0)\n"
    "                try:\n"
    "                    backpointers[(i, j)].append(((i,j-1), edit))\n"
    "                except KeyError:\n"
    "                    backpointers[(i, j)] = [((i,j-1), edit)]\n"
    "    return (distance_matrix, backpointers)\n"
    "\n"
    "def smart_open(fname, mode = 'r'):\n"
    "    if fname.endswith('.gz'):\n"
    "        import gzip\n"
    "        # Using max compression (9) by default seems to be slow.\n"
    "        # Let's try using the fastest.\n"
    "        return gzip.open(fname, mode, 1)\n"
    "    else:\n"
    "        return open(fname, mode)\n"
    "\n"
    "def paragraphs(lines, is_separator=lambda x : x == '\\n', joiner=''.join):\n"
    "    paragraph = []\n"
    "    for line in lines:\n"
    "        if is_separator(line):\n"
    "            if paragraph:\n"
    "                yield joiner(paragraph)\n"
    "                paragraph = []\n"
    "        else:\n"
    "            paragraph.append(line)\n"
    "    if paragraph:\n"
    "        yield joiner(paragraph)\n"
    "\n"
    "def uniq(seq, idfun=None):\n"
    "    # order preserving\n"
    "    if idfun is None:\n"
    "        def idfun(x): return x\n"
    "    seen = {}\n"
    "    result = []\n"
    "    for item in seq:\n"
    "        marker = idfun(item)\n"
    "        # in old Python versions:\n"
    "        # if seen.has_key(marker)\n"
    "        # but in new ones:\n"
    "        if marker in seen: continue\n"
    "        seen[marker] = 1\n"
    "        result.append(item)\n"
    "\n"
    "\n"
    "class M2ObjType:\n"
    "    pass\n"
    "\n"
    "class M2Obj(M2ObjType):\n"
    "\n"
    "    def __init__(self, m2_path, beta=1.0, max_unchanged_words=2,\n"
    "                 ignore_whitespace_casing=False):\n"
    "        self.m2_path = m2_path\n"
    "        self.beta = beta\n"
    "        self.max_unchanged_words = max_unchanged_words\n"
    "        self.ignore_whitespace_casing = ignore_whitespace_casing\n"
    "\n"
    "        self.source_sentences, self.gold_edits = load_annotation(m2_path)\n"
    "\n"
    "    def sufstats(self, cand_str, i):\n"
    "        stat_correct, stat_proposed, stat_gold = f1_suffstats(cand_str,\n"
    "                                                self.source_sentences[i],\n"
    "                                                self.gold_edits[i],\n"
    "                                                self.max_unchanged_words,\n"
    "                                                self.beta,\n"
    "                                                self.ignore_whitespace_casing)\n"
    "\n"
    "        return [int(stat_correct), int(stat_proposed), int(stat_gold)]\n"
  ;
}

}
