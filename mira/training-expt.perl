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

#wait for bleu files to appear in experiment folder if running as part of experiment.perl
my $wait_for_bleu = &param("general.wait-for-bleu", 0);

#job control
my $jackknife = &param("general.jackknife", 0);
my $working_dir = &param("general.working-dir");
my $general_decoder_settings = &param("general.decoder-settings", "");
system("mkdir -p $working_dir") == 0 or  die "Error: unable to create directory \"$working_dir\"";
my $train_script = "$name-train";
my $job_name = "$name-t";
my $hours = &param("train.hours",48);

# Check if a weight file with core weights was given
my $core_weight_file = &param("core.weightfile");

# Check if a weight file with core weights was given                                                                                       
my $start_weight_file = &param("start.weightfile");

#required training parameters
my $singleRef = 1;
my ($moses_ini_file, $input_file, $reference_files);
my (@moses_ini_files_folds, @input_files_folds, @reference_files_folds);
if ($jackknife) {
    my $array_ref = &param_required("train.moses-ini-files-folds");
    @moses_ini_files_folds= @$array_ref;
    foreach my $ini (@moses_ini_files_folds) {
	&check_exists ("moses ini file", $ini);
    }
    $array_ref = &param_required("train.input-files-folds");
    @input_files_folds = @$array_ref;
    foreach my $in (@input_files_folds) {
	&check_exists ("train input file", $in);
    }
    $array_ref = &param_required("train.reference-files-folds");
    @reference_files_folds = @$array_ref;
    foreach my $ref (@reference_files_folds) {
        &check_exists ("train reference file", $ref);
    }
}
else {
    $moses_ini_file = &param_required("train.moses-ini-file");
    &check_exists ("moses ini file", $moses_ini_file);
    $input_file = &param_required("train.input-file");
    &check_exists ("train input file", $input_file);
    $reference_files = &param_required("train.reference-files");
    if (&check_exists_noThrow ("ref files", $reference_files) != 0) {
	for my $ref (glob $reference_files . "*") {
	    &check_exists ("ref files", $ref);
	}
	$singleRef = 0;
    }
}

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
my $mixing_frequency = &param("train.mixing-frequency",0);
my $weight_dump_frequency = &param("train.weight-dump-frequency",0);
my $burn_in = &param("train.burn-in",0);
my $burn_in_input_file = &param("train.burn-in-input-file");
my $burn_in_reference_files = &param("train.burn-in-reference-files");
my $skipTrain = &param("train.skip", 0);
my $train_decoder_settings = &param("train.decoder-settings", "");
if (!$train_decoder_settings) {
    $train_decoder_settings = $general_decoder_settings;
}

#devtest configuration
my ($devtest_input_file, $devtest_reference_files,$devtest_ini_file,$bleu_script,$use_moses);
my $test_exe = &param("devtest.moses");
&check_exists("test executable", $test_exe);
$bleu_script  = &param_required("devtest.bleu");
&check_exists("multi-bleu script", $bleu_script);
$devtest_input_file = &param_required("devtest.input-file");
&check_exists ("devtest input file", $devtest_input_file);
$devtest_reference_files = &param_required("devtest.reference-file");
if (&check_exists_noThrow ("devtest ref file", $devtest_reference_files) != 0) {
    for my $ref (glob $devtest_reference_files . "*") {
	&check_exists ("devtest ref file", $ref);
    }
}
$devtest_ini_file = &param_required("devtest.moses-ini-file");
&check_exists ("devtest ini file", $devtest_ini_file);


my $weight_file_stem = "$name-weights";
my $extra_memory_devtest = &param("devtest.extra-memory",0);
my $skip_devtest = &param("devtest.skip-devtest",0);
my $skip_dev = &param("devtest.skip-dev",0);
my $skip_submit_test = &param("devtest.skip-submit",0);
my $devtest_decoder_settings = &param("devtest.decoder-settings", "");
if (!$devtest_decoder_settings) {
    $devtest_decoder_settings = $general_decoder_settings;
}


