#!/usr/bin/env python

# Combines the system definition from one .ini file with the weights contained
# in another. Works for the new moses.ini format with fully named feature
# functions. Writes the new .ini file to stdout
# Script by Ulrich Germann.

import re,sys,os
from optparse import OptionParser

SectionHeaderPattern = re.compile(r'^\[(.*)\]\s*$')
def read_ini(filename):
    ''' 
    Reads a moses.ini file and returns a dictionary mapping 
    from section names to a list of lines contained in that section.
    '''
    AllSections = {}
    CurSection  = AllSections.setdefault('',[])
    for line in open(filename):
        line = line.strip()
        m = SectionHeaderPattern.match(line)
        if m:
            CurSection = AllSections.setdefault(m.group(1),[])
        elif len(line):
            CurSection.append(line)
            pass
        pass
    return AllSections

parser = OptionParser()
parser.add_option("-s", "--system", dest = "system",
                  help = "moses.ini file defining the system")
parser.add_option("-w", "--weights", dest = "weight",
                  help = "moses.ini file defining the system")

opts,args = parser.parse_args()

system = read_ini(opts.system)
weight = read_ini(opts.weight)

for s in system:
    if len(s) == 0 or s[0:6] == 'weight': continue
    print "[%s]"%s
    print "\n".join(system[s])
    print
    pass

if 'weight' in weight:
    print '[weight]'
    print "\n".join(weight['weight'])
else:
    for s in weight:
        if s[0:6] != 'weight': continue
        print "[%s]"%s
        print "\n".join(system[s])
        print
        pass
    pass



