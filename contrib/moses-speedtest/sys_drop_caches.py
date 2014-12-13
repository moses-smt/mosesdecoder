#!/usr/bin/spython
from sys import argv, stderr, exit
from os import linesep as ls
procfile = "/proc/sys/vm/drop_caches"
options = ["1","2","3"]
flush_type = None
try:
    flush_type = argv[1][0:1] 
    if not flush_type in options:
        raise IndexError, "not in options"
    with open(procfile, "w") as f:
        f.write("%s%s" % (flush_type,ls))
    exit(0)
except IndexError, e:
    stderr.write("Argument %s required.%s" % (options, ls))
except IOError, e:
    stderr.write("Error writing to file.%s" % ls)
except StandardError, e:
    stderr.write("Unknown Error.%s" % ls)

exit(1)

