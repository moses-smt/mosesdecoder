DIMwid
 ======

DIMwid (Decoder Inspection for Moses using widgets) is a tool
presenting Moses' different chart/stack outputs in a readable tabular
view.


Installation
============

In order to run DIMwid you need to install PyQt, Qt 4.8 and Python
2.7. Other versions have not yet been tested.  Linux/Unix users simply
install these packages using their package-manager or built them from
source.  Windows can skip the installation of Qt since PyQt itself
does cover everything, except Python.

Usage
=====

Users are recommended to read the accompanying paper "DIMwid --
Decoder Inspection for Moses (using Widgets)" appearing in PBML XY.

DIMwid is able to read multiple decoder outputs of the Moses
translation system. These include the standard trace outputs for both
phrase- and syntax-based decoding, the search-graphs for both, the
"level 3 verbose" output for phrase-based and a special trace output
(available as a Moses fork at :
https://github.com/RobinQrtz/mosesdecoder) for all possible
translations for syntax-based decoding.

After producing the outputs from Moses, start DIMwid by running
DIMwid.py and first select your format and after that your file. If
you have chosen the wrong file or format an error message will
appear. Otherwise you will see the first sentence. Cells can be
inspected by either double-clicking, opening a new window with the
full content, or hovering over the cell, showing a tooltip with the
first 20 lines of the cell's content.

If needed, the user can restrict the number of rules per cell, using
the "Cell Limit" spinbox.

Navigating through the sentences of the input file can be done by
either using the "Next" and "Prev" buttons, or choosing a certain
sentence number using the lower left spinbox and clicking the "GoTo"
button.

Moses
=====

Information about Moses can be found here: http://statmt.org/moses/

The used flags for the output are:
    * -t for phrase-based trace
    * -T for syntax-based trace
    * -v 3 for phrase-based verbose level 3
    * -output-search-graph for both search graphs
    * -Tall for the Moses fork's new feature


Trouble
=======

If you are running into trouble using DIMwid or have suggestions for
improvements or new features email me at 

robin DOT qrtz AT gmail DOT com