#!/usr/bin/env perl

use strict;
#eddie specific
use lib "/exports/informatics/inf_iccs_smt/perl/lib/perl5/site_perl";
use Config::Simple;
use File::Basename;
use Getopt::Long "GetOptions";

my ($config_file,$execute,$continue);
die("gibble.perl -config config-file [-exec]")
    unless  &GetOptions('config=s' => \$config_file,
        'cont=s' => \$continue,
        'exec' => \$execute);

my $config = new Config::Simple($config_file) || 
    die "Error: unable to read config file \"$config_file\"";

#substitution
foreach my $key ($config->param) {
    my $value = $config->param($key);
    while ($value =~ m/(.*?)\$\{(.*?)\}(.*)/) {
        my $sub = $config->param($2);
        if (! $sub) {
            #try in this scope
            my $scope = (split /\./, $key)[0];
            $sub = $config->param($scope . "." . $2);
        }
        if (! $sub) {
            #then general
            $sub = $config->param("general." . $2);
        }
        $value = $1 . $sub . $3;
    }
    print STDERR "$key => "; print STDERR $value; print STDERR "\n";
    $config->param($key,$value);
}

#check if we're using sge
my $have_sge = 0;
if (`which qsub 2>/dev/null`) {
  print "Using sge for job control.\n";
  $have_sge = 1;
} else {
  print "No sge detected.\n";
}

#required global parameters
my $name = &param_required("general.name");

#optional globals
my $queue = &param("general.queue", "inf_iccs_smt");
my $mpienv = &param("general.mpienv", "openmpi-smp");
my $vmem = &param("general.vmem", "6");

my $feature_file = &param("general.features");
if ($feature_file) {
     &check_exists("feature file ", $feature_file);
}

#
# Create the training script
#

#job control
my $working_dir = &param("general.working-dir");
system("mkdir -p $working_dir") == 0 or  die "Error: unable to create directory \"$working_dir\"";
my $train_script = "$name-train";
my $jobs = &param("train.jobs",8);
my $job_name = "$name-t";
my $hours = &param("train.hours",48);

#required training parameters
my $moses_ini_file = &param_required("train.moses-ini-file");
&check_exists ("moses ini file", $moses_ini_file);
my $input_file = &param_required("train.input-file");
&check_exists ("train input file", $input_file);
my $reference_files = &param_required("train.reference-files");
my $samplerank_exe = &param_required("train.samplerank");
&check_exists("samplerank executable", $samplerank_exe);
my $weights_file = &param_required("train.weights-file");
&check_exists("weights file ", $weights_file);
my $epoch_lines = &param_required("train.epoch-lines");


#optional training parameters
my $burnin = &param("train.burnin", 100);
my $epochs = &param("train.epochs", 2);
my $fixed_temp = &param("train.fixed-temp", 0.000001);
my $learner = &param("train.learner", "mira+");
my $margin_scale = &param("train.margin-scale", 5);
my $samples = &param("train.samples",10);
my $slack = &param("train.slack", 0.1);
my $extra_args = &param("train.extra-args");

my $batch_lines = &param("train.batch-lines", 1);
my $weight_dump_samples = &param("train.weight-dump-samples",0);
my $weight_dump_batches = &param("train.weight-dump-batches",0);
if ($weight_dump_samples == 0 && $weight_dump_batches == 0) {
  # Dump once every epoch
  my $total_batch_lines = $batch_lines*$jobs;
  if ($batch_lines) {
    die "The number of epoch lines must be divisible by batch_lines*jobs" 
      if ($epoch_lines % $total_batch_lines != 0);
    $weight_dump_batches = $epoch_lines / $total_batch_lines;
  } else {
    $weight_dump_batches = 1;
  }
}

#test configuration
my ($test_input_file, $test_reference_file,$test_ini_file,$bleu_script,$use_moses);
my $test_exe = &param("test.josiah");
if (!$test_exe) {
    $use_moses = 1;
    $test_exe = &param_required("test.moses");
} else {
    $use_moses = 0;
}
&check_exists("test  executable", $test_exe);
$bleu_script  = &param_required("test.bleu");
&check_exists("multi-bleu script", $bleu_script);
$test_input_file = &param_required("test.input-file");
$test_reference_file = &param_required("test.reference-file");
&check_exists ("test input file", $test_input_file);

for my $ref (glob $test_reference_file . "*") {
    &check_exists ("test ref file", $ref);
}
$test_ini_file = &param_required("test.moses-ini-file");
&check_exists ("test ini file", $test_ini_file);
my $weight_file_stem = "$name-weights";


#file names
my $train_script_file = $working_dir . "/" . $train_script . ".sh"; 
my $train_out = $train_script . ".out";
my $train_err = $train_script . ".err";

