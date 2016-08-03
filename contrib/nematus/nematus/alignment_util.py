__author__ = 'canliu'
"""
Save the alignment matrix in XML format. Like the following:
<sentence>
    <source> </source>
    <target> <target>
    <alignment>
        <sourceword> x,x,x... </sourceword>
        <sourceword> x,x,x... </sourceword>
    </alignment>
</sentence>

The number of rows is equal to the number of target_words + 1.
The number of columns is equal to the number of source_words + 1.
"""
import json
import sys
import codecs

def get_alignments(attention, x_mask, y_mask):
    #print "\nPrinting Attention..."
    #print attention
    #print "\nPrinting x_mask, need to figure out how to use it"
    #print x_mask
    #print "\nPrinting y_mask, need to figure out how to use it"
    #print y_mask

    n_rows, n_cols = y_mask.shape  ###n_cols correspond to the number of sentences.
    #print "Number of rows and number of columns: \n\n", n_rows, n_cols

    for target_sent_index in range(n_cols):
        #print "\n\n","*" * 40
        print "Going through sentence", target_sent_index
        #source_sent_index = source_indexes[target_sent_index]
        target_length = y_mask[:,target_sent_index].tolist().count(1)
        source_length = x_mask[:,target_sent_index].tolist().count(1)
        # #print "STEP1: The attention matrix that is relevant for this sentence",
        temp_attention = attention[range(target_length),:,:]
        #print "STEP2: The attention matrix that is particular to just this sentence\n",
        this_attention = temp_attention[:,target_sent_index,range(source_length)]

        jdata = {}
        jdata['matrix'] = this_attention.tolist()
        jdata = json.dumps(jdata)
        #print "\t\tJSON Data"
        #print "\t\t",jdata
        yield jdata

def combine_source_target_text(source_IN, nbest_IN, saveto, alignment_IN):
    """
    there can be multiple target sentences, aligned to the same source sentence.
    """
    source_IN.seek(0)
    nbest_IN.seek(0)
    alignment_IN.seek(0)

    with open(saveto + "_withwords.json", "w") as alignment_OUT:
        all_matrixes = alignment_IN.readlines()
        nbest_lines = nbest_IN.readlines()
        source_lines = source_IN.readlines()
        assert len(all_matrixes) == len(nbest_lines), "The number of lines does not match with each other!"

        for target_index in range(len(all_matrixes)):
            jdata = json.loads(all_matrixes[target_index])
            target_line = nbest_lines[target_index]
            elements = target_line.strip().split("|||")
            refer_index = int(elements[0].strip())
            source_sent = source_lines[refer_index].strip()
            target_sent = elements[1].strip()

            jdata["source_sent"] = source_sent
            jdata["target_sent"] = target_sent
            jdata["id"] = refer_index
            jdata["prob"] = 0 #float(elements[2].strip().split()[1])

            #jdata = json.dumps(jdata)
            jdata = json.dumps(jdata).decode('unicode-escape').encode('utf8')
            alignment_OUT.write(jdata + "\n")

def combine_source_target_text_1to1(source_IN, target_IN, saveto, alignment_IN):
    """
    There is a 1-1 mapping of target and source sentence.
    """
    source_IN.seek(0)
    target_IN.seek(0)
    alignment_IN.seek(0)
    with open(saveto + "_withwords.json", "w") as alignment_OUT:

        all_matrixes = alignment_IN.readlines()
        target_lines = target_IN.readlines()
        source_lines = source_IN.readlines()
        assert len(all_matrixes) == len(target_lines), "The number of lines does not match with each other!"

        for target_index in range(len(all_matrixes)):
            jdata = json.loads(all_matrixes[target_index])

            jdata["source_sent"] = source_lines[target_index].strip()
            jdata["target_sent"] = target_lines[target_index].strip()
            jdata["id"] = target_index
            jdata["prob"] = 0 #float(elements[2].strip().split()[1])

            #jdata = json.dumps(jdata)
            jdata = json.dumps(jdata).decode('unicode-escape').encode('utf8')
            alignment_OUT.write(jdata + "\n")


def convert_to_nodes_edges_v1(filename):
    """
    Take as input the aligned file with file names ".withtext", and convert this into a file with nodes and edges.
    Which will later used for Visualization.
    """
    with open(filename, "r") as IN:
        with open(filename + ".forweb" , "w") as OUT:
            in_lines = IN.readlines()
            for data in in_lines:
                data4web = convert_to_nodes_edges_each_v1(data)
                OUT.write(data4web + "\n")

