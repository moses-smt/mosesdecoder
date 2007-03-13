#!/usr/bin/perl

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

unless (-x "$bin_dir/GIZA++" && -x "$bin_dir/snt2cooc.out" && -x "$bin_dir/mkcls" ) {
  print <<EOT;
Please specify a BINDIR.

  The BINDIR directory must contain GIZA++, snt2cooc.out and mkcls executables.
  These are available from http://www.fjoch.com/GIZA++.html and
  http://www-i6.informatik.rwth-aachen.de/Colleagues/och/software/mkcls.html .
EOT
  exit 0;
}


exit 0;

