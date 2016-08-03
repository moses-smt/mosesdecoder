import sys
import argparse

# given a source sentence, a target sentence, and a sequence of probabilities (one per target word, plus an end-of-sentence probability),
# visualize the probability of each target word via HTML output.
# black fields indicate high confidence, light fields low confidence.
# example input:
"""
Unsere digitalen Leben haben die Notwendigkeit, stark, lebenslustig und erfolgreich zu erscheinen, verdoppelt.
Our digital lives have doubled the need to appear strong, lifel... ike and successful .
0.882218956947 0.989946246147 0.793388187885 0.790167689323 0.768674969673 0.941913545132 0.955783545971 0.777168631554 0.266917765141 0.909709095955 0.990240097046 0.341023534536 0.828059256077 0.854399263859 0.906807541847 0.960786998272 0.997184157372"""

html_text = """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html>
<HEAD>
<title>Results page</title>
<meta http-equiv=Content-Type content=text/html; charset=UTF8>
<style>
html, body, pre{{
background-color: #FFFFFF;
color: #FFFFFF;
font-family: Arial, Helvetica, sans-serif;
font-size: 20px;
}}
td {{
min-width:100px;
}}
th {{
color: #000000;
text-align: left;
}}
</style>
</head>
\n
\n
<body>
 <table>
  {0}
</table> 

</body>
</html>
"""


def print_probdist(infile, outfile):

    entries = []

    for i, line in enumerate(infile):
        if i % 3 == 0:
            #words = line.split()
            entry = ""
            #for w in words:
                #entry += "<th>" + w + "</thr>\n"
            entry = "<tr><th colspan=\"0\">" + line + "</th></tr>\n"
            entries.append(entry)

        if i % 3 == 1:
            words = line.split()
            words.append('&lt;/s&gt;')
        elif i % 3 == 2:
            probs = map(float, line.split())
            entry = ""
            for w,p in zip(words, probs):
                color = '#%02x%02x%02x' % (int((1-p)*255), int((1-p)*255), int((1-p)*255))
                entry += "<td bgcolor=\"{0}\">{1}</td>".format(color, w)
            entry = "<tr>" + entry + "</tr>\n"
            entries.append(entry)


    outfile.write(html_text.format('\n'.join(entries)))


parser = argparse.ArgumentParser()
parser.add_argument('--input', '-i', type=argparse.FileType('r'),
                        default=sys.stdin, metavar='PATH',
                        help="Input file (default: standard input)")
parser.add_argument('--output', '-o', type=argparse.FileType('w'),
                        default=sys.stdout, metavar='PATH',
                        help="Output file (default: standard output)")

args = parser.parse_args()

print_probdist(args.input, args.output)