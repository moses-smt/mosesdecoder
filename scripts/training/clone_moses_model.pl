#!/usr/bin/perl

# $Id$
# given a moses.ini file, creates a fresh version of it
# in the current directory
# All relevant files are hardlinked or copied to the directory, too.

use strict;
use Getopt::Long;

my @fixpath = ();
  # specify search-replace pattern to fix paths.
  # use a space to delimit source and target pathnames
my $symlink = 0; # prefer symlink over hardlink, but revert to hardlink or copy
                 # if symlink fails
GetOptions(
  "fixpath=s" => \@fixpath,
  "symlink"=>\$symlink,
);
my @fixrepls = map {
    my ($fixsrc, $fixtgt) = split / /, $_;
    print STDERR "Will replace >$fixsrc< with >$fixtgt<\n";
    [ $fixsrc, $fixtgt ];
  } @fixpath;

my $ini = shift;
die "usage: clone_moses_model.pl /a/source/moses.ini" if !defined $ini;

die "./moses.ini exists, will not overwrite" if -e "moses.ini";

my %cnt; # count files per section
open INI, $ini or die "Can't read $ini";
open OUT, ">moses.ini" or die "Can't write ./moses.ini";
my $section = undef;
while (<INI>) {
  if (/^\[([^\]]*)\]\s*$/) {
    $section = $1;
  }
  if (/^[0-9]/) {
    if ($section eq "ttable-file") {
      chomp;
      my ($a, $b, $c, $d, $fn) = split(/ /, $_, 5);
      $cnt{$section}++;

		if ( $a eq '8' ) {
			# suffix arrays model: <src-corpus> <tgt-corpus> <alignment>.
			my ($src, $tgt, $align) = split(/ /, $fn);

			$src = fixpath($src);
			$src = ensure_relative_from_origin($src, $ini);
			$src = ensure_exists_or_gzipped_exists($src);
			my $src_suffix = ($src =~ /\.gz$/ ? ".gz" : "");
		 	clone_file_or_die($src, "./sa.src.$cnt{$section}$src_suffix");

			$tgt = fixpath($tgt);
			$tgt = ensure_relative_from_origin($tgt, $ini);
			$tgt = ensure_exists_or_gzipped_exists($tgt);
			my $tgt_suffix = ($tgt =~ /\.gz$/ ? ".gz" : "");
			clone_file_or_die($tgt, "./sa.tgt.$cnt{$section}$tgt_suffix");

			$align = fixpath($align);
			$align = ensure_relative_from_origin($align, $ini);
			$align = ensure_exists_or_gzipped_exists($align);
			my $align_suffix = ($align =~ /\.gz$/ ? ".gz" : "");
		  	clone_file_or_die($align, "./sa.align.$cnt{$section}$align_suffix");

			$_ = "$a $b $c $d ./sa.src.$cnt{$section}$src_suffix ./sa.tgt.$cnt{$section}$tgt_suffix ./sa.align.$cnt{$section}$align_suffix\n";
		}
    elsif ( $a eq '1' ) {
      # handle binarized phrase tables
      $fn = ensure_relative_from_origin(fixpath($fn));
      foreach my $suf (qw( idx srctree srcvoc tgtdata tgtvoc )) {
        my $fullname = "$fn.binphr.$suf";
        if (-f $fullname) {
          clone_file_or_die($fullname, "./$section.$cnt{$section}.binphr.$suf");
        } else {
          die "Binary format specified but file $fullname not found!\n";
        }
      }
      $_ = "$a $b $c $d ./$section.$cnt{$section}\n";
    } else {
		  $fn = fixpath($fn);
		  $fn = ensure_relative_from_origin($fn, $ini);
		  $fn = ensure_exists_or_gzipped_exists($fn);
		  my $suffix = ($fn =~ /\.gz$/ ? ".gz" : "");
		  clone_file_or_die($fn, "./$section.$cnt{$section}$suffix");
		  $_ = "$a $b $c $d ./$section.$cnt{$section}$suffix\n";
		}
    }
    if ($section eq "generation-file" || $section eq "lmodel-file") {
      chomp;
      my ($a, $b, $c, $fn) = split / /;
      $cnt{$section}++;
      $fn = fixpath($fn);
      $fn = ensure_relative_from_origin($fn, $ini);
      $fn = ensure_exists_or_gzipped_exists($fn);
      my $suffix = ($fn =~ /\.gz$/ ? ".gz" : "");
      clone_file_or_die($fn, "./$section.$cnt{$section}$suffix");
      $_ = "$a $b $c ./$section.$cnt{$section}$suffix\n";
    }
    if ($section eq "distortion-file") {
      chomp;
      my ($a, $b, $c, $fn) = split / /;
      $cnt{$section}++;
      $fn = fixpath($fn);
      $fn = ensure_relative_from_origin($fn, $ini);
      $fn = ensure_exists_or_gzipped_exists($fn);
      my $suffix = ($fn =~ /\.gz$/ ? ".gz" : "");
      clone_file_or_die($fn, "./$section.$cnt{$section}$suffix");
      $_ = "$a $b $c ./$section.$cnt{$section}$suffix\n";
    }
  }
  print OUT $_;
}
close INI;
close OUT;

