#!/usr/bin/perl -w

#parse-en-berkeleyparser.perl - run berkeleyparser on tokenized segments
#gets input from STDIN
#writes syntax trees wrapped in tags to STDOUT
#example: cat ~/europarl/xaaa | ./parse-en-berkeleyparser.perl > ~/europarl/grammar-berkeleyparser.e

use strict;
use warnings;
use List::MoreUtils qw(first_index  last_index );use Carp;

#berkeleyparser commandline
my $grammar = "/home/john/berkeleyparser/eng_sm6.gr";
my $berkeleyparser = "java -jar /home/john/berkeleyparser/trunk/BerkeleyParser.jar -gr $grammar";

open my $PARSE, "$berkeleyparser|";

while ( my $line = <$PARSE> ) {

    chomp $line;

    my @tokens = split /\s+
#tokens are white space delimited
/xms, $line;
#convert the parens to tags
    my @wrapped_tree = map { parens2tags($_) } @tokens;

    print "@wrapped_tree\n";
}

close $PARSE or croak "could not close pipe";

sub parens2tags {
my ($token) = @_;

if ( $token =~ /\((\w+)
#open paren followed by constituent 
/xms ){
    return "<tree label=\"$1\">";
}

if ( $token =~ /\($
#lone open paren
/xms ) {
    return '<tree>';
}

if ( $token =~ /(.+)(\)+)
#leaf node followed by clos paren
/xms ) {
    my @leaf = split /
#put characters in list
/xms, $token;
    my $inner_most_close_paren_index = first_index {/\)
#find the first close paren
/xms} @leaf;
    my $outter_most_close_paren_index = last_index {/\)
#find the last close paren
/xms} @leaf;
    my $number_of_close_parens = ( $outter_most_close_paren_index - $inner_most_close_paren_index ) + 1;
    my $leaf = substr $token, 0, $inner_most_close_paren_index;
    my $close_tags = '</tree>' x $number_of_close_parens;
    return "$leaf $close_tags";
}

if ( $token =~ /^\)$
#lone close paren
/xms ) {
return '</tree>';
}

return q{};
}
 
