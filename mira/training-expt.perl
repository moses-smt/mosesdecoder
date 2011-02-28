#!/usr/bin/env perl

use strict;
#eddie specific
use lib "/exports/informatics/inf_iccs_smt/perl/lib/perl5/site_perl";
use Config::Simple;
use File::Basename;
use Getopt::Long "GetOptions";

my ($config_file,$execute,$continue);
die("training-expt.perl -config config-file [-exec]")
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
my $mpienv = &param("general.mpienv", "openmpi_smp8_mark2");
my $vmem = &param("general.vmem", "6");
my $decoder_settings = &param("general.decoder-settings", "");

#
# Create the training script
#

#job control
my $working_dir = &param("general.working-dir");
system("mkdir -p $working_dir") == 0 or  die "Error: unable to create directory \"$working_dir\"";
my $train_script = "$name-train";
my $job_name = "$name-t";
my $hours = &param("train.hours",48);

#required training parameters

my $moses_ini_file = &param_required("train.moses-ini-file");
&check_exists ("moses ini file", $moses_ini_file);
my $input_file = &param_required("train.input-file");
&check_exists ("train input file", $input_file);
my $reference_files = &param_required("train.reference-files");
my $trainer_exe = &param_required("train.trainer");
&check_exists("Training executable", $trainer_exe);
#my $weights_file = &param_required("train.weights-file");
#&check_exists("weights file ", $weights_file);


#optional training parameters
my $epochs = &param("train.epochs", 2);
my $learner = &param("train.learner", "mira");
my $batch = &param("train.batch", 1);
my $extra_args = &param("train.extra-args");
my $continue_from_epoch = &param("train.continue-from-epoch", 0);
my $by_node = &param("train.by-node",0);
my $slots = &param("train.slots",8);
my $jobs = &param("train.jobs",8);

#test configuration
my ($test_input_file, $test_reference_file,$test_ini_file,$bleu_script,$use_moses);
my $test_exe = &param("test.moses");
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
my $weight_frequency = &param("test.frequency",1);
my $extra_memory_test = &param("test.extra-memory",0);

# adjust test frequency when using batches > 1
if ($batch > 1) {
    $weight_frequency = 1;
}

# check that number of jobs, dump frequency and number of input sentences are compatible
# shard size = number of input sentences / number of jobs, ensure shard size >= dump frequency
my $result = `wc -l $input_file`;
my @result = split(/\s/, $result);
my $inputSize = $result[0];
my $shardSize = $inputSize / $jobs;
if ($shardSize < $weight_frequency) {
    $weight_frequency = $shardSize;
    print "Warning: dump frequency must not be larger than shard size, setting dump frequency to $shardSize\n";
}

#file names
my $train_script_file = $working_dir . "/" . $train_script . ".sh"; 
my $train_out = $train_script . ".out";
my $train_err = $train_script . ".err";

#write the script
open TRAIN, ">$train_script_file" or die "Unable to open \"$train_script_file\" for writing";

&header(*TRAIN,$job_name,$working_dir,$slots,$jobs,$hours,$vmem,$train_out,$train_err);
if ($by_node) {
    print TRAIN "mpirun -np $jobs --bynode $trainer_exe \\\n";
}
else {
    print TRAIN "mpirun -np \$NSLOTS $trainer_exe \\\n";
}
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
#if ($weights_file) {
#    print TRAIN "-w $weights_file \\\n";
#}
print TRAIN "-l $learner \\\n";
print TRAIN "--weight-dump-stem $weight_file_stem \\\n";
print TRAIN "--weight-dump-frequency $weight_frequency \\\n";
print TRAIN "--epochs $epochs \\\n";
print TRAIN "-b $batch \\\n";
print TRAIN "--decoder-settings \"$decoder_settings\" \\\n";
print TRAIN $extra_args;
print TRAIN "\n";
print TRAIN "echo \"mpirun finished.\"\n";
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

# optionally continue from a later epoch (if $continue_from_epoch > 0)
if ($continue_from_epoch > 0) {
    $train_iteration += $continue_from_epoch;
    print "Continuing training from epoch $continue_from_epoch, with weights from ini file $moses_ini_file.\n";  
}