# clean up old files
system("rm $working_dir/$name*");

#write the script
open TRAIN, ">$train_script_file" or die "Unable to open \"$train_script_file\" for writing";

&header(*TRAIN,$job_name,$working_dir,$jobs,$hours,$vmem,$train_out,$train_err);
print TRAIN "mpirun -np \$NSLOTS $samplerank_exe \\\n";
print TRAIN "-f $moses_ini_file \\\n";
print TRAIN "-i $input_file \\\n";
my @refs;
if (ref($reference_files) eq 'ARRAY') {
    @refs = @$reference_files;
} else {
    @refs = glob $reference_files;
}
for my $ref (@refs) {
    &check_exists("train ref file",  $ref);
    print TRAIN "-r $ref ";
}
print TRAIN "\\\n";
if ($weights_file) {
    print TRAIN "-w $weights_file \\\n";
}
if ($feature_file) {
    print TRAIN "-X $feature_file \\\n";
}
print TRAIN "--weight-dump-stem $weight_file_stem --weight-dump-samples $weight_dump_samples --weight-dump-batches $weight_dump_batches \\\n";
print TRAIN "-b $burnin -s $samples \\\n";
print TRAIN "--epoch-lines $epoch_lines --epochs $epochs \\\n";
print TRAIN "--batch-lines $batch_lines \\\n";
print TRAIN "--fixed-temperature $fixed_temp \\\n";
print TRAIN "--learner $learner --margin-scale $margin_scale --slack $slack \\\n";
print TRAIN $extra_args;

print TRAIN "\n";
close TRAIN;

if (! $execute) {
    print "Written train file: $train_script_file\n";
    exit 0;
}


#submit the training job
my $train_job_id;
if ($have_sge) {
  $train_job_id = &submit_job_sge($train_script_file);
} else {
  $train_job_id = &submit_job_no_sge($train_script_file, $train_out,$train_err);
}

die "Failed to submit training job" unless $train_job_id;

#wait for the next weights file to appear, or the training job to end
my $train_iteration = -1;

