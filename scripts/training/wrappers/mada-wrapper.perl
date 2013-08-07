#!/usr/bin/perl -w

use strict;
use File::Temp qw/tempfile/;
use Getopt::Long "GetOptions";

my $TMPDIR = "tmp";
my $SCHEME = "D2";
my $KEEP_TMP = 0;

GetOptions(
  "scheme=s" => \$SCHEME,
  "tmpdir=s" => \$TMPDIR,
  "keep-tmp" => \$KEEP_TMP
) or die("ERROR: unknown options");

`mkdir -p $TMPDIR`;
my ($dummy, $tmpfile) = tempfile("mada-in-XXXX", DIR=>$TMPDIR, UNLINK=>!$KEEP_TMP);

print STDERR $tmpfile."\n";
open(TMP,">$tmpfile");
while(<STDIN>) { 
  print TMP $_;
}
close(TMP);

my $madadir = "/home/pkoehn/statmt/project/mada-3.2";
`perl $madadir/MADA+TOKAN.pl >/dev/null 2>/dev/null config=$madadir/config-files/template.madaconfig file=$tmpfile TOKAN_SCHEME="SCHEME=$SCHEME"`;

`rm $tmpfile`;
`rm $tmpfile.bw`;
`rm $tmpfile.bw.mada`;
print `cat $tmpfile.bw.mada.tok`;
`rm $tmpfile.bw.mada.tok`;
