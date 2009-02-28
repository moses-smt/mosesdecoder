#!/usr/bin/perl

# $Id$

my ($home, $target_dir, $release_dir, $bin_dir) = @ARGV;

#print "HOME: $home\nTARGET_DIR: $target_dir\nRELEASE_DIR: $release_dir\n";

if ($target_dir eq '' || -z $target_dir) {
  print <<EOT;
Please specify a TARGETDIR.

  For development releases you probably want the following:
  TARGETDIR=$home/releases make release

  For shared environments, you will want to set TARGETDIR to
  some appropriately common directory.

EOT
  exit 1;
}

if (-e $release_dir) {
  print "Targetdir exists! Not touching it! $release_dir";
  exit 1;
}

exit 0;