while(1) {
    $train_iteration += 1;
    my $new_weight_file = "$working_dir/$weight_file_stem" . "_";
    $new_weight_file  .= $train_iteration;
    print "Waiting for $new_weight_file\n";
    while ((! -e $new_weight_file) && &check_running($train_job_id)) {
        sleep 10;
    }
    if (! -e $new_weight_file ) {
        print "Training finished at " . scalar(localtime()) . "\n";
        exit 0;
    }
    my $train_iteration_name = sprintf("%03d", $train_iteration);
    #new weight file written. create test script and submit
    #file names
    my $job_name = $name . "_$train_iteration_name";
    my $test_script = "$name-test";
    my $test_script_file = $working_dir . "/" . $test_script . ".$train_iteration_name.sh"; 
    my $test_out = $test_script . ".$train_iteration_name.out";
    my $test_err = $test_script . ".$train_iteration_name.err";
    my $output_file = $working_dir . "/" . $job_name . ".out";
    if (! (open TEST, ">$test_script_file" )) {
        print "Warning: unable to create test script $test_script_file\n";
        next;
    }
    my $hours = &param("test.hours",12);
    my $memory = &param("test.memory",1);
    my $extra_args = &param("test.extra-args");

    if ($use_moses) {
        # Normalise the weights and splice them into the moses ini file.
        my ($default_weight,$wordpenalty_weight,@phrasemodel_weights,@lm_weights,$distortion_weight, @lexreorder_weights);
        # Check if there's a feature file, and a core feature. If there
        # is, then we read the core weights from there
        my $core_weight_file = $new_weight_file;
        my $have_core = 0;
        if ($feature_file) {
          my $feature_config;
          if (!($feature_config = new Config::Simple($feature_file))) {
              print "Warning: unable to read feature file \"$feature_file\"";
              next;
          }
          if ($feature_config->param("core.weightfile")) {
            $core_weight_file = $feature_config->param("core.weightfile");
            $have_core = 1;
          }
        }
        if (! (open WEIGHTS, "$core_weight_file")) {
            print "Warning: unable to open weights file $core_weight_file\n";
            next;
        }
        my %extra_weights;
        while(<WEIGHTS>) {
            chomp;
            my ($name,$value) = split;
            next if ($name =~ /^!Unknown/);
            if ($name eq "DEFAULT_") {
                $default_weight = $value;
            } else {
                if ($name eq "WordPenalty") {
                  $wordpenalty_weight = $value;
                } elsif ($name =~ /^PhraseModel/) {
                  push @phrasemodel_weights,$value;
                } elsif ($name =~ /^LM_(.*)/) {
                  $lm_weights[$1] = $value;
                } elsif ($name eq "Distortion") {
                  $distortion_weight = $value;
                } elsif ($name =~  "LexicalReordering") {
                  push @lexreorder_weights, $value;
                } else {
                  $extra_weights{$name} = $value;
                }
            }
        }
        close WEIGHTS;
        # If there was a core weight file, then we have to load the weights
        # from the new weight file
        if ($core_weight_file ne $new_weight_file) {
          if (! (open WEIGHTS, "$new_weight_file")) {
            print "Warning: unable to open weights file $new_weight_file\n";
            next;
          }
          while(<WEIGHTS>) {
            chomp;
            my ($name,$value) = split;
            $extra_weights{$name} = $value;
          }
        }

        #Normalising factor
        my $total = abs($wordpenalty_weight+$default_weight) + 
                    abs($distortion_weight+$default_weight);
        foreach my $phrasemodel_weight (@phrasemodel_weights) {
            $total += abs($phrasemodel_weight + $default_weight);
        }
        foreach my $lexreorder_weight (@lexreorder_weights) {
            $total += abs($lexreorder_weight + $default_weight);
        }
        foreach my $lm_weight (@lm_weights) {
            $total += abs($lm_weight + $default_weight);
        }

        #Create new ini file
        my $new_test_ini_file = $working_dir . "/" . $test_script . ".$train_iteration_name.ini";
        if (! (open NEWINI, ">$new_test_ini_file" )) {
            print "Warning: unable to create ini file $new_test_ini_file\n";
            next;
        }
        if (! (open OLDINI, "$test_ini_file" )) {
            print "Warning: unable to read ini file $test_ini_file\n";
            next;
        }
        while(<OLDINI>) {
            if (/weight-l/) {
                print NEWINI "[weight-l]\n";
                #print NEWINI ($lm_weight+$default_weight)/$total;
                #print NEWINI "\n";
                foreach my $lm_weight (@lm_weights) {
                    $lm_weight = 0 unless $lm_weight;
                    print NEWINI ($lm_weight+$default_weight)/$total;
                    print NEWINI "\n";
                    readline(OLDINI);
                }
            } elsif (/weight-t/) {
                print NEWINI "[weight-t]\n";
                foreach my $phrasemodel_weight (@phrasemodel_weights) {
                    print NEWINI ($phrasemodel_weight+$default_weight) / $total;
                    print NEWINI "\n";
                    readline(OLDINI);
                }
            } elsif (/weight-d/) {
                print NEWINI "[weight-d]\n";
                print NEWINI ($distortion_weight+$default_weight)/$total;
                print NEWINI "\n";
                readline(OLDINI);
                # lex reorder weights, if any
                foreach my $lexreorder_weight (@lexreorder_weights) {
                  print NEWINI ($lexreorder_weight+$default_weight) / $total;
                  print NEWINI "\n";
                  readline(OLDINI);
                }
            } elsif (/weight-w/) {
                print NEWINI "[weight-w]\n";
                print NEWINI ($wordpenalty_weight+$default_weight)/$total;
                print NEWINI "\n";
                readline(OLDINI);
            } else {
                print NEWINI;
            }
        }
        close NEWINI;
        close OLDINI;

        # if there are any non-core weights, write them to a weights file
        my $extra_weight_file = undef;
        if (%extra_weights) {
          $extra_weight_file = "$new_weight_file.scaled";
          if (! (open EXTRAWEIGHT,">$extra_weight_file")) {
            print "Warning: unable to create extra weights file $extra_weight_file";
            next;
          }
          my $core_weight = 1;
          if ($have_core) {
            $default_weight = $extra_weights{"DEFAULT_"};
            $core_weight = $extra_weights{"core"} + $default_weight;
          }
          #print "total: $total core: $core_weight default: $default_weight\n";
          foreach my $name (sort keys %extra_weights) {
            next if ($name eq "core");
            next if ($name eq "DEFAULT_");
            my $value = $extra_weights{$name} + $default_weight;
            #print "name: $name value: $value\n";
            $value /= $core_weight;
            $value /= $total;
            #print "name: $name value: $value\n";
            print EXTRAWEIGHT "$name $value\n" if $value;
          }
        }
  

        print TEST "#!/bin/sh\n";
        print TEST "#\$ -N $job_name\n";
        print TEST "#\$ -wd $working_dir\n";
        print TEST "#\$ -l h_rt=$hours:00:00\n";
        print TEST "#\$ -pe memory-2G $memory\n" if $memory > 1;
        print TEST "#\$ -o $test_out\n";
        print TEST "#\$ -e $test_err\n";
        print TEST "\n";
        print TEST "$test_exe -i $test_input_file -f $new_test_ini_file ";
        if ($extra_weight_file) {
          print TEST "-weight-file $extra_weight_file ";
        }
        print TEST $extra_args;
        print TEST " > $output_file\n";
        print TEST  "$bleu_script $test_reference_file < $output_file\n";

    } else {
        my $jobs = &param("test.jobs",4);

        my $samples = &param("test.samples", 5000);
        my $burnin = &param("test.burnin",100);
        my $reheatings = &param("test.reheatings",2);
        my $fixed_temp_test = &param("test.fixed-temp", 0.000001);

        &header(*TEST,$job_name,$working_dir,$jobs,$hours,$vmem,$test_out,$test_err);
        print TEST "mpirun -np \$NSLOTS $test_exe \\\n";
        print TEST "-f $test_ini_file \\\n";
        print TEST "-i $test_input_file \\\n";
        print TEST "-o $output_file \\\n";
        if ($feature_file) {
            print TEST "-X $feature_file \\\n";
        }

        print TEST "-s $samples -m -b $burnin ";
        if ($reheatings) {
            print TEST "-a --reheatings $reheatings ";
        }
        print TEST "-w $new_weight_file \\\n";
        print TEST "--mapdecode --fixed-temp-accept --fixed-temperature $fixed_temp_test \\\n";
        print TEST $extra_args;
        print TEST "\n";

        print TEST "cat $output_file*_of_* > $output_file\n";
        print TEST "rm -f $output_file*_of_*\n";
        print TEST  "$bleu_script $test_reference_file < $output_file\n";

    }
    close TEST;

    #launch testing
    if ($have_sge) {
      &submit_job_sge($test_script_file);
    } else {
      &submit_job_no_sge($test_script_file, $test_out,$test_err);
    }


}


