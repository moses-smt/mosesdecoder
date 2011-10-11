#!/usr/bin/env perl
# downloads the regression data
use strict;
use MosesRegressionTesting;

my $data_version = MosesRegressionTesting::TESTING_DATA_VERSION;

exit 0 if -d "moses-reg-test-data-$data_version";
  # data in place

safesystem("wget http://www.statmt.org/moses/reg-testing/moses-reg-test-data-$data_version.tgz")
  or die "wget failed";
safesystem("tar xzf moses-reg-test-data-$data_version.tgz")
  or die "untar failed";
safesystem("rm moses-reg-test-data-$data_version.tgz");

sub safesystem {
  # print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
      print STDERR "Failed to execute: @_\n  $!\n";
      exit(1);
  }
  elsif ($? & 127) {
      printf STDERR "Execution of: @_\n  died with signal %d, %s coredump\n",
          ($? & 127),  ($? & 128) ? 'with' : 'without';
      exit(1);
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}