def convert_to_nodes_edges_each_v1(data):
    """
    give a single data object string, convert it into a json data string that is compatible with the Web interface.
    """
    jdata = json.loads(data)
    web_data = {}
    source_words = jdata["source_sent"].strip().split()
    target_words = jdata["target_sent"].strip().split()

    ###make the data for source and target words
    web_data["nodes"] = []
    for word in source_words:
        web_data["nodes"].append({"name":word, "group": 1})
    web_data["nodes"].append({"name":"<EOS", "group": 1})
    for word in target_words:
        web_data["nodes"].append({"name":word, "group": 2})
    web_data["nodes"].append({"name":"<EOS", "group": 2})

    matrix = jdata["matrix"]
    n_rows = len(matrix)
    n_cols = len(matrix[0])

    web_data["links"] = []
    for target_index in range(n_rows):
        for source_index in range(n_cols):
            if target_index == (n_rows-1):
                target_word = "<EOS>"
            else:
                target_word = target_words[target_index]
            if source_index == (n_cols-1):
                source_word = "<EOS>"
            else:
                source_word = source_words[source_index]
            score = matrix[target_index][source_index]
            web_data["links"].append( {"source": source_word, "target": target_word, "value": score} )

    web_data = json.dumps(web_data).decode('unicode-escape').encode('utf8')
    #print web_data, "\n\n"
    return web_data

def convert_to_nodes_edges_v2(filename):
    """
    Take as input the aligned file with file names ".withtext", and convert this into a file with nodes and edges.
    Which will later used for Visualization.
    """
    with codecs.open(filename, "r", encoding="UTF-8") as IN:
        with open(filename + ".forweb" , "w") as OUT:
            in_lines = IN.readlines()
            source_list = []
            target_list = []
            all_links = []
            for sent_id in range(len(in_lines)):
                data = in_lines[sent_id]
                #print data
                source_sent, target_sent, links = convert_to_nodes_edges_each_v2(data, sent_id)
                source_list.append(source_sent)
                target_list.append(target_sent)
                all_links += links

            jdata = {}
            jdata["source_list"] = source_list
            jdata["target_list"] = target_list
            jdata["links"] = all_links
            jdata = json.dumps(jdata).decode('unicode-escape').encode('utf8')

            OUT.write(jdata)

def convert_to_nodes_edges_each_v2(data, sent_id):
    """
    give a single data object string, convert it into a json data string that is compatible with the Web interface.
    """
    #print data
    jdata = json.loads(data)
    #jdata = json.loads(json.dumps(jdata).decode('unicode-escape').encode('utf8'))
    print jdata
    source_words = jdata["source_sent"].encode('unicode-escape').strip().split()
    source_words.append("EOS")
    target_words = jdata["target_sent"].strip().split()
    #print target_words
    target_words.append("EOS")

    #print target_words

    matrix = jdata["matrix"]
    n_rows = len(matrix)
    n_cols = len(matrix[0])

    links = []
    for target_index in range(n_rows):
        for source_index in range(n_cols):
            five_tuple = []
            score = matrix[target_index][source_index]
            five_tuple.append(target_index)
            five_tuple.append(sent_id)
            five_tuple.append(score)
            five_tuple.append(source_index)
            five_tuple.append(sent_id)
            links.append(five_tuple)

    return source_words, target_words, links

if __name__ == "__main__":
    """
    Run the conversion to Web format if needed.
    """
    input_file = sys.argv[1]
    convert_to_nodes_edges_v2(input_file)

    import sys
    reload(sys)
    sys.setdefaultencoding('utf-8')
### Json for web visuaslization format version 1.
### This corresponds to convert_to_nodes_edges_each_v1; and convert_to_nodes_edges_v1.
"""
{
  "nodes":[
        {"name":"Good","group":1},
        {"name":"Morning","group":1},
        {"name":"Buenos","group":2},
        {"name":"dias","group":2}
    ],
  "links":[
        {"source":"Good" ,"target":"Buenos","value":0.90},
        {"source":"Good" ,"target":"dias","value":0.30},
        {"source":"Morning" ,"target":"Buenos","value":0.50},
        {"source":"Morning" ,"target":"dias","value":0.95}
    ]
}
"""

### Json for Web visualization format version 2.
### This corresponds to

