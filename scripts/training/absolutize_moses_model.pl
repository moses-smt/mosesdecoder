#!/usr/bin/perl

# $Id$
# given a moses.ini file, prints a copy to stdout but replaces all relative
# paths with absolute paths.
#
# Ondrej Bojar.

my $ini = shift;
die "usage: absolutize_moses_model.pl path-to-moses.ini > moses.abs.ini"
  if !defined $ini;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

$inih = my_open($ini);
while (<$inih>) {
  if (/^\[([^\]]*)\]\s*$/) {
    $section = $1;
  }
  if (/^[0-9]/) {
    if ($section eq "ttable-file") {
      chomp;
      my ($type, $b, $c, $d, $fn) = split(/ /, $_, 5);

		if ( $type eq '8' ) {
			# suffix arrays model: <src-corpus> <tgt-corpus> <alignment>.
			my ($src, $tgt, $align) = split(/ /, $fn);

			my $abs_src = ensure_absolute($src, $ini);
			die "File not found or empty: $src (searched for $abs_src or $abs_src.binphr.idx)"
				if ! -s $abs_src && ! -s $abs_src.".binphr.idx"; # accept binarized ttables

			my $abs_tgt = ensure_absolute($tgt, $ini);
			die "File not found or empty: $tgt (searched for $abs_tgt or $abs_tgt.binphr.idx)"
				if ! -s $abs_tgt && ! -s $abs_tgt.".binphr.idx"; # accept binarized ttables

			my $abs_align = ensure_absolute($align, $ini);
			die "File not found or empty: $align (searched for $abs_align or $abs_align.binphr.idx)"
				if ! -s $abs_align && ! -s $abs_align.".binphr.idx"; # accept binarized ttables

			$_ = "$type $b $c $d $abs_src $abs_tgt $abs_align\n";
		}
                elsif ( $type eq '12' ) {
		  $abs = ensure_absolute($fn, $ini);
		  die "File not found or empty: $fn (searched for $abs.minphr)"
		    if ! -s $abs.".minphr"; # accept compact binarized ttables
		  $_ = "$type $b $c $d $abs\n";                    
                }
		else {
		  $abs = ensure_absolute($fn, $ini);
		  die "File not found or empty: $fn (searched for $abs or $abs.binphr.idx)"
		    if ! -s $abs && ! -s $abs.".binphr.idx"; # accept binarized ttables
		  $_ = "$type $b $c $d $abs\n";
		}	
    }
    if ($section eq "generation-file" || $section eq "lmodel-file") {
      chomp;
      my ($a, $b, $c, $fn) = split / /;
      $abs = ensure_absolute($fn, $ini);
      die "File not found or empty: $fn (searched for $abs)"
        if ! -s $abs;
      $_ = "$a $b $c $abs\n";
    }
    if ($section eq "distortion-file") {
      chomp;
      my ($a, $b, $c, $fn) = split / /;
      $abs = ensure_absolute($fn, $ini);
      die "File not found or empty: $fn (searched for $abs or $abs.binlexr.idx or $abs.minlexr)"
        if ! -s $abs && ! -s $abs.".binlexr.idx" && ! -s $abs.".minlexr"; # accept binarized and compact lexro models
      $_ = "$a $b $c $abs\n";
    }
  }
  print $_;
}
close $inih if $ini ne "-";

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

sub ensure_absolute {
  my $target = shift;
  my $originfile = shift;

  my $cwd = `pawd 2> /dev/null`;
  $cwd = `pwd` if ! defined $cwd; # not everyone has pawd!
  die "Failed to absolutize $target. Failing to get cwd!" if ! defined $cwd;
  chomp $cwd;
  $cwd.="/";

  my $absorigin = ensure_relative_to_origin($originfile, $cwd);
  return ensure_relative_to_origin($target, $absorigin);
}

sub ensure_relative_to_origin {
  my $target = shift;
  my $originfile = shift;
  return $target if $target =~ /^\/|^~/; # the target path is absolute already
  $originfile =~ s/[^\/]*$//; # where does the origin reside
  my $out = $originfile."/".$target;
  $out =~ s/\/+/\//g;
  $out =~ s/\/(\.\/)+/\//g;
  return $out;
}

sub my_open {
  my $f = shift;
  if ($f eq "-") {
    binmode(STDIN, ":utf8");
    return *STDIN;
  }

  die "Not found: $f" if ! -e $f;

  my $opn;
  my $hdl;
  my $ft = `file '$f'`;
  # file might not recognize some files!
  if ($f =~ /\.gz$/ || $ft =~ /gzip compressed data/) {
    $opn = "zcat '$f' |";
  } elsif ($f =~ /\.bz2$/ || $ft =~ /bzip2 compressed data/) {
    $opn = "bzcat '$f' |";
  } else {
    $opn = "$f";
  }
  open $hdl, $opn or die "Can't open '$opn': $!";
  binmode $hdl, ":utf8";
  return $hdl;
}
