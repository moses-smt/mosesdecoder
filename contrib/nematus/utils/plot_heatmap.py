import numpy
import matplotlib.pyplot as plt
import sys
import json
import argparse

# input:
#  alignment matrix - numpy array
#  shape (target tokens + eos, number of hidden source states = source tokens +eos)
# one line correpsonds to one decoding step producing one target token
# each line has the attention model weights corresponding to that decoding step
# each float on a line is the attention model weight for a corresponding source state.
# plot: a heat map of the alignment matrix
# x axis are the source tokens (alignment is to source hidden state that roughly corresponds to a source token)
# y axis are the target tokens

# http://stackoverflow.com/questions/14391959/heatmap-in-matplotlib-with-pcolor
def plot_head_map(mma, target_labels, source_labels):
  fig, ax = plt.subplots()
  heatmap = ax.pcolor(mma, cmap=plt.cm.Blues)

  # put the major ticks at the middle of each cell
  ax.set_xticks(numpy.arange(mma.shape[1])+0.5, minor=False)
  ax.set_yticks(numpy.arange(mma.shape[0])+0.5, minor=False)
  
  # without this I get some extra columns rows
  # http://stackoverflow.com/questions/31601351/why-does-this-matplotlib-heatmap-have-an-extra-blank-column
  ax.set_xlim(0, int(mma.shape[1]))
  ax.set_ylim(0, int(mma.shape[0]))

  # want a more natural, table-like display
  ax.invert_yaxis()
  ax.xaxis.tick_top()

  # source words -> column labels
  ax.set_xticklabels(source_labels, minor=False)
  # target words -> row labels
  ax.set_yticklabels(target_labels, minor=False)
  
  plt.xticks(rotation=45)

  #plt.tight_layout()
  plt.show()

# column labels -> target words
# row labels -> source words

def read_alignment_matrix(f):
  header = f.readline().strip().split('|||')
  if header[0] == '':
    return None, None, None, None
  sid = int(header[0].strip())
  # number of tokens in source and translation +1 for eos
  src_count, trg_count = map(int,header[-1].split())
  # source words
  source_labels = header[3].decode('UTF-8').split()
  source_labels.append('</s>')
  # target words
  target_labels = header[1].decode('UTF-8').split()
  target_labels.append('</s>')

  mm = []
  for r in range(trg_count):
    alignment = map(float,f.readline().strip().split())
    mm.append(alignment)
  mma = numpy.array(mm)
  return sid,mma, target_labels, source_labels


def read_plot_alignment_matrices(f, n):
  while(f):
    sid, mma, target_labels, source_labels = read_alignment_matrix(f)
    if mma is None:
      return
    if sid >n:
      return
    plot_head_map(mma, target_labels, source_labels)
    # empty line separating the matrices
    f.readline()


"""
Adding functions to read the json format.
"""

def read_plot_alignment_json(file, n):
    while (file):
        sid, mma, target_labels, source_labels = read_alignment_json(file)
        if mma is None:
            return
        if sid > n:
            return
        plot_head_map(mma, target_labels, source_labels)

def read_alignment_json(file):
    data = file.readline() ##one line containing the json object.
    if len(data.strip()) == 0:
        return None, None, None, None
    jdata = json.loads(data)
    ## messy json encodings... TODO: make this better
    jdata = json.loads(json.dumps(jdata).decode('unicode-escape').encode('utf8'))
    #print jdata
    sid = int(jdata["id"])
    mma = numpy.array(jdata["matrix"])
    ##target words
    target_labels = jdata["target_sent"].split()
    target_labels.append('</s>')
    ##source words
    source_labels = jdata["source_sent"].split()
    source_labels.append('</s>')
    return sid,mma, target_labels, source_labels

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    # '/Users/mnadejde/Documents/workspace/MTMA2016/models/wmt16_systems/en-de/test.alignment'
    parser.add_argument('--input', '-i', type=argparse.FileType('r'),
                            default='/Users/mnadejde/Documents/workspace/MTMA2016/models/wmt16_systems/ro-en/newstest2016-roen-src.ro.alignment', metavar='PATH',
                            help="Input file (default: standard input)")

    parser.add_argument('--json', '-j', required = False,action="store_true",
                            help="If this option is used, then read alignment matrix from a Json formatted file.")
    args = parser.parse_args()

    if args.json:
        read_plot_alignment_json(args.input, 10)   ##n is the maximum number of sentences to process.
    else:
        read_plot_alignment_matrices(args.input,10)
>>>>>>> origin/nematus-liucan
