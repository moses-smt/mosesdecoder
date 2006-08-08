#!/usr/bin/perl
# given a moses.ini file, prints a copy to stdout but replaces all relative
# paths with absolute paths.
#
# Ondrej Bojar.

my $ini = shift;
die "usage: absolutize_moses_model.pl path-to-moses.ini > moses.abs.ini"
  if !defined $ini;

open INI, $ini or die "Can't read $ini";
while (<INI>) {
  if (/^\[([^\]]*)\]\s*$/) {
    $section = $1;
  }
  if (/^[0-9]/) {
    if ($section eq "ttable-file" || $section eq "lmodel-file") {
      chomp;
      my ($a, $b, $c, $fn) = split / /;
      $abs = ensure_absolute($fn, $ini);
      die "File not found or empty: $fn (interpreted as $abs)"
        if ! -s $abs;
      $_ = "$a $b $c $abs\n";
    }
    if ($section eq "generation-file") {
      chomp;
      my ($a, $b, $c, $fn) = split / /;
      $abs = ensure_absolute($fn, $ini);
      die "File not found or empty: $fn (interpreted as $abs)"
        if ! -s $abs;
      $_ = "$a $b $c $abs\n";
    }
    if ($section eq "distortion-file") {
      chomp;
      my $fn = $_;
      $abs = ensure_absolute($fn, $ini);
      die "File not found or empty: $fn (interpreted as $abs)"
        if ! -s $abs;
      $_ = "$abs\n";
    }
  }
  print $_;
}
close INI;

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

  my $cwd = `pwd`;
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