# check that number of jobs, dump frequency and number of input sentences are compatible
# shard size = number of input sentences / number of jobs, ensure shard size >= dump frequency
if ($jackknife) {
    # TODO..
}
else {
    my $result = `wc -l $input_file`;
    my @result = split(/\s/, $result);
    my $inputSize = $result[0];
    my $shardSize = $inputSize / $jobs;
    if ($mixing_frequency != 0) {
	if ($shardSize < $mixing_frequency) {
	    $mixing_frequency = int($shardSize);
	    if ($mixing_frequency == 0) {
		$mixing_frequency = 1;
	    }

	    print "Warning: mixing frequency must not be larger than shard size, setting mixing frequency to $mixing_frequency\n";
	}
    }

    if ($weight_dump_frequency != 0) {
	if ($shardSize < $weight_dump_frequency) {
	    $weight_dump_frequency = int($shardSize);
	    if ($weight_dump_frequency == 0) {
		$weight_dump_frequency = 1;
	    }
	    
	    print "Warning: weight dump frequency must not be larger than shard size, setting weight dump frequency to $weight_dump_frequency\n";
	}
    }

    if ($mixing_frequency != 0) {
	if ($mixing_frequency > ($shardSize/$batch)) {
	    $mixing_frequency = int($shardSize/$batch);
	    if ($mixing_frequency == 0) {
		$mixing_frequency = 1;
	    }
	    
	    print "Warning: mixing frequency must not be larger than (shard size/batch size), setting mixing frequency to $mixing_frequency\n";
	}
    }

    if ($weight_dump_frequency != 0) {
	if ($weight_dump_frequency > ($shardSize/$batch)) {
	    $weight_dump_frequency = int($shardSize/$batch);
	    if ($weight_dump_frequency == 0) {
		$weight_dump_frequency = 1;
	    }

	    print "Warning: weight dump frequency must not be larger than (shard size/batch size), setting weight dump frequency to $weight_dump_frequency\n";
	}
    }
}

#file names
my $train_script_file = $working_dir . "/" . $train_script . ".sh"; 
my $train_out = $train_script . ".out";
my $train_err = $train_script . ".err";
my $train_job_id = 0;

my @refs;
if (ref($reference_files) eq 'ARRAY') {
    @refs = @$reference_files;
} elsif ($singleRef){
    $refs[0] = $reference_files;
} else {
    @refs = glob $reference_files . "*"
}
my $arr_refs = \@refs;

if (!$skipTrain) {
#write the script
open TRAIN, ">$train_script_file" or die "Unable to open \"$train_script_file\" for writing";

&header(*TRAIN,$job_name,$working_dir,$slots,$jobs,$hours,$vmem,$train_out,$train_err);
if ($jobs == 1) {
    print TRAIN "$trainer_exe ";
}
else {
    if ($by_node) {
	print TRAIN "mpirun -np $jobs --bynode $trainer_exe \\\n";
    }
    else {
	print TRAIN "mpirun -np \$NSLOTS $trainer_exe \\\n";
    }
}

if ($jackknife) {
    foreach my $ini (@moses_ini_files_folds) {
	print TRAIN "--configs-folds $ini ";
    }
    print TRAIN "\\\n";
    foreach my $in (@input_files_folds) {
	print TRAIN "--input-files-folds $in ";
    }
    print TRAIN "\\\n";
    for my $ref (@reference_files_folds) {
        print TRAIN "--reference-files-folds $ref ";
    }
    print TRAIN "\\\n";
}
else {
    print TRAIN "-f $moses_ini_file \\\n";
    print TRAIN "-i $input_file \\\n";
    for my $ref (@refs) {
	print TRAIN "-r $ref ";
    }
    print TRAIN "\\\n";
}

if ($burn_in) {
    print TRAIN "--burn-in 1 \\\n";
    print TRAIN "--burn-in-input-file $burn_in_input_file \\\n";
    my @burnin_refs;
    if (ref($burn_in_reference_files) eq 'ARRAY') {
	@burnin_refs = @$burn_in_reference_files;
    } else {
	@burnin_refs = glob $burn_in_reference_files . "*"; # TODO:
    }
    for my $burnin_ref (@burnin_refs) {
	&check_exists("burn-in ref file",  $burnin_ref);
	print TRAIN "--burn-in-reference-files $burnin_ref ";
    }
    print TRAIN "\\\n";
}
#if ($weights_file) {
#    print TRAIN "-w $weights_file \\\n";
#}
if (defined $start_weight_file) {
    print TRAIN "--start-weights $start_weight_file \\\n"; 
}
elsif (defined $core_weight_file) {
    print TRAIN "--core-weights $core_weight_file \\\n";
}
print TRAIN "-l $learner \\\n";
print TRAIN "--weight-dump-stem $weight_file_stem \\\n";
print TRAIN "--mixing-frequency $mixing_frequency \\\n";
if ($weight_dump_frequency != -1) {
    print TRAIN "--weight-dump-frequency $weight_dump_frequency \\\n";
}
print TRAIN "--epochs $epochs \\\n";
print TRAIN "-b $batch \\\n";
print TRAIN "--decoder-settings \"$train_decoder_settings\" \\\n";
print TRAIN $extra_args;
print TRAIN "\n";
if ($jobs == 1) {
    print TRAIN "echo \"mira finished.\"\n";
}
else {
    print TRAIN "echo \"mpirun finished.\"\n";
}
close TRAIN;

if (! $execute) {
    print "Written train file: $train_script_file\n";
    exit 0;
}

#submit the training job
if ($have_sge) {
    $train_job_id = &submit_job_sge($train_script_file);
    
} else {
  $train_job_id = &submit_job_no_sge($train_script_file, $train_out,$train_err);
}

die "Failed to submit training job" unless $train_job_id;
}

