package MosesScriptsRegressionTesting;

use strict;

# if your tests need a new version of the test data, increment this
# and make sure that a moses-scripts-regression-tests-vX.Y is available
use constant TESTING_DATA_VERSION => '1.0';

# find the data directory in a few likely locations and make sure
# that it is the correct version
sub find_data_directory
{
  my ($test_script_root, $data_dir) = @_;
  my $data_version = TESTING_DATA_VERSION;
  my @ds = ();
  my $mrtp = "moses-scripts-reg-test-data-$data_version";
  push @ds, $data_dir if defined $data_dir;
  push @ds, "$test_script_root/$mrtp";
  push @ds, "/tmp/$mrtp";
  push @ds, "/var/tmp/$mrtp";
  foreach my $d (@ds) {
    next unless (-d $d);

    return $d;
  }
  print STDERR<<EOT;

You do not appear to have the regression testing data installed.
You may either specify a non-standard location (absolute path) 
when running the test suite with the --data-dir option, 
or, you may install it in any one of the following
standard locations: $test_script_root, /tmp, or /var/tmp with these
commands:

  cd <DESIRED_INSTALLATION_DIRECTORY>
  wget http://www.statmt.org/moses/reg-testing/moses-scripts-reg-test-data-$data_version.tgz
  tar xzf moses-scripts-reg-test-data-$data_version.tgz
  rm moses-scripts-reg-test-data-$data_version.tgz

EOT
	exit 1;
}

1;

sub get_localized_moses_ini
{
  use File::Temp;
  my ($moses_ini, $data_dir) = @_;
  use Cwd qw/ abs_path /; use File::Basename; my $TEST_PATH = dirname(abs_path($moses_ini));
  my $LM_PATH = "$data_dir/lm";
  my $TM_PATH = "$data_dir/models";
  my $RM_PATH = "$data_dir/models";
  my $local_moses_ini = new File::Temp( UNLINK => 0, SUFFIX => '.ini' );

  open MI, "<$moses_ini" or die "Couldn't read $moses_ini";
  open MO, ">$local_moses_ini" or die "Couldn't open $local_moses_ini for writing";
  while (my $l = <MI>) {
        $l =~ s/\$\{LM_PATH\}/$LM_PATH/g;
        $l =~ s/\$\{TM_PATH\}/$TM_PATH/g;
        $l =~ s/\$\{RM_PATH\}/$RM_PATH/g;
        $l =~ s/\$\{TEST_PATH\}/$TEST_PATH/g;
        print $local_moses_ini $l;
  }
  close MO;
  close MI;

  return $local_moses_ini->filename;
}


