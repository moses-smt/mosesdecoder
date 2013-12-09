#!/usr/bin/env python
#
# Picaro: An simple command-line alignment visualization tool.
#
# picaro.py
# Visualize alignments between sentences in a grid format.
#
# Jason Riesa <riesa@isi.edu>
# version: 01-16-2010
#
# Copyright (C) 2013 Jason Riesa
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

import sys, os, commands
from collections import defaultdict

#TC_BIN = "tc/tc.linux32"

a1_file_str = ""
a2_file_str = ""
f_file_str = ""
e_file_str = ""
SHOW_TC_A1 = 0
SHOW_TC_A2 = 0
maxlen = float('inf')

# Process command line options
try:
    while len(sys.argv) > 1:
        option = sys.argv[1];           del sys.argv[1]
        if  option == '-a1':
            a1_file_str = sys.argv[1];  del sys.argv[1]
        elif option == '-a2':
            a2_file_str = sys.argv[1];  del sys.argv[1]
        elif option == '-f':
            f_file_str = sys.argv[1];   del sys.argv[1]
        elif option == '-e':
            e_file_str = sys.argv[1];   del sys.argv[1]
	elif option == '-maxlen':
	    maxlen = int(sys.argv[1]);  del sys.argv[1]
        else:
            sys.stderr.write("Invalid option: %s\n" % (option))
            sys.exit(1)
	'''
        elif option == '-tc':
            if sys.argv[1] == '1':
                SHOW_TC_A1 = 1; del sys.argv[1]
            elif sys.argv[1] == '2':
                SHOW_TC_A2 = 2; del sys.argv[1]
            else:
                raise Exception, "Invalid argument to option -tc"
	'''

    if a1_file_str == "" or f_file_str == "" or e_file_str == "":
        raise Exception, "Not all options properly specified."
    # Make sure transitive closure binary exists if user has enabled this option
    if SHOW_TC_A1 or SHOW_TC_A2:
        if not os.path.exists(TC_BIN):
            raise Exception, "Transitive closure binary "+TC_BIN+" not found."
except Exception, msg:
    sys.stderr.write("%s: %s\n" % (sys.argv[0], msg))
    sys.stderr.write("Usage: %s: -a1 <alignment1> -f <f> -e <e> [-a2 <alignment2>]\n" % (sys.argv[0]))
    sys.stderr.write("Mandatory arguments:\n")
    sys.stderr.write(" -a1 <a1>\t path to alignment 1 file in f-e format\n")
    sys.stderr.write(" -f <f>\t\t path to source text f\n")
    sys.stderr.write(" -e <e>\t\t path to target text e\n")
    sys.stderr.write("Optional arguments:\n")
    sys.stderr.write(" -a2 <a2>\t path to alignment 2 file in f-e format\n")
    sys.stderr.write(" -maxlen <len>\t display alignment only when e and f have length <= len\n")
    sys.exit(1)

    
a_file = open(a1_file_str, 'r')
f_file = open(f_file_str, 'r')
e_file = open(e_file_str, 'r')
if a2_file_str != "":
    a2_file = open(a2_file_str, 'r')
    
