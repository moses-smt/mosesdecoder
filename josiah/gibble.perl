#!/usr/bin/env perl

use strict;
#eddie specific
use lib "/exports/informatics/inf_iccs_smt/perl/lib/perl5/site_perl";
use Config::Simple;

if ($#ARGV != 0) {
    print STDERR "Usage: perl gibble.perl <config-file>\n";
    exit 1;
}


my $config_file = $ARGV[0];
my $config = new Config::Simple($config_file) || 
    die "Error: unable to read config file \"$config_file\"";


#required global parameters
my $name = &param_required("general.name");
my $josiah_home_dir = &param_required("general.josiah-home");
my $weights_file = &param_required("general.weights-file");

#optional globals
my $queue = &param("general.queue", "inf_iccs_smt");

&check_exists ("weights file", $weights_file);
my $nweights=`wc -l $weights_file`;

#
# Create the training script
#

#job control
my $working_dir = &param("general.working-dir");
system("mkdir -p $working_dir") == 0 or  die "Error: unable to create directory \"$working_dir\"";
my $train_script = "$name-train";
my $jobs = &param("train.jobs",10);
my $job_name = "$name-t";
my $hours = &param("train.hours",48);

#required training parameters
my $moses_ini_file = &param_required("train.moses-ini-file");
my $input_file = &param_required("train.input-file");
my $reference_files = &param_required("train.reference-files");

&check_exists ("moses ini file", $moses_ini_file);
&check_exists ("train input file", $input_file);

#optional training parameters
my $feature_file = &param("train.feature-file");
&check_exists ("feature file", $feature_file) if $feature_file;
my $samples = &param("train.samples",2000);
my $burnin = &param("train.burnin", 100);
my $reheatings = &param("train.reheatings",2);
my $batch = &param("train.batch", 200);
my $optimisations = &param("train.optimisations",10);
my $eta = &param("train.eta",1);
my $mu = &param("train.mu",1);
my $extra_args = &param("train.extra-args");

#test configuration
my $test_freq = &param("test.frequency",0);
my ($test_input_file, $test_reference_file,$test_ini_file);
if ($test_freq) {
    $test_input_file = &param_required("test.input-file");
    $test_reference_file = &param_required("test.reference-file");
    &check_exists ("test input file", $test_input_file);

    for my $ref (glob $test_reference_file . "*") {
        &check_exists ("test ref file", $ref);
    }
    $test_ini_file = &param_required("test.moses-ini-file");
    &check_exists ("test ini file", $test_ini_file);
}
my $weight_file_stem = "$name-weights";
if (glob ($weight_file_stem . "_*")) {
    die "Error: weights files with stem $weight_file_stem already exist";
}


#file names
my $train_script_file = $working_dir . "/" . $train_script . ".sh"; 
my $train_out = $train_script . ".out";
my $train_err = $train_script . ".err";

#write the script
open TRAIN, ">$train_script_file" || die "Unable to open \"$train_script_file\" for writing";

&header(*TRAIN,$job_name,$working_dir,$jobs,$hours,$train_out,$train_err);
print TRAIN "mpirun -np \$NSLOTS $josiah_home_dir/josiah/josiah \\\n";
print TRAIN "-f $moses_ini_file \\\n";
print TRAIN "-i $input_file \\\n";
my @refs = split /,/, $reference_files;
for my $ref (@refs) {
    &check_exists("train ref file",  $ref);
    print TRAIN "-r $ref ";
}
print TRAIN "\\\n";
print TRAIN "-w $weights_file \\\n";
if ($feature_file) {
    print TRAIN "-X $feature_file \\\n";
}
print TRAIN "--weight-dump-freq $test_freq --weight-dump-stem $weight_file_stem \\\n";
print TRAIN "--gradient --xbleu ";
print TRAIN "-S $batch -M 10000 ";
print TRAIN "-b $burnin -s $samples ";
if ($reheatings) {
    print TRAIN "-a --reheatings $reheatings ";
}
print TRAIN "--optimizer-freq $optimisations ";
print TRAIN "--eta $eta " x $nweights;
print TRAIN "--mu $mu ";
print TRAIN $extra_args;

print TRAIN "\n";
close TRAIN;

#submit the training job
my $qsub_result = `qsub -P $queue $train_script_file`;
if ($qsub_result !~ /Your job (\d+)/) {
    die "Failed to qsub train job: $qsub_result";
}
my $train_job_id = $1;
print "Submitted training job id: $train_job_id at " . scalar(localtime()) . "\n";

#wait for the next weights file to appear, or the training job to end
my $train_iteration = 0;

