package MosesRegressionTesting;

use strict;

# if your tests need a new version of the test data, increment this
# and make sure that a moses-regression-tests-vX.Y is available for
# download from statmt.org (redpony AT umd dot edu for more info)

# find the data directory in a few likely locations and make sure
# that it is the correct version
sub find_data_directory
{
  my ($test_script_root, $data_dir) = @_;
  my @ds = ();
	my $mrtp = "moses-reg-test-data";
	push @ds, $data_dir if defined $data_dir;
  push @ds, "$test_script_root/$mrtp";
  push @ds, "/export/ws06osmt/regression-testing/$mrtp";
	push @ds, "/tmp/$mrtp";
  push @ds, "/var/tmp/$mrtp";
	foreach my $d (@ds) {
	  next unless (-d $d);
		if (!-d "$d/models") {
			print STDERR "Found $d but it is malformed: missing subdir models/\n";
			next;
		}
		if (!-d "$d/lm") {
			print STDERR "Found $d but it is malformed: missing subdir lm/\n";
			next;
		}
		return $d;
	}
	print STDERR<<EOT;

You do not appear to have the regression testing data installed.  You may
either specify a non-standard location when running the test suite with
the --data-dir option, or, you may install it in any one of the following
standard locations: $test_script_root, /tmp, or /var/tmp with these
commands:

  cd <DESIRED_INSTALLATION_DIRECTORY>
  git clone https://github.com/hieuhoang/moses-reg-test-data.git

EOT
	exit 1;
}


sub get_localized_moses_ini
{
  use File::Temp;
  my ($moses_ini, $data_dir, $results_dir) = @_;
  my $LM_PATH = "$data_dir/lm";
  my $MODEL_PATH = "$data_dir/models";
  use Cwd qw/ abs_path /; use File::Basename; my $TEST_PATH = dirname(abs_path($moses_ini));
  my $local_moses_ini = new File::Temp( UNLINK => 0, SUFFIX => '.ini' );

  open MI, "<$moses_ini" or die "Couldn't read $moses_ini";
  open MO, ">$local_moses_ini" or die "Couldn't open $local_moses_ini for writing";
  while (my $l = <MI>) {
	$l =~ s/\$\{LM_PATH\}/$LM_PATH/g;
	$l =~ s/\$\{MODEL_PATH\}/$MODEL_PATH/g;
	$l =~ s/\$\{TEST_PATH\}/$TEST_PATH/g;
	$l =~ s/\$\{RESULTS_PATH\}/$results_dir/g;
	print $local_moses_ini $l;
  }
  close MO;
  close MI;

  return $local_moses_ini->filename;
}

sub get_nbestlist
{
  my ($moses_ini) = @_;
  my $nbestfile = undef;
  my $nbestsize = undef;

  open MI, "<$moses_ini" or die "Couldn't read $moses_ini";
  while (my $l = <MI>) {
    if ($l =~ /^\[n-best-list\]/i){
      chomp($nbestfile = <MI>);
      chomp($nbestsize = <MI>);
    }
  }
  close MI;

  return ($nbestfile,$nbestsize);
}


1;

