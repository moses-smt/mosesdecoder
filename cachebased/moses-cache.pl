#!/usr/bin/perl

# CACHE-BASED DOCUMENT-LEVEL TRANSLATION
#
# Wrapper script for Moses that takes information from the translation
# of a sentence (currently, the target language tokens) and includes them
# as an XML tag into the input of following sentence.
#
# The moses command-line has to be given as an argument to this script.
# The sentences to translate have to be given line-by-line at the standard input. 
#
# Example:
# cat inputfile | perl moses-cache.pl "../bin/moses -f moses.ini -v 2"
#
#

use strict;
use warnings;
use FileHandle;
use IPC::Open3;

if ($#ARGV+1 < 1) {
    print "Usage: $0 moses-cmd-line\n";
    exit -1;
}


IPC::Open3::open3 (\*MOSESIN, \*MOSESOUT, \*MOSESERR, $ARGV[0]);

# remove spaces at begin and end, and newline
sub trim {
    my $str = shift;
    chomp $str;
    $str =~ s/^\s+//;
    $str =~ s/\s+$//; 
    return $str;
}

# add to the input line of moses (first parameter) some XML information
# built from the array of tokens of the previous translation (second parameter)  
sub add_xml_info {
    my ($line, $info) = @_;
    my $attrlist = "";
    if (!defined $info) {
	return $line;
    }

    my @words = @$info;
    my @attrs = map { "trg$_=\"" . $words[$_-1] . "\"" } (1..scalar(@words));
    my $xml_line = "<dlt len=\"" . scalar(@words) . "\" " . join(" ", @attrs) . "/> " . $line;
    return $xml_line;
}

# Take the lines of the standard output and error from the finished translation
# task and produce information to be stored for the next translation task
# (currently, the list of tokens).
sub process_output {
    my ($out, $err) = @_;
    my @words = split(/ /, $out);
    return \@words;    
}

# The following variable is an opaque pointer containing the data that is
# preserved between sentence translations. It is used by add_xml_info
# and process_output.
my $info;


# The main input/output loop
while (my $line = <STDIN>) {
    $line = trim($line);
    my $xml_line = add_xml_info($line, $info);
    print "Next input: $xml_line\n";
    print MOSESIN "$xml_line\n";
    my $translation = <MOSESOUT>;
    $translation = trim($translation);
    print "Output: $translation\n";
    my $moreinfo = "";
    my $hasmoreinfo = 1;
    while ($hasmoreinfo) {
        my $errline = <MOSESERR>;
	$moreinfo .= $errline;
	if ($errline =~ /^Line (.*) total$/) {
	    $hasmoreinfo = 0;
	}
    } 
    $info = process_output($translation, $moreinfo);
}

close MOSESIN;
close MOSESOUT;
close MOSESERR;
