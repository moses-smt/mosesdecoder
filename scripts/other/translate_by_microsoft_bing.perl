#!/usr/bin/env perl

# Script implemented by Pranava Swaroop Madhyastha (a student at Charles
# University, UFAL)
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use strict;
use warnings;

my $appid = shift;
die "Usage: $0 YOUR-DEVELOPER-ID < input    ... will write to microsoft_translated.out"
  if ! defined $appid;
# this is someone's ID, do not use it: 28AEB40E8307D187104623046F6C31B0A4DF907E

binmode STDIN, ":utf8";
binmode STDOUT, ":raw";
open(OUTPUTFILE, ">>microsoft_translated.out");
use LWP::UserAgent;
sub print_translation {
    my $text = shift;
    my $from = "en";
    my $to = "cs";
    my $ua = LWP::UserAgent->new;
    $ua->agent('Mozilla/5.0');
    my $dumb =  "http://api.microsofttranslator.com/v2/Http.svc/Translate?appId=". $appid ."&text=". $text ."&from=". $from ."&to=". $to ;
    my $response = $ua->get($dumb);
    die "Error: ", $response->status_line unless $response->is_success;

    my $content = $response->content;
        my $cs_text = $content;
	$cs_text =~ s/<string (.*?)>\s*//;
	$cs_text =~ s/<.string>//;
        print OUTPUTFILE "$cs_text\n";
#	print $cs_text;
};

my $number_of_lines;
my $no = 0;
my $en_text;
while (<>) {
    $en_text .= $_;
    $number_of_lines++;
    if ($number_of_lines == 1) {
        $no = $no + 10;
	print STDERR "Sending sentence $no  to microsofttranslator...\n and writing it in microsoft_translated.out \n";
        print_translation($en_text);
        $number_of_lines = 0;
        $en_text = "";
    }
}
print_translation($en_text);
