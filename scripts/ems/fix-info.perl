#!/usr/bin/perl -w

use strict;

my ($file,$step) = @ARGV;
$step = "*" unless defined($step);

die("fix-info.perl file [step]") unless defined($file);
die("file not found") unless -e $file;
die("full path!") unless $file =~ /^\//; 
my @filestat = stat($file);
my $newtime = $filestat[9];

open(LS,"ls steps/$step/*INFO|") || die;
while(my $info = <LS>) {
    chop($info);
    my @INFO = `cat $info`;
    my $changed = 0;
    foreach (@INFO) {
	if (/$file .*\[/) {
	    $changed++;
	    s/($file) (.*\[)\d+/$1 $2$newtime/g;
	}  
    }
    if ($changed) {
	print "updating $info\n";
	open(INFO,">$info");
	foreach (@INFO) { print INFO $_; }
	close(INFO);
    }
}
close(LS);