sub param {
    my ($key,$default) = @_;
    my $value = $config->param($key);
    $value = $default if (!defined($value));
    # Empty arguments get interpreted as arrays
    $value = "" if (ref($value) eq 'ARRAY' && scalar(@$value) == 0);
    return $value;
}

sub param_required {
    my ($key) = @_;
    my $value = &param($key);
    die "Error: required parameter \"$key\" was missing" if (!defined($value));
    #$value = join $value if (ref($value) eq 'ARRAY');
    return $value;
}

sub header {
    my ($OUT,$name,$working_dir,$jobs,$hours,$vmem,$out,$err) = @_;
    print $OUT "#!/bin/sh\n";
    if ($have_sge) {
      print $OUT "#\$ -N $name\n";
      print $OUT "#\$ -wd $working_dir\n";
      print $OUT "#\$ -pe $mpienv $jobs\n";
      print $OUT "#\$ -l h_rt=$hours:00:00\n";
      print $OUT "#\$ -l h_vmem=$vmem" . "G" . "\n";
      print $OUT "#\$ -o $out\n";
      print $OUT "#\$ -e $err\n";
    } else {
      print $OUT "\nNSLOTS=$jobs\n";
    }
    print $OUT "\n";
    if ($have_sge) {
# some eddie specific stuff
      print $OUT ". /etc/profile.d/modules.sh\n";
      print $OUT "module load openmpi/ethernet/gcc/latest\n";
      print $OUT "export LD_LIBRARY_PATH=/exports/informatics/inf_iccs_smt/shared/boost/lib:\$LD_LIBRARY_PATH\n";
    }
}

sub check_exists {
    my ($name,$filename) = @_;
    die "Error: unable to read $name: \"$filename\"" if ! -r $filename;
}

#
# Used to submit train/test jobs. Return the job id, or 0 on failure
#

sub submit_job_sge {
    my($script_file) = @_;
    my $qsub_result = `qsub -P $queue $script_file`;
    if ($qsub_result !~ /Your job (\d+)/) {
        print "Failed to qsub job: $qsub_result\n";
        return 0;
    }
    my $job_name = basename($script_file);
    print "Submitted job: $job_name  id: $1  " .
        scalar(localtime()) . "\n";
    return $1;
}

#
# As above, but no sge version. Returns the pid.
#

sub submit_job_no_sge {
  my($script_file,$out,$err) = @_;
  my $pid = undef;
  if ($pid = fork) {
    my $job_name = basename($script_file);
    print "Launched : $job_name  pid: $pid  " .  scalar(localtime()) . "\n";
    return $pid;
  } elsif (defined $pid) {
   `cd $working_dir; sh $script_file >$out 2> $err`;
    exit;
  } else {
    # Fork failed
    return 0;
  }
}

sub check_running {
  my ($job_id) = @_;
  if ($have_sge) {
    my $qstat=`qstat`;
    return $qstat =~ /$job_id/;
  } else {
    return `ps h  -p $job_id | grep -v defunct`;
  }
}