#wait for the next weights file to appear, or the training job to end
my $train_iteration = -1;

# optionally continue from a later epoch (if $continue_from_epoch > 0)
if ($continue_from_epoch > 0) {
    $train_iteration = $continue_from_epoch - 1;
    print "Continuing training from epoch $continue_from_epoch, with weights from ini file $moses_ini_file.\n";  
}

while(1) {
    my($epoch, $epoch_slice);
    $train_iteration += 1;   # starts at 0
    my $new_weight_file = "$working_dir/$weight_file_stem" . "_";
    if ($weight_dump_frequency == 0) {
	print "No weights, no testing..\n";
	exit(0);
    }
    
    if ($weight_dump_frequency == 1) {
	if ($train_iteration < 10) {
	    $new_weight_file .= "0".$train_iteration;
	}
	else {
	    $new_weight_file .= $train_iteration;
	}
    } else {
        #my $epoch = 1 + int $train_iteration / $weight_dump_frequency;
        $epoch = int $train_iteration / $weight_dump_frequency;
	$epoch_slice = $train_iteration % $weight_dump_frequency;
        if ($epoch < 10) {
	    $new_weight_file .= "0".$epoch."_".$epoch_slice;
	}
	else {
	    $new_weight_file .= $epoch."_".$epoch_slice;
	}	
    }
    
    my $expected_num_files = $epochs*$weight_dump_frequency;
    if ($wait_for_bleu) {
	print "Expected number of BLEU files: $expected_num_files \n";
    }
    if (-e "$working_dir/stopping") {
	wait_for_bleu($expected_num_files) if ($wait_for_bleu);
	
	print "Training finished at " . scalar(localtime()) . " because stopping criterion was reached.\n";
        exit 0;
    }
    else {
	print "Waiting for $new_weight_file\n";
	if (!$skipTrain) {
	    while ((! -e $new_weight_file) && &check_running($train_job_id)) {
		sleep 10;
	    }
	}
	if (! -e $new_weight_file ) {
	    wait_for_bleu($expected_num_files) if ($wait_for_bleu);
	    
	    print "Training finished at " . scalar(localtime()) . "\n";
	    exit 0;
	}
    }

    #new weight file written. create test script and submit    
    my $suffix = "";
    print "weight file exists? ".(-e $new_weight_file)."\n";
    if (!$skip_devtest) {
	createTestScriptAndSubmit($epoch, $epoch_slice, $new_weight_file, $suffix, "devtest", $devtest_ini_file, $devtest_input_file, $devtest_reference_files, $skip_submit_test);
    }
    if (!$skip_dev) {
	createTestScriptAndSubmit($epoch, $epoch_slice, $new_weight_file, $suffix, "dev", $moses_ini_file, $input_file, $reference_files, $skip_submit_test);
    }
}

