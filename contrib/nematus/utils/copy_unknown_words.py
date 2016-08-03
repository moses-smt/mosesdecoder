# -*- coding: utf8 -*-
'''
This script is to replace the unknown words in target sentences with their aligned words in source sentences.
Args: 
	- input: a text file (json format), each line 
			including a full alignment matrix, a pair of source and target sentences
	- output (optional): updated text file (json format)
	- unknown word token (optional): a string, default="UNK"
To use:
	python copy_unknown_words.py -i translation.txt -o updated_translation.txt -u 'UNK'
'''

import json
import numpy
import argparse
import sys

''' 
Example input file:
{"id": 0, "prob": 0, "target_sent": "Obama empfängt Netanjahu", "matrix": [[0.9239920377731323, 0.04680762067437172, 0.003626488381996751, 0.02343202754855156, 0.0021418146789073944], [0.009942686185240746, 0.4995519518852234, 0.44341862201690674, 0.02077348716557026, 0.026313267648220062], [0.01032756082713604, 0.6475557088851929, 0.029476342722773552, 0.27724361419677734, 0.035396818071603775], [0.0010026689851656556, 0.35200807452201843, 0.06362949311733246, 0.4778701961040497, 0.1054895892739296]], "source_sent": "Obama kindly receives Netanjahu"}
'''

def copy_unknown_words(filename, out_filename, unk_token):
	for line in filename:
		sent_pair = json.loads(line)
# 		print "Translation:"
# 		print sent_pair
		source_sent = sent_pair["source_sent"]
		target_sent = sent_pair["target_sent"]
		# matrix dimension: (len(target_sent) + 1) * (len(source_sent) + 1)
		# sum of values in a row = 1
		full_alignment = sent_pair["matrix"]
		source_words = source_sent.split()
		target_words = target_sent.split()
		# get the indices of maximum values in each row 
		# (best alignment for each target word)
		hard_alignment = numpy.argmax(full_alignment, axis=1)
# 		print hard_alignment
		
		updated_target_words = []
		for j in xrange(len(target_words)):
			if target_words[j] == unk_token:
				unk_source = source_words[hard_alignment[j]]
				updated_target_words.append(unk_source)
			else:
				updated_target_words.append(target_words[j])
				
		sent_pair["target_sent"] = " ".join(updated_target_words)
# 		print "Updated translation:"
# 		print sent_pair
		sent_pair = json.dumps(sent_pair).decode('unicode-escape').encode('utf8')
		print >>out_filename, sent_pair

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument('--input', '-i', type=argparse.FileType('r'),
						metavar='PATH', required=True,
						help='''Input text file in json format including alignment matrix, 
								source sentences, target sentences''')
	parser.add_argument('--output', '-o', type=argparse.FileType('w'),
						default=sys.stdout, metavar='PATH',
						help="Output file (default: standard output)")
	parser.add_argument('--unknown', '-u', type=str, nargs = '?', default="UNK",
						help='Unknown token to be replaced (default: "UNK")')

	args = parser.parse_args()
		
	copy_unknown_words(args.input, args.output, args.unk)
		
	