sub clone_file_or_die {
  my $src = shift;
  my $tgt = shift;

  my $src = resolve($src); # resolve symlinks

  my $ok = 0;
  if ($symlink) {
    # attemt a symlink
    $ok = safesystem("ln", "-s", $src, $tgt);
  }

  if (!$ok) {
    # perform a hardlink or a copy
    if (! safesystem("ln", $src, $tgt)) {
      # hardlink failed perform a copy
      safesystem("cp", "-u", $src, $tgt)
        or die "Failed to clone $src into $tgt";
    }
  }
  
  safesystem("echo $src > $tgt.info"); # dump a short information
}

sub ensure_exists_or_gzipped_exists {
  my $fn = shift;
  return $fn if -e $fn;
  my $tryfn = $fn.".gz";
  return $tryfn if -e $tryfn;
  die "$0:$ini:Neither file $fn nor $tryfn found.";
}

sub fixpath {
  my $fn = shift;
  foreach my $pair (@fixrepls) {
    $fn =~ s/$pair->[0]/$pair->[1]/g;
  }
  return $fn;
}

sub safesystem {
  print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
      print STDERR "Failed to execute: @_\n  $!\n";
      exit(1);
  }
  elsif ($? & 127) {
      printf STDERR "Execution of: @_\n  died with signal %d, %s coredump\n",
          ($? & 127),  ($? & 128) ? 'with' : 'without';
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}

sub resolve {
  my $f = shift;
  return $f if ! -l $f;
  my $targ_from_lnk = readlink($f) or die "Can't lstat $f";
  my $targ = ensure_relative_from_origin($targ_from_lnk, $f);
  # print STDERR "$f   ---> $targ_from_lnk  ---> $targ\n";

  my $fully = 1; # resolve the full chain of symlinks
  if ($fully) {
    my $newtarg = resolve($targ);
    $targ = $newtarg if defined $newtarg;
  }
  return $targ;
}

sub ensure_relative_from_origin {
  my $target = shift;
  my $originfile = shift;
  return $target if $target =~ /^\/|^~/; # the target path is absolute already
  $originfile =~ s/[^\/]*$//;
  my $prefix = ($originfile eq "" ? "" : $originfile."/");
  return simplify_path($prefix.$target);
}


sub simplify_path {
  my $path = shift;
  my $lastpath = "";
  while ($lastpath ne $path) {
    $lastpath = $path;
    $path =~ s/\/+/\//g;
    $path =~ s/(\/\.)+\//\//g;
    $path =~ s/\/[^\/]+(?<!\/\.\.)\/\.\.\//\//g;
    $path =~ s/^[^\/]+(?<!\/\.\.)\/\.\.\///g;
  }
  return $path;
}
