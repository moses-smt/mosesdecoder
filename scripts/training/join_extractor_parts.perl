#!/usr/bin/perl

use strict;
my @scores;
my ($TYPE, $ID, $NO, $SCORES, @REST);
$ID = 0;
$NO = 0;
my $END;

while (<STDIN>) {
    chomp;
    if (/^(FEATURES|SCORES)_TXT_BEGIN/) {
        my ($id, $no);
        ($TYPE, $id, $no, $SCORES, @REST) = split(/\s/);
        if ($id == $ID) {
            $NO += $no;
        }
        else {
            print join(" ", $TYPE, $ID, $NO, $SCORES, @REST), "\n";
            print "$_\n" foreach(@scores);
            print "$END\n";
            
            $ID = $id;
            $NO = $no;
            @scores = ();
        }
        
    }
    elsif(/(FEATURES|SCORES)_TXT_END/) {
        $END = $_;
    }
    else {
        push(@scores, $_);
    }
}
print join(" ", $TYPE, $ID, $NO, $SCORES, @REST), "\n";
print "$_\n" foreach(@scores);
print "$END\n";
