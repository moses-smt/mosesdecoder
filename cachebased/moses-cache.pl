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
# cat inputfile | perl moses-cache.pl "../bin/moses -f moses.ini"
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


IPC::Open3::open3 (\*MOSESIN, \*MOSESOUT, \*MOSESERR, "$ARGV[0] -t");

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

    my @phrases = @$info;
    my @attrs = map { "trg$_=\"" . $phrases[$_-1] . "\"" } (1..scalar(@phrases));
    my $xml_line = "<dlt len=\"" . scalar(@phrases) . "\" " . join(" ", @attrs) . "/> " . $line;
    return $xml_line;
}

# Take the lines of the standard output and error from the finished translation
# task and produce information to be stored for the next translation task
# (currently, the list of tokens).
sub process_output {
    my ($out, $translation, $err) = @_;
    my @words = split(/ /, $translation);
    return \@words;

    # the following code builds the list of phrases (instead of tokens)
    # of the translation    
    #my @phrases = split(/\|[0-9]+-[0-9]+\|/, $out);
    #@phrases = map { trim($_); } @phrases; 
    #return \@phrases;    
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
    my $output = <MOSESOUT>;
    $output = trim($output);
    my $translation = $output;
    $translation =~ s/ \|[0-9]+-[0-9]+\|//g;
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
    $info = process_output($output, $translation, $moreinfo);
}

close MOSESIN;
close MOSESOUT;
close MOSESERR;