while(1) {
    $train_iteration += $test_freq;
    my $new_weight_file = "$working_dir/$weight_file_stem" . "_" . $train_iteration;
    while ((! $test_freq || ! -e $new_weight_file) && `qstat | grep $train_job_id`) {
        sleep 10;
    }
    if (!$test_freq ||  ! -e $new_weight_file ) {
        print "Training finished at " . scalar(localtime()) . "\n";
        exit 0;
    }
    #new weight file written. create test script and submit
    my $test_script = "$name-test";
    my $jobs = &param("test.jobs",5);
    my $job_name = $name . "_$train_iteration";
    my $hours = &param("test.hours",12);

    my $samples = &param("test.samples", 5000);
    my $burnin = &param("test.burnin",100);
    my $reheatings = &param("test.reheatings",2);
    my $mbr_size = &param("test.mbr-size",1000);

    #file names
    my $test_script_file = $working_dir . "/" . $test_script . ".$train_iteration.sh"; 
    my $test_out = $test_script . ".$train_iteration.out";
    my $test_err = $test_script . ".$train_iteration.err";
    my $output_file = $working_dir . "/" . $job_name . ".out";

    if (! (open TEST, ">$test_script_file" )) {
        print "Warning: unable to create test script $test_script_file\n";
        next
    }

    &header(*TEST,$job_name,$working_dir,$jobs,$hours,$test_out,$test_err);
    print TEST "mpirun -np \$NSLOTS $josiah_home_dir/josiah/josiah \\\n";
    print TEST "-f $test_ini_file \\\n";
    print TEST "-i $test_input_file \\\n";
    print TEST "-o $output_file \\\n";

    print TEST "-s $samples -d -t -m -b $burnin ";
    if ($reheatings) {
        print TEST "-a --reheatings $reheatings ";
    }
    print TEST "--decode-random ";
    print TEST "-w $new_weight_file ";
    print TEST "--mbr --mbr-size $mbr_size ";
    print TEST "\n";

    my $bleu_script = "$josiah_home_dir/scripts/generic/multi-bleu.perl";
    print TEST "cat $output_file*_of_* > $output_file\n";
    print TEST "echo \"Max Derivation\"\n";
    print TEST  "perl -e '\$deriv=0; while(<>) {print if (\$deriv%3) == 0; ++\$deriv;}' $output_file  | $bleu_script $test_reference_file\n";
    print TEST "echo \"Max Translation\"\n";
    print TEST  "perl -e '\$deriv=0; while(<>) {print if (\$deriv%3) == 1; ++\$deriv;}' $output_file  | $bleu_script $test_reference_file\n";
    print TEST "echo \"MBR\"\n";
    print TEST  "perl -e '\$deriv=0; while(<>) {print if (\$deriv%3) == 2; ++\$deriv;}' $output_file  | $bleu_script $test_reference_file\n";

    close TEST;

    #launch testing
    my $qsub_result = `qsub -P $queue $test_script_file`;
    if ($qsub_result !~ /Your job (\d+)/) {
        print "Failed to qsub test job: $qsub_result\n";
        next;
    }
    print "Submitted test job id: $1 for iteration $train_iteration at " .
        scalar(localtime()) . "\n";


}


sub param {
    my ($key,$default) = @_;
    my $value = $config->param($key);
    $value = $default if !$value;
    $value = join $value if (ref($value) eq 'ARRAY');
    return $value;
}

sub param_required {
    my ($key) = @_;
    my $value = $config->param($key);
    die "Error: required parameter \"$key\" was missing" if (!defined($value));
    $value = join $value if (ref($value) eq 'ARRAY');
    return $value;
}

sub header {
    my ($OUT,$name,$working_dir,$jobs,$hours,$out,$err) = @_;
    print $OUT "#!/bin/sh\n";
    print $OUT "#\$ -N $name\n";
    print $OUT "#\$ -wd $working_dir\n";
    print $OUT "#\$ -pe openmpi $jobs\n";
    print $OUT "#\$ -l h_rt=$hours:00:00\n";
    print $OUT "#\$ -o $out\n";
    print $OUT "#\$ -e $err\n";
    print $OUT "\n";
# some eddie specific stuff
    print $OUT ". /etc/profile.d/modules.sh\n";
    print $OUT "module load openmpi/gcc/64/1.2.5/tcp\n";
    print $OUT "export LD_LIBRARY_PATH=/exports/informatics/inf_iccs_smt/shared/boost/lib:\$LD_LIBRARY_PATH\n";
}

sub check_exists {
    my ($name,$filename) = @_;
    die "Error: unable to read $name: \"$filename\"" if ! -r $filename;
}