sub wait_for_bleu() {
    my $expected_num_files = $_[0];
    print "Waiting for $expected_num_files bleu files..\n";
    my @bleu_files = glob("*.bleu");
    while (scalar(@bleu_files) < $expected_num_files) {
	sleep 10;
	@bleu_files = glob("*.bleu");
    }
    print "$expected_num_files BLEU files completed, continue.\n"; 
}

sub createTestScriptAndSubmit {
    my $epoch = $_[0];
    my $epoch_slice = $_[1];
    my $new_weight_file = $_[2];
    my $suffix = $_[3];
    my $testtype = $_[4];
    my $old_ini_file = $_[5];
    my $input_file = $_[6];
    my $reference_file = $_[7];
    my $skip_submit = $_[8];

    #file names
    my $output_file;
    my $output_error_file;
    my $bleu_file;
    my $file_id = "";
    if ($weight_dump_frequency == 1) {
	if ($train_iteration < 10) {
	    $output_file = $working_dir."/".$name."_0".$train_iteration.$suffix."_$testtype".".out";
	    $output_error_file = $working_dir."/".$name."_0".$train_iteration.$suffix."_$testtype".".err";
	    $bleu_file = $working_dir."/".$name."_0".$train_iteration.$suffix."_$testtype".".bleu";
	    $file_id = "0".$train_iteration.$suffix;
	}
	else {
	    $output_file = $working_dir."/".$name."_".$train_iteration.$suffix."_$testtype".".out";
	    $output_error_file = $working_dir."/".$name."_".$train_iteration.$suffix."_$testtype".".err";
	    $bleu_file = $working_dir."/".$name."_".$train_iteration.$suffix."_$testtype".".bleu";
	    $file_id = $train_iteration.$suffix;
	}        
    }
    else {
	if ($epoch < 10) {
	    $output_file = $working_dir."/".$name."_0".$epoch."_".$epoch_slice.$suffix."_$testtype".".out";
	    $output_error_file = $working_dir."/".$name."_0".$epoch."_".$epoch_slice.$suffix."_$testtype".".err";
	    $bleu_file = $working_dir."/".$name."_0".$epoch."_".$epoch_slice.$suffix."_$testtype".".bleu";
	    $file_id = "0".$epoch."_".$epoch_slice.$suffix;
	}
	else {
	    $output_file = $working_dir."/".$name."_".$epoch."_".$epoch_slice.$suffix."_$testtype".".out";
	    $output_error_file = $working_dir."/".$name."_".$epoch."_".$epoch_slice.$suffix."_$testtype".".err";
	    $bleu_file = $working_dir."/".$name."_".$epoch."_".$epoch_slice.$suffix."_$testtype".".bleu";
	    $file_id = $epoch."_".$epoch_slice.$suffix;
	}        
    }

    my $job_name = $name."_".$testtype."_".$file_id;

    my $test_script = "$name-$testtype";
    my $test_script_file = "$working_dir/$test_script.$file_id.sh"; 
    my $test_out = "$test_script.$file_id.out";
    my $test_err = "$test_script.$file_id.err";

    if (! (open TEST, ">$test_script_file" )) {
        die "Unable to create test script $test_script_file\n";
    }
    
    my $hours = &param("test.hours",12);
    my $extra_args = &param("test.extra-args");

    # Splice the weights into the moses ini file.
    my ($default_weight,$wordpenalty_weight,@phrasemodel_weights,$lm_weight,$lm2_weight,$distortion_weight,@lexicalreordering_weights);

    if (! (open WEIGHTS, "$new_weight_file")) {
	die "Unable to open weights file $new_weight_file\n";
    }

    my $readCoreWeights = 0;
    my $readExtraWeights = 0;
    my %extra_weights;
    my $abs_weights = 0;
    while(<WEIGHTS>) {
        chomp;
	my ($name,$value) = split;
        next if ($name =~ /^!Unknown/);
	next if ($name =~ /^BleuScore/);
        if ($name eq "DEFAULT_") {
            $default_weight = $value;
        } else {
            if ($name eq "WordPenalty") {
              $wordpenalty_weight = $value;
	      $abs_weights += abs($value);
	      $readCoreWeights += 1;
            } elsif ($name =~ /^PhraseModel/) {
              push @phrasemodel_weights,$value;
	      $abs_weights += abs($value);
	      $readCoreWeights += 1;
	    } elsif ($name =~ /^LM\:2/) {
              $lm2_weight = $value;
	      $abs_weights += abs($value);
	      $readCoreWeights += 1;
            }  
	    elsif ($name =~ /^LM/) {
              $lm_weight = $value;
	      $abs_weights += abs($value);
	      $readCoreWeights += 1;
            } elsif ($name eq "Distortion") {
              $distortion_weight = $value;
	      $abs_weights += abs($value);
	      $readCoreWeights += 1;
            } elsif ($name =~ /^LexicalReordering/) {
              push @lexicalreordering_weights,$value;
	      $abs_weights += abs($value);
	      $readCoreWeights += 1;
            } else {
              $extra_weights{$name} = $value;
	      $readExtraWeights += 1;
            }
        }
    }
    close WEIGHTS;
    
    print "Number of core weights read: ".$readCoreWeights."\n";
    print "Number of extra weights read: ".$readExtraWeights."\n";
    
    # If there is a core weight file, we have to load the core weights from that file (NOTE: this is not necessary if the core weights are also printed to the weights file)
#    if (defined $core_weight_file) {
#	@phrasemodel_weights = ();
#	@lexicalreordering_weights = ();
#	$readCoreWeights = 0;
#	if (! (open CORE_WEIGHTS, "$core_weight_file")) {
#	    die "Unable to open core weights file $core_weight_file\n";
#	}
#	print "Reading core weights from file..\n";
#	while(<CORE_WEIGHTS>) {
#	    chomp;
#	    my ($name,$value) = split;
#	    next if ($name =~ /^!Unknown/);
#	    next if ($name =~ /^BleuScore/);
#	    if ($name eq "DEFAULT_") {
#		$default_weight = $value;
#	    } 
#	    else {
#		if ($name eq "WordPenalty") {
#		    $wordpenalty_weight = $value;
#		    $abs_weights += abs($value);
#		    $readCoreWeights += 1;
#		} elsif ($name =~ /^PhraseModel/) {
#		    push @phrasemodel_weights,$value;
#		    $abs_weights += abs($value);
#		    $readCoreWeights += 1;
#		} elsif ($name =~ /^LM\:2/) {
#		    $lm2_weight = $value;
#		    $abs_weights += abs($value);
#		    $readCoreWeights += 1;
#		}  
#		elsif ($name =~ /^LM/) {
#		    $lm_weight = $value;
#		    $abs_weights += abs($value);
#		    $readCoreWeights += 1;
#		} elsif ($name eq "Distortion") {
#		    $distortion_weight = $value;
#		    $abs_weights += abs($value);
#		    $readCoreWeights += 1;
#		} elsif ($name =~ /^LexicalReordering/) {
#		    push @lexicalreordering_weights,$value;	      
#		    $abs_weights += abs($value);
#		    $readCoreWeights += 1;
#		} else {
#		    # there should be no extra weights in the core weights file
#		    print "weight not matched: $name:$value\n";
#		}
#	    }
#	}
#	close CORE_WEIGHTS;
#	print "Number of core weights read: ".$readCoreWeights."\n";
#    }
          
    # Create new ini file (changing format: expt1-devtest.00_2.ini instead of expt1-devtest.3.ini)
    # my $new_ini_file = $working_dir."/".$test_script.".".$train_iteration.$suffix.".ini";
    my $new_ini_file = "$working_dir/$test_script.$file_id.ini";
    if (! (open NEWINI, ">$new_ini_file" )) {
        die "Unable to create ini file $new_ini_file\n";
    }
    if (! (open OLDINI, "$old_ini_file" )) {
        die "Unable to read ini file $old_ini_file\n";
    }

    # write normalized weights to ini file
    while(<OLDINI>) {
        if (/weight-l/) {
            print NEWINI "[weight-l]\n";
            print NEWINI ($lm_weight/$abs_weights);
            print NEWINI "\n";

	    if (defined $lm2_weight) {
		readline(OLDINI);
		print NEWINI ($lm2_weight/$abs_weights);
		print NEWINI "\n";
	    }

            readline(OLDINI);
        } elsif (/weight-t/) {
            print NEWINI "[weight-t]\n";
            foreach my $phrasemodel_weight (@phrasemodel_weights) {
                print NEWINI ($phrasemodel_weight/$abs_weights);
                print NEWINI "\n";
                readline(OLDINI);
            }
        } elsif (/weight-d/) {
            print NEWINI "[weight-d]\n";
            print NEWINI ($distortion_weight/$abs_weights);
            print NEWINI "\n";
            readline(OLDINI);
            foreach my $lexicalreordering_weight (@lexicalreordering_weights) {
                print NEWINI ($lexicalreordering_weight/$abs_weights);
                print NEWINI "\n";
                readline(OLDINI);
            }
        } elsif (/weight-w/) {
            print NEWINI "[weight-w]\n";
            print NEWINI ($wordpenalty_weight/$abs_weights);
            print NEWINI "\n";
            readline(OLDINI);
        } else {
            print NEWINI;
        }
    }
    close OLDINI;

    my $writtenExtraWeights = 0;

    # if there are any non-core weights, write them to a weights file (normalized)
    my $extra_weight_file = undef;
    if (%extra_weights) {
      $extra_weight_file = "$new_weight_file.sparse.scaled";
      if (! (open EXTRAWEIGHT,">$extra_weight_file")) {
        print "Warning: unable to create extra weights file $extra_weight_file";
        next;
      }
#      my $core_weight = 1;
#      if ($have_core) {
#        $default_weight = $extra_weights{"DEFAULT_"};
#        $core_weight = $extra_weights{"core"};
#      }
      foreach my $name (sort keys %extra_weights) {
        next if ($name eq "core");
        next if ($name eq "DEFAULT_");
        my $value = $extra_weights{$name}/$abs_weights;
	
	# write only non-zero feature weights to file
	if ($value) {
#	    $value /= $core_weight;
	    print EXTRAWEIGHT "$name $value\n";
	    $writtenExtraWeights += 1;
	} 
      }
    }

    # add specification of sparse weight file to ini
    print NEWINI "\n[weight-file] \n";
    print NEWINI "$extra_weight_file \n";
    close NEWINI;
    
    print TEST "#!/bin/sh\n";
    print TEST "#\$ -N $job_name\n";
    print TEST "#\$ -wd $working_dir\n";
    print TEST "#\$ -l h_rt=$hours:00:00\n";
    print TEST "#\$ -o $test_out\n";
    print TEST "#\$ -e $test_err\n";
    print TEST "\n";
    if ($have_sge) {
# some eddie specific stuff                                                                                                                                          
  	print TEST ". /etc/profile.d/modules.sh\n";
	print TEST "module load openmpi/ethernet/gcc/latest\n";
	print TEST "export LD_LIBRARY_PATH=/exports/informatics/inf_iccs_smt/shared/boost/lib:\$LD_LIBRARY_PATH\n";
    }
    print TEST "$test_exe $devtest_decoder_settings -i $input_file -f $new_ini_file ";
# now written to ini file
#    if ($extra_weight_file) {
#      print TEST "-weight-file $extra_weight_file ";
#    }
    print TEST $extra_args;
    print TEST " 1> $output_file 2> $output_error_file\n";
    print TEST "echo \"Decoding of ".$testtype." set finished.\"\n";
    print TEST "$bleu_script $reference_file < $output_file > $bleu_file\n";
    print TEST "echo \"Computed BLEU score of ".$testtype." set.\"\n";
    close TEST;

    #launch testing
    if(!$skip_submit) {
	if ($have_sge) {
	    if ($extra_memory_devtest) {
		print "Extra memory for test job: $extra_memory_devtest \n";
		&submit_job_sge_extra_memory($test_script_file,$extra_memory_devtest);
	    }
	    else {
		&submit_job_sge($test_script_file);
	    }
	} else {
	    &submit_job_no_sge($test_script_file, $test_out,$test_err);
	}
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
      if ($jobs != 1) {
	  print $OUT "#\$ -pe $mpienv $slots\n";
      }
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

sub check_exists_noThrow {
    my ($name,$filename) = @_;
    return 1 if ! -r $filename;
    return 0;
}

#
# Used to submit train/test jobs. Return the job id, or 0 on failure
#

sub submit_job_sge {
    my($script_file) = @_;
    my $qsub_result = `qsub -P $queue $script_file`;
    print "SUBMIT CMD: qsub -P $queue $script_file\n";
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