while(1) {
    my($epoch, $epoch_slice);
    $train_iteration += 1;
    my $new_weight_file = "$working_dir/$weight_file_stem" . "_";
    if ($weight_frequency == 1) {
	if ($train_iteration < 10) {
	    $new_weight_file  .= "0".$train_iteration;
	}
	else {
	    $new_weight_file  .= $train_iteration;
	}
    } else {
        #my $epoch = 1 + int $train_iteration / $weight_frequency;
        $epoch = int $train_iteration / $weight_frequency;
	$epoch_slice = $train_iteration % $weight_frequency;
        if ($epoch < 10) {
	    $new_weight_file .= "0" . $epoch . "_" . $epoch_slice;
	}
	else {
	    $new_weight_file .= $epoch . "_" . $epoch_slice;
	}	
    }
    
    if (-e "$working_dir/stopping") {
	print "Training finished at " . scalar(localtime()) . " because stopping criterion was reached.\n";
        exit 0;
    }
    else {
	print "Waiting for $new_weight_file\n";
	while ((! -e $new_weight_file) && &check_running($train_job_id)) {
	    sleep 10;
	}
	if (! -e $new_weight_file ) {
	    print "Training finished at " . scalar(localtime()) . "\n";
	    exit 0;
	}
    }

    #new weight file written. create test script and submit
    #file names
    my $job_name = $name . "_$train_iteration";
    my $test_script = "$name-test";
    my $test_script_file = $working_dir . "/" . $test_script . ".$train_iteration.sh"; 
    my $test_out = $test_script . ".$train_iteration.out";
    my $test_err = $test_script . ".$train_iteration.err";
    #my $output_file = $working_dir . "/" . $job_name . ".out";
    my $output_file;
    my $output_error_file;
    my $bleu_file;
    if ($weight_frequency == 1) {
	if ($train_iteration < 10) {
	    $output_file = $working_dir."/".$name."_0".$train_iteration.".out";
	    $output_error_file = $working_dir."/".$name."_0".$train_iteration.".err";
	    $bleu_file = $working_dir."/".$name."_0".$train_iteration.".bleu";
	}
	else {
	    $output_file = $working_dir."/".$name."_0".$train_iteration.".out";
	    $output_error_file = $working_dir."/".$name."_0".$train_iteration.".err";
	    $bleu_file = $working_dir."/".$name."_0".$train_iteration.".bleu";
	}        
    }
    else {
	if ($epoch < 10) {
	    $output_file = $working_dir."/".$name."_0".$epoch."_".$epoch_slice.".out";
	    $output_error_file = $working_dir."/".$name."_0".$epoch."_".$epoch_slice.".err";
	    $bleu_file = $working_dir."/".$name."_0".$epoch."_".$epoch_slice.".bleu";
	}
	else {
	    $output_file = $working_dir."/".$name."_".$epoch."_".$epoch_slice.".out";
	    $output_error_file = $working_dir."/".$name."_".$epoch."_".$epoch_slice.".err";
	    $bleu_file = $working_dir."/".$name."_".$epoch."_".$epoch_slice.".bleu";
	}        
    }

    if (! (open TEST, ">$test_script_file" )) {
        print "Warning: unable to create test script $test_script_file\n";
        next;
    }
    my $hours = &param("test.hours",12);
    my $extra_args = &param("test.extra-args");

    # Normalise the weights and splice them into the moses ini file.
    my ($default_weight,$wordpenalty_weight,$unknownwordpenalty_weight,@phrasemodel_weights,$lm_weight,$lm2_weight,$distortion_weight,@lexicalreordering_weights);
    # Check if there's a feature file, and a core feature. If there
    # is, then we read the core weights from there
    my $core_weight_file = $new_weight_file;
    my $have_core = 0;
    #if ($feature_file) {
    #  my $feature_config;
    #  if (!($feature_config = new Config::Simple($feature_file))) {
    #      print "Warning: unable to read feature file \"$feature_file\"";
    #      next;
    #  }
    #  if ($feature_config->param("core.weightfile")) {
    #    $core_weight_file = $feature_config->param("core.weightfile");
    #    $have_core = 1;
    #  }
    #}
    if (! (open WEIGHTS, "$core_weight_file")) {
        print "Warning: unable to open weights file $core_weight_file\n";
        next;
    }
    my $readExtraWeights = 0;
    my %extra_weights;
    while(<WEIGHTS>) {
        chomp;
	my ($name,$value) = split;
        #next if ($name =~ /^!Unknown/);
	next if ($name =~ /^BleuScore/);
        if ($name eq "DEFAULT_") {
            $default_weight = $value;
        } else {
            if ($name eq "WordPenalty") {
              $wordpenalty_weight = $value;
            } elsif ($name eq "!UnknownWordPenalty") {
              $unknownwordpenalty_weight = $value;
            } elsif ($name =~ /^PhraseModel/) {
              push @phrasemodel_weights,$value;
	    } elsif ($name =~ /^LM\:2/) {
              $lm2_weight = $value;
            }  
	    elsif ($name =~ /^LM/) {
              $lm_weight = $value;
            } elsif ($name eq "Distortion") {
              $distortion_weight = $value;
            } elsif ($name =~ /^LexicalReordering/) {
              push @lexicalreordering_weights,$value;
            } else {
              $extra_weights{$name} = $value;
	      $readExtraWeights += 1;
            }
        }
    }
    close WEIGHTS;

    print "Number of extra weights read: ".$readExtraWeights."\n";
    print "Number of extra weights stored: ". keys( %extra_weights ) ."\n";

    die "LM weight not defined" unless defined $lm_weight;

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

    # Normalising factor
    my $total = abs($wordpenalty_weight+$default_weight) + 
                abs($unknownwordpenalty_weight+$default_weight) +
                abs($lm_weight+$default_weight) +
                abs($distortion_weight+$default_weight);
    
    if (defined $lm2_weight) {
	$total += abs($lm2_weight + $default_weight);
    }

    foreach my $phrasemodel_weight (@phrasemodel_weights) {
        $total += abs($phrasemodel_weight + $default_weight);
    }
    foreach my $lexicalreordering_weight (@lexicalreordering_weights) {
        $total += abs($lexicalreordering_weight + $default_weight);
    }

    foreach my $extra_weight (keys %extra_weights) {
	$total += abs($extra_weights{$extra_weight} + $default_weight);
    }

    print "TOTAL: ", $total, "\n";
    
    # Create new ini file
    my $new_test_ini_file = $working_dir . "/" . $test_script . ".$train_iteration.ini";
    if (! (open NEWINI, ">$new_test_ini_file" )) {
        print "Warning: unable to create ini file $new_test_ini_file\n";
        next;
    }
    if (! (open OLDINI, "$test_ini_file" )) {
        print "Warning: unable to read ini file $test_ini_file\n";
        next;
    }

    my $verifyNormalisation = 0;
    my $uWeightCounted = 0;

    while(<OLDINI>) {
        if (/weight-l/) {
            print NEWINI "[weight-l]\n";
            print NEWINI ($lm_weight+$default_weight) / $total;
	    $verifyNormalisation += abs($lm_weight+$default_weight) / $total;
            print NEWINI "\n";

	    if (defined $lm2_weight) {
		readline(OLDINI);
		print NEWINI ($lm2_weight+$default_weight) / $total;
		$verifyNormalisation += abs($lm2_weight+$default_weight) / $total;
		print NEWINI "\n";
	    }

            readline(OLDINI);
        } elsif (/weight-t/) {
            print NEWINI "[weight-t]\n";
            foreach my $phrasemodel_weight (@phrasemodel_weights) {
                print NEWINI ($phrasemodel_weight+$default_weight) / $total;
		$verifyNormalisation += abs($phrasemodel_weight+$default_weight) / $total;
                print NEWINI "\n";
                readline(OLDINI);
            }
        } elsif (/weight-d/) {
            print NEWINI "[weight-d]\n";
            print NEWINI ($distortion_weight+$default_weight) / $total;
	    $verifyNormalisation += abs($distortion_weight+$default_weight) / $total;
            print NEWINI "\n";
            readline(OLDINI);
            foreach my $lexicalreordering_weight (@lexicalreordering_weights) {
                print NEWINI ($lexicalreordering_weight+$default_weight) / $total;
		$verifyNormalisation += abs($lexicalreordering_weight+$default_weight) / $total;
                print NEWINI "\n";
                readline(OLDINI);
            }
        } elsif (/weight-w/) {
            print NEWINI "[weight-w]\n";
            print NEWINI ($wordpenalty_weight+$default_weight) / $total;
	    $verifyNormalisation += abs($wordpenalty_weight+$default_weight) / $total;
            print NEWINI "\n";
            readline(OLDINI);
        } elsif (/weight-u/) { 
            print NEWINI "[weight-u]\n";
            print NEWINI ($unknownwordpenalty_weight+$default_weight) / $total;
	    $verifyNormalisation += abs($unknownwordpenalty_weight+$default_weight) / $total;
	    $uWeightCounted = 1;
        #    print NEWINI "\n";
        #    readline(OLDINI);
        } else {
            print NEWINI;
        }
    }
    close NEWINI;
    close OLDINI;
    
    if(!$uWeightCounted) {
	$verifyNormalisation += abs($unknownwordpenalty_weight+$default_weight) / $total;
    }

    print "Core weights add up to ", $verifyNormalisation, "\n";
    my $writtenExtraWeights = 0;

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
      foreach my $name (sort keys %extra_weights) {
        next if ($name eq "core");
        next if ($name eq "DEFAULT_");
        my $value = $extra_weights{$name} + $default_weight;
	
	# print only non-zero feature weights to file
	if ($value) {
	    $value /= $core_weight;
	    $value /= $total;
	    print EXTRAWEIGHT "$name $value\n";
	    $verifyNormalisation += abs($value);
	    $writtenExtraWeights += 1;
	} 
      }
    }

    print "Number of extra weights written to scale file: ", $writtenExtraWeights, "\n";
    print "All weights add up to ", $verifyNormalisation, "\n";

    print TEST "#!/bin/sh\n";
    print TEST "#\$ -N $job_name\n";
    print TEST "#\$ -wd $working_dir\n";
    print TEST "#\$ -l h_rt=$hours:00:00\n";
    print TEST "#\$ -o $test_out\n";
    print TEST "#\$ -e $test_err\n";
    print TEST "\n";
    # use same decoder settings than for experiment.perl evaluation, but omit -t option (segmentation)
    print TEST "$test_exe $decoder_settings -i $test_input_file -f $new_test_ini_file ";
    if ($extra_weight_file) {
      print TEST "-weight-file $extra_weight_file ";
    }
    print TEST $extra_args;
    print TEST " 1> $output_file 2> $output_error_file\n";
    print TEST "echo \"Decoding of test set finished.\"\n";
    print TEST "$bleu_script $test_reference_file < $output_file > $bleu_file\n";
    print TEST "echo \"Computed BLEU score of test set.\"\n";
    close TEST;

    #launch testing
    if ($have_sge) {
	if ($extra_memory_test) {
	    print "SUBMIT TEST JOB WITH EXTRA MEMORY: $extra_memory_test \n";
	    &submit_job_sge_extra_memory($test_script_file,$extra_memory_test);
	}
	else {
	    print "SUBMIT TEST JOB WITH NO EXTRA MEMORY\n";
	    &submit_job_sge($test_script_file);
	}
    } else {
      &submit_job_no_sge($test_script_file, $test_out,$test_err);
    }
}


sub param {
    my ($key,$default) = @_;
    my $value = $config->param($key);
    $value = $default if !$value;
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
    my ($OUT,$name,$working_dir,$slots,$jobs,$hours,$vmem,$out,$err) = @_;
    print $OUT "#!/bin/sh\n";
    if ($have_sge) {
      print $OUT "#\$ -N $name\n";
      print $OUT "#\$ -wd $working_dir\n";
      print $OUT "#\$ -pe $mpienv $slots\n";
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

sub submit_job_sge_extra_memory {
    my($script_file,$extra_memory) = @_;
    my $qsub_result = `qsub -pe $extra_memory -P $queue $script_file`;
    print "SUBMIT CMD: qsub -pe $extra_memory -P $queue $script_file \n";
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
      print "Executing script $script_file, writing to $out and $err.\n";
      `cd $working_dir; sh $script_file 1>$out 2> $err`;
    exit;
  } else {
    # Fork failed
    return 0;
  }
}

sub check_running {
  my ($job_id) = @_;
  if ($have_sge) {
    return `qstat | grep $job_id`; 
  } else {
    return `ps -e | grep $job_id | grep -v defunct`;
  }
}