sentenceNumber = 0
nextRequested = 1
for aline in a_file:
    eline = e_file.readline()
    fline = f_file.readline()
    if a2_file_str != "":
        a2line = a2_file.readline()
        
    links = aline.split()
    e_words = eline.split()
    f_words = fline.split()
    if a2_file_str != "":
        links2 = a2line.split()
        
    # Get transitive closure of links and links2
    if SHOW_TC_A1:
        cmd = 'echo "' + ' '.join(links) + '" | ' + TC_BIN
        failure1, output1 = commands.getstatusoutput(cmd)
        tc1 = output1.split()
    if SHOW_TC_A2:
        cmd = 'echo "' + ' '.join(links2) + '" | ' + TC_BIN
        failure2, output2 = commands.getstatusoutput(cmd)
        tc2 = output2.split()
    
    # Update tracking counts    
    sentenceNumber += 1
    if sentenceNumber < nextRequested:
        continue

    # Don't generate alignment grids for very large sentences
    if len(e_words) > maxlen or len(f_words) > maxlen:
        continue
    
    
    print "== SENTENCE ",sentenceNumber," =="

    # Initialize alignment objects
    # a holds alignments of user-specified -a1 <file>
    # a2 holds alignments of user-specified -a2 <file>
    a = defaultdict(lambda: defaultdict(int))     
    a2 = defaultdict(lambda: defaultdict(int))     
    
    # Print e_words on the columns
    # First, find the length of the longest word
    longestEWordSize = 0
    longestEWord = 0
    for w in e_words:
        if len(w) > longestEWordSize:
            longestEWordSize = len(w)
            longestEWord = w
   
    # Now, print the e-words
    for i in range(longestEWordSize, 0, -1):
        for w in e_words:
            if len(w) < i:
                print " ",
            else:
                print w[(i*-1)],
        print
        
    
    # Fill in alignment matrix 1
    for link in links:
        i, j = map(int, link.split('-'))
        a[int(i)][int(j)] = 1
    # Fill in extra links added by transitive closure
    if SHOW_TC_A1:
        for link in tc1:
            i, j = map(int, link.split('-'))
            if(a[i][j] != 1):
                a[i][j] = 2
        
    # Fill in alignment matrix 2
    if(a2_file_str != ""):
        for link in links2:
            i, j = map(int, link.split('-'))
            a2[i][j] = 1
        # Fill in extra links added by transitive closure
        if SHOW_TC_A2:
            for link in tc2:
                i, j = map(int, link.split('-'))
                if(a2[i][j] != 1):
                    a2[i][j] = 2

    # Print filled-in alignment matrix
    if a2_file_str == "":
        for i, _ in enumerate(f_words):
            for j, _ in enumerate(e_words):
                val1 = a[i][j]
                if val1 == 0:
                    # No link
                    print ':',
                elif val1 == 1:
                    # Regular link
                    print u'\u001b[44m\u0020\u001b[0m',
                elif val1 == 2:
                    # Link due to transitive closure
                    # Render as gray-shaded square
                    print 'O',
            print f_words[i]
        print
    else:
        for i, _ in enumerate(f_words):
            for j, _ in enumerate(e_words):
                val1 = a[i][j]
                val2 = a2[i][j]
                
                if val1 == 0 and val2 == 0:
                    # Link not in a nor a2
                    # Empty grid box
                    print ':',
                # Link in both a and a2
                elif val1 > 0 and val2 > 0:
                    # Green box
                    if val1 == 1:
                        if val2 == 1:
                            print u'\u001b[42m\u001b[1m\u0020\u001b[0m',
                        elif val2 == 2:
                            print u'\u001b[42m\u001b[30m2\u001b[0m',
                    elif val1 == 2:
                        if val2 == 1:
                            print u'\u001b[42m\u0020\u001b[0m',
                        elif val2 == 2:
                            print u'\u001b[42m\u001b[30m3\u001b[0m',
                # Link in a2, but not a
                elif val1 == 0 and val2 > 0:
                    if val2 == 1:
                        # Yellow box
                        print u'\u001b[1m\u001b[43m\u0020\u001b[0m',
                    elif val2 == 2:
                        # Artificial link by transitive closure
                        print u'\u001b[43m\u001b[30m2\u001b[0m',
                
                # Link in a, but not a2
                elif val1 > 0 and val2 == 0:
                    if val1 == 1:
                        # Blue box
                        print u'\u001b[1m\u001b[44m\u0020\u001b[0m',
                    elif val1 == 2:
                        print u'\u001b[44m\u001b[37m1\u001b[0m',
            print f_words[i]
    nextDefault = sentenceNumber + 1
    sys.stdout.write("Enter next alignment number or 'q' to quit [%d]: " %(nextDefault))
    user_input = sys.stdin.readline().strip()
    if user_input == "":
        nextRequested = nextDefault
    elif user_input[0] == "q" or user_input == "quit":
        sys.exit(1)
    else:
        try:
            nextRequested = int(user_input)
        except:
            nextRequested = sentenceNumber + 1
            sys.stdout.write("Unknown alignment id: %s\nContinuing with %d.\n" %(user_input, nextRequested))

a_file.close()
e_file.close()
f_file.close()

