#!/usr/bin/perl
# given a moses.ini file, creates a wiseln of it and all the included bits
# in the current directory

# relies on wiseln, a wise variant of linking. You might just use ln -s instead.

my $ini = shift;
die "usage!" if !defined $ini;

my %cnt; # count files per section
open INI, $ini or die "Can't read $ini";
open OUT, ">moses.ini" or die "Can't write ./moses.ini";
while (<INI>) {
  if (/^\[([^\]]*)\]\s*$/) {
    $section = $1;
  }
  if (/^[0-9]/) {
    if ($section eq "ttable-file" || $section eq "lmodel-file") {
      chomp;
      my ($a, $b, $c, $fn) = split / /;
      $cnt{$section}++;
      my $suffix = ($fn =~ /\.gz$/ ? ".gz" : "");
      $fn = ensure_relative_to_origin($fn, $ini);
      safesystem("wiseln $fn ./$section.$cnt{$section}$suffix") or die;
      $_ = "$a $b $c ./$section.$cnt{$section}$suffix\n";
    }
    if ($section eq "generation-file") {
      chomp;
      my ($a, $b, $fn) = split / /;
      $cnt{$section}++;
      my $suffix = ($fn =~ /\.gz$/ ? ".gz" : "");
      $fn = ensure_relative_to_origin($fn, $ini);
      safesystem("wiseln $fn ./$section.$cnt{$section}$suffix") or die;
      $_ = "$a $b ./$section.$cnt{$section}$suffix\n";
    }
    if ($section eq "distortion-file") {
      chomp;
      my $fn = $_;
      $cnt{$section}++;
      my $suffix = ($fn =~ /\.gz$/ ? ".gz" : "");
      $fn = ensure_relative_to_origin($fn, $ini);
      safesystem("wiseln $fn ./$section.$cnt{$section}$suffix") or die;
      $_ = "./$section.$cnt{$section}$suffix\n";
    }
  }
  print OUT $_;
}
close INI;
close OUT;

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

sub ensure_relative_to_origin {
  my $target = shift;
  my $originfile = shift;
  return $target if $target =~ /^\/|^~/; # the target path is absolute already
  $originfile =~ s/[^\/]*$//;
  return $originfile."/".$target;
}
