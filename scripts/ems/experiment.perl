#!/usr/bin/perl -w

# Experiment Management System
# Documentation at http://www.statmt.org/moses/?n=FactoredTraining.EMS

use strict;
use Getopt::Long "GetOptions";
use FindBin qw($RealBin);

sub trim($)
{
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	return $string;
}

my $host = `hostname`; chop($host);
print STDERR "STARTING UP AS PROCESS $$ ON $host AT ".`date`;

my ($CONFIG_FILE,$EXECUTE,$NO_GRAPH,$CONTINUE,$FINAL_STEP,$FINAL_OUT,$VERBOSE,$IGNORE_TIME,$DELETE_CRASHED,$DELETE_VERSION);
my $SLEEP = 2;
my $META = "$RealBin/experiment.meta";

# check if it is run on a multi-core machine
# set number of maximal concurrently active processes
my ($MULTICORE,$MAX_ACTIVE) = (0,2);
&detect_if_multicore();

# check if running on a gridengine cluster
my $CLUSTER;
&detect_if_cluster();

# get command line options;
die("experiment.perl -config config-file [-exec] [-no-graph]")
    unless  &GetOptions('config=s' => \$CONFIG_FILE,
			'continue=i' => \$CONTINUE,
			'delete-crashed=i' => \$DELETE_CRASHED,
			'delete-run=i' => \$DELETE_VERSION,
			'delete-version=i' => \$DELETE_VERSION,
			'ignore-time' => \$IGNORE_TIME,
			'exec' => \$EXECUTE,
			'cluster' => \$CLUSTER,
			'multicore' => \$MULTICORE,
		   	'final-step=s' => \$FINAL_STEP,
		   	'final-out=s' => \$FINAL_OUT,
		   	'meta=s' => \$META,
			'verbose' => \$VERBOSE,
			'sleep=i' => \$SLEEP,
			'max-active=i' => \$MAX_ACTIVE,
			'no-graph' => \$NO_GRAPH);
if (! -e "steps") { `mkdir -p steps`; }

die("error: could not find config file") 
    unless ($CONFIG_FILE && -e $CONFIG_FILE) ||
   	   ($CONTINUE && -e &steps_file("config.$CONTINUE",$CONTINUE)) ||
   	   ($DELETE_CRASHED && -e &steps_file("config.$DELETE_CRASHED",$DELETE_CRASHED)) ||
   	   ($DELETE_VERSION && -e &steps_file("config.$DELETE_VERSION",$DELETE_VERSION));
$CONFIG_FILE = &steps_file("config.$CONTINUE",$CONTINUE) if $CONTINUE && !$CONFIG_FILE;
$CONFIG_FILE = &steps_file("config.$DELETE_CRASHED",$DELETE_CRASHED) if $DELETE_CRASHED;
$CONFIG_FILE = &steps_file("config.$DELETE_VERSION",$DELETE_VERSION) if $DELETE_VERSION;

my (@MODULE,
    %MODULE_TYPE,
    %MODULE_STEP,
    %STEP_IN,
    %STEP_OUT,
    %STEP_OUTNAME,    # output file name for step result
    %STEP_TMPNAME,    # tmp directory to be used by step
    %STEP_FINAL,      # output is part of the final model, not an intermediate step
    %STEP_PASS,       # config parameters that have to be set, otherwise pass
    %STEP_PASS_IF,    # config parameters that have to be not set, otherwise pass
    %STEP_IGNORE,     # config parameters that have to be set, otherwise ignore
    %STEP_IGNORE_IF,  # config parameters that have to be not set, otherwise ignore
    %QSUB_SCRIPT,     # flag if script contains qsub's when run on cluster
    %QSUB_STEP,       # flag if step contains qsub's when run on cluster
    %RERUN_ON_CHANGE, # config parameter whose change invalidates old runs
    %ONLY_EXISTENCE_MATTERS, # re-use check only on existance, not value
    %MULTIREF,	      # flag if step may be run on multiple sets (reference translations)
    %TEMPLATE,        # template if step follows a simple pattern
    %TEMPLATE_IF,     # part of template that is conditionally executed
    %ONLY_FACTOR_0,   # only run on a corpus that includes surface word
    %PARALLELIZE,     # flag, if step may be run through parallelizer
    %ERROR,           # patterns to check in stderr that indicate errors
    %NOT_ERROR);      # patterns that override general error indicating patterns
&read_meta();

print "LOAD CONFIG...\n";
my (@MODULE_LIST,  # list of modules (included sets) used
    %CONFIG);      # all (expanded) parameter settings from configuration file
&read_config();
print "working directory is ".&check_and_get("GENERAL:working-dir")."\n";
chdir(&check_and_get("GENERAL:working-dir"));

my $VERSION = 0;     # experiment number
$VERSION = $CONTINUE if $CONTINUE;
$VERSION = $DELETE_CRASHED if $DELETE_CRASHED;
$VERSION = $DELETE_VERSION if $DELETE_VERSION;

&compute_version_number() if $EXECUTE && !$CONTINUE && !$DELETE_CRASHED && !$DELETE_VERSION;
`mkdir -p steps/$VERSION` unless -d "steps/$VERSION";

&log_config() unless $DELETE_CRASHED || $DELETE_VERSION;
print "running experimental run number $VERSION\n";

print "\nESTABLISH WHICH STEPS NEED TO BE RUN\n";
my (%NEEDED,     # mapping of input files to step numbers
    %USES_INPUT, # mapping of step numbers to input files
    @DO_STEP,    # list of steps with fully specified name (LM:all:binarize)
    %STEP_LOOKUP,# mapping from step name to step number
    %PASS,       # steps for which no action needs to be taken
    %GIVEN);     # mapping of given output files to fully specified name
&find_steps();

print "\nFIND DEPENDENCIES BETWEEN STEPS\n";
my @DEPENDENCY;
&find_dependencies();

if (defined($DELETE_CRASHED)) {
  &delete_crashed($DELETE_CRASHED);
  exit;
}

if (defined($DELETE_VERSION)) {
  &delete_version($DELETE_VERSION);
  exit;
}

print "\nCHECKING IF OLD STEPS ARE RE-USABLE\n";
my @RE_USE;      # maps re-usable steps to older versions
my %RECURSIVE_RE_USE; # stores links from .INFO files that record prior re-use
&find_re_use();

print "\nDEFINE STEPS (run with -exec if everything ok)\n" unless $EXECUTE || $CONTINUE;
&define_step("all") unless $EXECUTE || $CONTINUE;
&init_agenda_graph();
&draw_agenda_graph();

print "\nEXECUTE STEPS\n" if $EXECUTE;
my (%DO,%DONE,%CRASHED);  # tracks steps that are currently processed or done
&execute_steps() if $EXECUTE;
&draw_agenda_graph();

exit();

### SUB ROUTINES
# graph that depicts steps of the experiment, with depedencies

sub init_agenda_graph() {
    my $dir = &check_and_get("GENERAL:working-dir");    

    my $graph_file = &steps_file("graph.$VERSION",$VERSION);
    open(PS,">".$graph_file.".ps") or die "Cannot open: $!";
    print PS "%!\n"
		."/Helvetica findfont 36 scalefont setfont\n"
		."72 72 moveto\n"
		."(its all gone blank...) show\n"
		."showpage\n";
    close(PS);

    `convert -alpha off $graph_file.ps $graph_file.png`;

    if (!$NO_GRAPH && !fork) {
	# use ghostview by default, it it is installed
	if (`which gv 2> /dev/null`) {
	  `gv -watch $graph_file.ps`;
        }
	# ... otherwise use graphviz's display
	else {
	  `display -update 10 $graph_file.png`;
	}
	#gotta exit the fork once the user has closed gv. Otherwise we'll have an extra instance of
	#experiment.perl floating around running steps in parallel with its parent.
	exit;
    }
}

# detection of cluster or multi-core machines

sub detect_machine {
    my ($hostname,$list) = @_;
    $list =~ s/\s+/ /;
    $list =~ s/^ //;
    $list =~ s/ $//;
    foreach my $machine (split(/ /,$list)) {
	return 1 if $hostname =~ /$machine/;
    }
    return 0;
}

sub detect_if_cluster {
    my $hostname = `hostname`; chop($hostname);
    foreach my $line (`cat $RealBin/experiment.machines`) {
	next unless $line =~ /^cluster: (.+)$/;
	if (&detect_machine($hostname,$1)) {
	    $CLUSTER = 1;
	    print "running on a cluster\n" if $CLUSTER;
        }
    }  
}

sub detect_if_multicore {
    my $hostname = `hostname`; chop($hostname);
    foreach my $line (`cat $RealBin/experiment.machines`) {
	next unless $line =~ /^multicore-(\d+): (.+)$/;
	my ($cores,$list) = ($1,$2);
	if (&detect_machine($hostname,$list)) {
	    $MAX_ACTIVE = $cores;
	    $MULTICORE = 1;
        }
    }
}

### Read the meta information about all possible steps

sub read_meta {
    open(META,$META) || die("ERROR: no meta file at $META");
    my ($module,$step);
    while(<META>) {
	s/\#.*$//; # strip comments
	next if /^\s*$/;	
	while (/\\\s*$/) {
	   $_ .= <META>;
           s/\s*\\\s*[\n\r]*\s+/ /;
        }
	if (/^\[(.+)\]\s+(\S+)/) {
	    $module = $1;
	    push @MODULE,$module;
	    $MODULE_TYPE{$module} = $2;
#	    print "MODULE_TYPE{$module} = $2;\n";
	}
	elsif (/^(\S+)/) {
	    $step = $1;
	    push @{$MODULE_STEP{$module}},$step;
#	    print "{MODULE_STEP{$module}},$step;\n";
	}
	elsif (/^\s+(\S+): (.+\S)\s*$/) {
	    if ($1 eq "in") {
		@{$STEP_IN{"$module:$step"}} = split(/\s+/,$2);
	    }
	    elsif ($1 eq "out") {
		$STEP_OUT{"$module:$step"} = $2;
	    }
	    elsif ($1 eq "default-name") {
		$STEP_OUTNAME{"$module:$step"} = $2;
	    }
	    elsif ($1 eq "tmp-name") {
		$STEP_TMPNAME{"$module:$step"} = $2;
	    }
	    elsif ($1 eq "final-model") {
		$STEP_FINAL{"$module:$step"} = $2;
	    }
	    elsif ($1 eq "pass-unless") {
		@{$STEP_PASS{"$module:$step"}} = split(/\s+/,$2);
		push @{$RERUN_ON_CHANGE{"$module:$step"}}, split(/\s+/,$2);
	    }
	    elsif ($1 eq "pass-if") {
		@{$STEP_PASS_IF{"$module:$step"}} = split(/\s+/,$2);
		push @{$RERUN_ON_CHANGE{"$module:$step"}}, split(/\s+/,$2);
	    }
	    elsif ($1 eq "ignore-unless") {
		$STEP_IGNORE{"$module:$step"} = $2;
	    }
	    elsif ($1 eq "ignore-if") {
		$STEP_IGNORE_IF{"$module:$step"} = $2;
	    }
	    elsif ($1 eq "qsub-script") {
		$QSUB_SCRIPT{"$module:$step"}++;
	    }
	    elsif ($1 eq "rerun-on-change") {
		push @{$RERUN_ON_CHANGE{"$module:$step"}}, split(/\s+/,$2);
	    }
	    elsif ($1 eq "only-existence-matters") {
		$ONLY_EXISTENCE_MATTERS{"$module:$step"}{$2}++;
	    }
	    elsif ($1 eq "multiref") {
		$MULTIREF{"$module:$step"} = $2;
	    }
	    elsif ($1 eq "template") {
		my $escaped_template = $2;
		$escaped_template =~ s/^IN/EMS_IN_EMS/;
		$escaped_template =~ s/ IN(\d*)/ EMS_IN$1_EMS/g;
		$escaped_template =~ s/ OUT/ EMS_OUT_EMS/g;
		$escaped_template =~ s/TMP/EMS_TMP_EMS/g;
		$TEMPLATE{"$module:$step"} = $escaped_template;
	    }
	    elsif ($1 eq "template-if") {
		my $escaped_template = $2;
		$escaped_template =~ s/^IN/EMS_IN_EMS/;
		$escaped_template =~ s/ IN(\d*)/ EMS_IN$1_EMS/g;
		$escaped_template =~ s/ OUT/ EMS_OUT_EMS/g;
		$escaped_template =~ s/TMP/EMS_TMP_EMS/g;
		my @IF = split(/\s+/,$escaped_template);
		push @{$TEMPLATE_IF{"$module:$step"}}, \@IF;
	    }
	    elsif ($1 eq "parallelizable") {
		$PARALLELIZE{"$module:$step"}++;
	    }
	    elsif ($1 eq "only-factor-0") {
		$ONLY_FACTOR_0{"$module:$step"}++;
	    }
	    elsif ($1 eq "error") {
		@{$ERROR{"$module:$step"}} = split(/,/,$2);
	    }
	    elsif ($1 eq "not-error") {
		@{$NOT_ERROR{"$module:$step"}} = split(/,/,$2);
	    }
	    else {
		die("META ERROR unknown parameter: $1");
	    }
	}
	else {
	    die("META ERROR buggy line $_");
	}
    }
    close(META);
}

### Read the configuration file

sub read_config {
    # read the file
    my $module = "GENERAL";
    my $error = 0;
    my $ignore = 0;
    my $line_count=0;
    open(INI,$CONFIG_FILE) || die("ERROR: CONFIG FILE NOT FOUND: $CONFIG_FILE");
    while(<INI>) {
	$line_count++;
	s/\#.*$//; # strip comments
	next if /^\#/ || /^\s*$/;
        while (/\\\s*$/) { # merge with next line
          s/\s*\\\s*$/ /;
          $_ .= <INI>;
        }
	if (/^\[(.+)\]/) {
	    $module = $1;
	    $ignore = /ignore/i;
	    push @MODULE_LIST,$1 unless $ignore;
	}
	elsif (! $ignore) {
	    if (/^(\S+) = (.+)$/) {
		my $parameter = $1;
		my $value = $2;
		$value =~ s/\s+/ /g;
		$value =~ s/^ //;
		$value =~ s/ $//;
                my @VALUE;
                if ($value =~ /^\"(.*)\"$/) {
                  @VALUE = ($1);
                }
                else {
		  @VALUE = split(/ /,$value);
                }
		$CONFIG{"$module:$parameter"} = \@VALUE;
	    }
	    else {
		print STDERR "BUGGY CONFIG LINE ($line_count): $_";
		$error++;
	    } 
	}
    }
    die("$error ERROR".(($error>1)?"s":"")." IN CONFIG FILE") if $error;

    # resolve parameters used in values
    my $resolve = 1;
    my $loop_count = 0;
    while($resolve && $loop_count++ < 100) {
	$resolve = 0;
	foreach my $parameter (keys %CONFIG) {
	    foreach (@{$CONFIG{$parameter}}) {
		next unless /\$/;
		my $escaped = 0;
		die ("BAD USE OF \$ IN VALUE used in parameter $parameter")
		    if ! ( /^(.*)\$([a-z\-\:\d]+)(.*)$/i ||
			  (/^(.*)\$\{([a-z\-\:\d]+)\}(.*)$/i && ($escaped = 1)));
		my ($pre,$substitution,$post) = ($1,$2,$3);
		my $pattern = $substitution;
		if ($substitution !~ /\:/) { # handle local variables
		    $parameter =~ /^(.+)\:/;
		    $substitution = $1.":".$substitution;
		}

		my $orig = $substitution;
		$substitution =~ s/^(.+):.+:(.+)$/$1:$2/ # not set-specific
		    unless defined($CONFIG{$substitution});
		$substitution = "GENERAL:$2" # back off to general
		    unless defined($CONFIG{$substitution});
		die ("UNKNOWN PARAMETER $orig used in parameter $parameter")
		    unless defined($CONFIG{$substitution});

		my $o = $CONFIG{$substitution}[0];
		print "changing $_ to " if $VERBOSE;
		s/\$\{$pattern\}/$o/ if $escaped;
		s/\$$pattern/$o/ unless $escaped;
		print "$_\n" if $VERBOSE;
		if (/\$/) { 
		    print "more resolving needed\n" if $VERBOSE;
		    $resolve = 1; 
		}
	    }
	}
    }
    close(INI);
    die("ERROR: CIRCULAR PARAMETER DEFINITION") if $resolve;

    # check if specified files exist
    $error = 0;
    foreach my $parameter (keys %CONFIG) {
	foreach (@{$CONFIG{$parameter}}) {	    
	    next if $parameter =~ /temp-dir/;
	    next if (!/^\// || -e);    # ok if not file, or exists
	    my $file = $_;	    
	    $file =~ s/ .+$//;         # remove switches
            my $gz = $file; $gz =~ s/\.gz$//; 
            next if -e $gz;            # ok if non gzipped exists
	    next if `find $file* -maxdepth 0 -follow`; # ok if stem
	    print STDERR "$parameter: file $_ does not exist!\n";
	    $error++;
	}
    }
    die if $error;
}

# log parameter settings into a file

sub log_config {
    my $dir = &check_and_get("GENERAL:working-dir");
    `mkdir -p $dir/steps`;
    my $config_file = &steps_file("config.$VERSION",$VERSION);
    `cp $CONFIG_FILE $config_file` unless $CONTINUE;
    open(PARAMETER,">".&steps_file("parameter.$VERSION",$VERSION)) or die "Cannot open: $!";
    foreach my $parameter (sort keys %CONFIG) {
	print PARAMETER "$parameter =";
	foreach (@{$CONFIG{$parameter}}) {
	    print PARAMETER " ".$_;
	}
	print PARAMETER "\n";
    }
    close(PARAMETER);
}

### find steps to run

sub find_steps {
    # find final output to be produced by the experiment
    if (defined($FINAL_OUT)) {
      push @{$NEEDED{$FINAL_OUT}}, "final";
    }
    elsif (!defined($FINAL_STEP)) {
      push @{$NEEDED{"REPORTING:report"}}, "final";
    }

    # go through each module
    while(1) {
      my $step_count_before = scalar(@DO_STEP);
      for(my $m=$#MODULE; $m>=0; $m--) {
	my $module = $MODULE[$m];

	# if module is "multiple" go through each set
	if ($MODULE_TYPE{$module} eq "multiple") {
	    my @SETS = &get_sets($module);
	    foreach my $set (@SETS) {
		&find_steps_for_module($module,$set);
	    }
	}

	# if module is "synchronous" go through each set of previous
	elsif ($MODULE_TYPE{$module} eq "synchronous") {
	    my $previous_module = $MODULE[$m-1];
	    my @SETS = &get_sets($previous_module);
	    foreach my $set (@SETS) {
		&find_steps_for_module($module,$set);
	    }
	}

	# otherwise, execute module once
	else {
	    &find_steps_for_module($module,"");
	}
    }
    last if $step_count_before == scalar(@DO_STEP);
  }
}

sub find_steps_for_module {
    my ($module,$set,$final_module) = @_;

    print "processing module $module:$set\n" if $VERBOSE;

    # go through potential steps from last to first (counter-chronological)
    foreach my $stepname (reverse @{$MODULE_STEP{$module}}) {

	my $step = &construct_name($module,$set,$stepname);
	my $defined_step = &defined_step($step); # without set
	next if defined($STEP_LOOKUP{$step});

	# FIRST, some checking...
	print "\tchecking step: $step\n" if $VERBOSE;

	# only add this step, if its output is needed by another step
	my $out = &construct_name($module,$set,$STEP_OUT{$defined_step});
	print "\t\tproduces $out\n" if $VERBOSE;
	next unless defined($NEEDED{$out}) || (defined($FINAL_STEP) && $FINAL_STEP eq $step);
	print "\t\tneeded\n" if $VERBOSE;
	
        # if output of a step is specified, you do not have 
        # to execute that step
	if(defined($CONFIG{$out})) {
	    $GIVEN{$out} = $step;
	    next;
	}
	print "\t\toutput not specified in config\n" if $VERBOSE;
	
	# not needed, if optional and not specified
	if (defined($STEP_IGNORE{$defined_step})) {
	    my $next = 0;
	    my $and = 0;
	    my @IGNORE = split(/ /,$STEP_IGNORE{$defined_step});
            if ($IGNORE[0] eq "AND") {
              $and = 1;
              shift @IGNORE;
            }
	    foreach my $ignore (@IGNORE) {
		my $extended_name = &extend_local_name($module,$set,$ignore);
		if (! &backoff_and_get($extended_name)) {
		    print "\t\tignored because of non-existance of ".$extended_name."\n" if $VERBOSE;
		    $next++;
		}
	    }
            next if !$and && ($next == scalar @IGNORE); # OR: all parameters have to be missing
            next if  $and && $next; # AND: any parameter has to be missing
	    print "\t\t=> not all non-existant, not ignored" if $next && $VERBOSE;
	}

	# not needed, if alternative step is specified
	if (defined($STEP_IGNORE_IF{$defined_step})) {
	    my $next = 0;
	    foreach my $ignore (split(/ /,$STEP_IGNORE_IF{$defined_step})) {
		my $extended_name = &extend_local_name($module,$set,$ignore);
		if (&backoff_and_get($extended_name)) {
		    print "\t\tignored because of existance of ".$extended_name."\n" if $VERBOSE;
		    $next++;
		}
	    }
	    next if $next;
	}

	# OK, add step to the list

	push @DO_STEP,$step;	    
	$STEP_LOOKUP{$step} = $#DO_STEP;
	print "\tdo-step: $step\n" if $VERBOSE;
	
	# mark as pass step (where no action is taken), if step is 
	# optional and nothing needs to be do done
	if (defined($STEP_PASS{$defined_step})) {
	    my $flag = 1;
	    foreach my $pass (@{$STEP_PASS{$defined_step}}) {
		$flag = 0 
		    if &backoff_and_get(&extend_local_name($module,$set,$pass));
	    }
	    $PASS{$#DO_STEP}++ if $flag;
	}

	if (defined($STEP_PASS_IF{$defined_step})) {
	    my $flag = 0;
	    foreach my $pass (@{$STEP_PASS_IF{$defined_step}}) {
		$flag = 1 
		    if &backoff_and_get(&extend_local_name($module,$set,$pass));
	    }
	    $PASS{$#DO_STEP}++ if $flag;
	}
	
	# special case for passing: steps that only affect factor 0
	if (defined($ONLY_FACTOR_0{$defined_step})) {
	    my $FACTOR = &backoff_and_get_array("LM:$set:factors");
	    if (defined($FACTOR)) {
		my $ok = 0;
		foreach my $factor (@{$FACTOR}) {
		    $ok++ if ($factor eq "word");
		}
		$PASS{$#DO_STEP}++ unless $ok;
	    }
	}

	# check for dependencies
	foreach (@{$STEP_IN{$defined_step}}) {
	    my $in = $_;

	    # if multiple potential inputs, find first that matches
	    if ($in =~ /=OR=/) {
    my @POTENTIAL_IN = split(/=OR=/,$in);
		foreach my $potential_in (@POTENTIAL_IN) {
		    if (&check_producability($module,$set,$potential_in)) {
			$in = $potential_in;
			last;
		    }

		}
		#die("ERROR: none of potential inputs $in possible for $step")
		$in = $POTENTIAL_IN[$#POTENTIAL_IN] if $in =~ /=OR=/;
	    }

	    # define input(s) as needed by this step
	    my @IN = &construct_input($module,$set,$in);
	    foreach my $in (@IN) {
		print "\t\tneeds input $in: " if $VERBOSE;
		if(defined($CONFIG{$in}) && $CONFIG{$in}[0] =~ /^\[(.+)\]$/) {
		    $in = $1;
		    print $in if $VERBOSE;
		    push @{$NEEDED{$in}}, $#DO_STEP;
		    print "\n\t\tcross-directed to $in\n" if $VERBOSE;
		}
		elsif(defined($CONFIG{$in})) {
		    print "\n\t\t... but that is specified\n" if $VERBOSE; 
		}
		else {
		    push @{$NEEDED{$in}}, $#DO_STEP;
	            print "\n" if $VERBOSE;
		}
		push @{$USES_INPUT{$#DO_STEP}},$in;
	    }
	}
    }
}

sub check_producability {
    my ($module,$set,$output) = @_;
    
    # find $output requested as input by step in $module/$set
    my @OUT = &construct_input($module,$set,$output);
    
    # if multiple outputs (due to multiple sets merged into one), 
    # only one needs to exist
    foreach my $out (@OUT) {
	print "producable? $out\n" if $VERBOSE;

	# producable, if specified as file in the command line
	return 1 if defined($CONFIG{$out});

	# find defined step that produces this
	$out =~ s/:.+:/:/g;
	my $defined_step;
	foreach my $ds (keys %STEP_OUT) {
	    my ($ds_module) = &deconstruct_name($ds);
	    my $ds_out = &construct_name($ds_module,"",$STEP_OUT{$ds});
	    print "checking $ds -> $ds_out\n" if $VERBOSE;
	    $defined_step = $ds if $out eq $ds_out;
	}
	die("ERROR: cannot possibly produce output $out")
	    unless $defined_step;

	# producable, if cannot be ignored
	return 1 unless defined($STEP_IGNORE{$defined_step});

	# producable, if required parameter specified
        foreach my $ignore (split(/ /,$STEP_IGNORE{$defined_step})) {
	    my ($ds_module) = &deconstruct_name($defined_step);
	    my $ds_set = $set;
	    $ds_set = "" if $MODULE_TYPE{$ds_module} eq "single";
	    my $req = &construct_name($ds_module,$ds_set,$ignore);
	    print "producable req $req\n" if $VERBOSE;
	    return 1 if defined($CONFIG{$req});
        }
    }
    print "not producable: ($module,$set,$output)\n" if $VERBOSE;
    return 0;
}

# given a current module and set, expand the input definition
# into actual input file parameters
sub construct_input {
    my ($module,$set,$in) = @_;

    # potentially multiple input files
    my @IN;
    
    # input from same module
    if ($in !~ /([^:]+):(\S+)/) {
	push @IN, &construct_name($module,$set,$in);
    }
    
    # input from previous model, multiple
    elsif ($MODULE_TYPE{$1} eq "multiple") {
	my @SETS = &get_sets($1);
	foreach my $set (@SETS) {
	    push @IN, &construct_name($1,$set,$2);
	}
    }
    # input from previous model, synchronized to multiple
    elsif ($1 eq "EVALUATION" && $module eq "REPORTING") {
	my @SETS = &get_sets("EVALUATION");
	foreach my $set (@SETS) {
	    push @IN, &construct_name($1,$set,$2);
	}
    }
    # input from previous module, single (ignore current set)
    else {
	push @IN,$in;
    }
    
    return @IN;
}

# get the set names for a module that runs on multiple sets
# (e.g. multiple LMs, multiple training corpora, multiple test sets)
sub get_sets {
    my ($config) = @_;
    my @SET;
    foreach (@MODULE_LIST) {
	if (/^$config:([^:]+)/) {
	    push @SET,$1;
	}
    }
    return @SET;
}

# DELETION OF STEPS AND VERSIONS
# delete step files for steps that have crashed
sub delete_crashed {
  my $crashed = 0;
  for(my $i=0;$i<=$#DO_STEP;$i++) {
    my $step_file = &versionize(&step_file($i),$DELETE_CRASHED);
    next unless -e $step_file;
    if (! -e $step_file.".DONE" ||     # interrupted (machine went down)
        &check_if_crashed($i,$DELETE_CRASHED,"no wait")) { # noted crash
      &delete_step($DO_STEP[$i],$DELETE_CRASHED);
      $crashed++;
    }
  }
  print "run with -exec to delete steps\n" if $crashed && !$EXECUTE;
  print "nothing to do\n" unless $crashed;
}

# delete all step and data files for a version
sub delete_version {

  # check which versions are already deleted
  my %ALREADY_DELETED;
  my $dir = &check_and_get("GENERAL:working-dir");    
  open(VERSION,"ls $dir/steps/*/deleted.* 2>/dev/null|");
  while(<VERSION>) {
    /deleted\.(\d+)/;
    $ALREADY_DELETED{$1}++;
  }
  close(VERSION);

  # check if any of the steps are re-used by other versions
  my (%USED_BY_OTHERS,%DELETABLE,%NOT_DELETABLE);
  open(VERSION,"ls $dir/steps|");
  while(my $version = <VERSION>) {
    chop($version);
    next if $version !~ /^\d+/ || $version == 0;
    open(RE_USE,"steps/$version/re-use.$version");
    while(<RE_USE>) {
      next unless /^(.+) (\d+)$/;
      my ($step,$re_use_version) = ($1,$2);

      # a step in the current version that is used in other versions
      $USED_BY_OTHERS{$step}++ if $re_use_version == $DELETE_VERSION && !defined($ALREADY_DELETED{$version});

      # potentially deletable step in already deleted version that current version uses
      push @{$DELETABLE{$re_use_version}}, $step if $version == $DELETE_VERSION && defined($ALREADY_DELETED{$re_use_version});

      # not deletable step used by not-deleted version
      $NOT_DELETABLE{$re_use_version}{$step}++ if $version != $DELETE_VERSION && !defined($ALREADY_DELETED{$version});
    }
    close(RE_USE);
  }

  # go through all steps for which step files where created
  open(STEPS,"ls $dir/steps/$DELETE_VERSION/[A-Z]*.$DELETE_VERSION|");
  while(my $step_file = <STEPS>) {
    chomp($step_file);
    my $step = &get_step_from_step_file($step_file);
    next if $USED_BY_OTHERS{$step};
    &delete_step($step,$DELETE_VERSION); 
  }

  # orphan killing: delete steps in deleted versions, if they were only preserved because this version needed them
  foreach my $version (keys %DELETABLE) {
    foreach my $step (@{$DELETABLE{$version}}) {
      next if defined($NOT_DELETABLE{$version}) && defined($NOT_DELETABLE{$version}{$step});
      &delete_step($step,$version);
    }
  }
  my $deleted_flag_file = &steps_file("deleted.$DELETE_VERSION",$DELETE_VERSION);
  `touch $deleted_flag_file` if $EXECUTE;
}

sub get_step_from_step_file {
  my ($step) = @_;
  $step =~ s/^.+\///;
  $step =~ s/\.\d+$//;
  $step =~ s/_/:/g;
  return $step;
}
 
sub delete_step {
  my ($step_name,$version) = @_;
  my ($module,$set,$step) = &deconstruct_name($step_name);

  my $step_file = &versionize(&step_file2($module,$set,$step),$version); 
  print "delete step $step_file\n";
  `rm $step_file $step_file.*` if $EXECUTE;

  my $out_file = $STEP_OUTNAME{"$module:$step"};
  $out_file =~ s/^(.+\/)([^\/]+)$/$1$set.$2/g if $set;
  &delete_output(&versionize(&long_file_name($out_file,$module,$set), $version));

  if (defined($STEP_TMPNAME{"$module:$step"})) {
    my $tmp_file = &get_tmp_file($module,$set,$step,$version);
    &delete_output($tmp_file);
  }
}

# delete output files that match a given prefix
sub delete_output {
  my ($file) = @_;
  # delete directory that matches exactly
  if (-d $file) {
    print "\tdelete directory $file\n";
    `rm -r $file` if $EXECUTE;
  }
  # delete regular file that matches exactly
  if (-e $file) {
    print "\tdelete file $file\n";
    `rm $file` if $EXECUTE;
  } 
  # delete files that have additional extension
  $file =~ /^(.+)\/([^\/]+)$/;
  my ($dir,$f) = ($1,$2);
  my @FILES = `ls $file.* 2>/dev/null`;
  foreach (`ls $dir`) {
    chop;
    next unless substr($_,0,length($f)) eq $f;
    if (-e $_) {
      print "\tdelete file $dir/$_\n";
      `rm $dir/$_` if $EXECUTE;
    }
    else {
      print "\tdelete directory $dir/$_\n";
      `rm -r $dir/$_` if $EXECUTE;
    }
  }
}

# RE-USE
# look for completed step jobs from previous experiments
sub find_re_use {
    my $dir = &check_and_get("GENERAL:working-dir");    
    return unless -e "$dir/steps";

    for(my $i=0;$i<=$#DO_STEP;$i++) {
	%{$RE_USE[$i]} = ();
    }

    # find older steps from previous versions that can be re-used
    open(LS,"find $dir/steps/* -maxdepth 1 -follow | sort -r |");
    while(my $info_file = <LS>) {
	next unless $info_file =~ /INFO$/;
	$info_file =~ s/.+\/([^\/]+)$/$1/; # ignore path
	for(my $i=0;$i<=$#DO_STEP;$i++) {
#	    next if $RE_USE[$i]; # already found one
	    my $pattern = &step_file($i);
	    $pattern =~ s/\+/\\+/; # escape plus signs in file names
	    $pattern = "^$pattern.(\\d+).INFO\$";
	    $pattern =~ s/.+\/([^\/]+)$/$1/; # ignore path
	    next unless $info_file =~ /$pattern/;
	    my $old_version = $1;
	    print "re_use $i $DO_STEP[$i] (v$old_version) ".join(" ",keys %{$RE_USE[$i]})." ?\n" if $VERBOSE;
            print "\tno info file ".&versionize(&step_file($i),$old_version).".INFO\n" if ! -e &versionize(&step_file($i),$old_version).".INFO" && $VERBOSE;
            print "\tno done file " if ! -e &versionize(&step_file($i),$old_version).".DONE" && $VERBOSE;
	    if (! -e &versionize(&step_file($i),$old_version).".INFO") {
		print "\tinfo file does not exist\n" if $VERBOSE;
		print "\tnot re-usable\n" if $VERBOSE;
	    }
	    elsif (! -e &versionize(&step_file($i),$old_version).".DONE") {
		print "\tstep not done (done file does not exist)\n" if $VERBOSE;
		print "\tnot re-usable\n" if $VERBOSE;
	    }
	    elsif (! &check_info($i,$old_version) ) {
		print "\tparameters from info file do not match\n" if $VERBOSE;
		print "\tnot re-usable\n" if $VERBOSE;
	    }
	    elsif (&check_if_crashed($i,$old_version)) {
		print "\tstep crashed\n" if $VERBOSE;
		print "\tnot re-usable\n" if $VERBOSE;
	    }
	    else {
		$RE_USE[$i]{$old_version}++;
		print "\tre-usable\n" if $VERBOSE;
	    }
	}
    }
    close(LS);

    # all preceding steps have to be re-usable
    # otherwise output from old step can not be re-used
    my $change = 1;
    while($change) {
	$change = 0;

	for(my $i=0;$i<=$#DO_STEP;$i++) {
	    next unless $RE_USE[$i];
	    foreach my $run (keys %{$RE_USE[$i]}) {
		print "check on dependencies for $i ($run) $DO_STEP[$i]\n" if $VERBOSE;
		foreach (@{$DEPENDENCY[$i]}) {
		    my $parent = $_;
		    print "\tchecking on $parent $DO_STEP[$parent]\n" if $VERBOSE;
		    my @PASSING;
		    # skip steps that are passed
		    while (defined($PASS{$parent})) {
			if (scalar (@{$DEPENDENCY[$parent]}) == 0) {
			    $parent = 0;
			    print "\tprevious step's output is specified\n" if $VERBOSE;
			}
			else {
			    push @PASSING, $parent;
			    $parent = $DEPENDENCY[$parent][0];
			    print "\tmoving up to $parent $DO_STEP[$parent]\n" if $VERBOSE;
			}
		    }
		    # check if parent step may be re-used
		    if ($parent) {
			my $reuse_run = $run;
			# if recursive re-use, switch to approapriate run
			if (defined($RECURSIVE_RE_USE{$i,$run,$DO_STEP[$parent]})) {
			    print "\trecursive re-use run $reuse_run\n" if $VERBOSE;
			    $reuse_run = $RECURSIVE_RE_USE{$i,$run,$DO_STEP[$parent]};
			}
			# additional check for straight re-use
			else {
			    # re-use step has to have passed the same steps
			    foreach (@PASSING) {
				my $passed = $DO_STEP[$_];
				$passed =~ s/:/_/g;
				if (-e &steps_file("$passed.$run",$run)) {
				    delete($RE_USE[$i]{$run});
				    $change = 1;
				    print "\tpassed step $DO_STEP[$_] used in re-use run $run -> fail\n" if $VERBOSE;
				}
			    } 
			}
			# re-use step has to exist for this run
			if (! defined($RE_USE[$parent]{$reuse_run})) {
			    print "\tno previous step -> fail\n" if $VERBOSE;
			    delete($RE_USE[$i]{$run});
			    $change = 1;	
			}
		    }
		}
	    }
	}
    }

    # summarize and convert hashes into integers for to be re-used 
    print "\nSTEP SUMMARY:\n";
    open(RE_USE,">".&steps_file("re-use.$VERSION",$VERSION)) or die "Cannot open: $!";
    for(my $i=$#DO_STEP;$i>=0;$i--) {
        if ($PASS{$i}) {
	    $RE_USE[$i] = 0;
            next;
        }
        print "$i $DO_STEP[$i] ->\t";
	if (scalar(keys %{$RE_USE[$i]})) {
	    my @ALL = sort { $a <=> $b} keys %{$RE_USE[$i]};
            print "re-using (".join(" ",@ALL).")\n";
	    $RE_USE[$i] = $ALL[0];
            if ($ALL[0] != $VERSION) {
	      print RE_USE "$DO_STEP[$i] $ALL[0]\n";
            }
	}
	else {
	    print "run\n";
	    $RE_USE[$i] = 0;
	}
    }
    close(RE_USE);
}

sub find_dependencies {
    for(my $i=0;$i<=$#DO_STEP;$i++) {
	@{$DEPENDENCY[$i]} = ();
    }
    for(my $i=0;$i<=$#DO_STEP;$i++) {
	my $step = $DO_STEP[$i];
	$step =~ /^(.+:)[^:]+$/; 
	my $module_set = $1;
	foreach my $needed_by (@{$NEEDED{$module_set.$STEP_OUT{&defined_step($step)}}}) {
	    print "$needed_by needed by $i\n" if $VERBOSE;
	    next if $needed_by eq 'final';
	    push @{$DEPENDENCY[$needed_by]},$i;
	}
    }

#    for(my $i=0;$i<=$#DO_STEP;$i++) {
#	print "to run step $i ($DO_STEP[$i]), we first need to run step(s) ".join(" ",@{$DEPENDENCY[$i]})."\n";
#    }
}

sub draw_agenda_graph {
    my %M;
    my $dir = &check_and_get("GENERAL:working-dir");
    open(DOT,">".&steps_file("graph.$VERSION.dot",$VERSION)) or die "Cannot open: $!";
    print DOT "digraph Experiment$VERSION {\n";
    print DOT "  ranksep=0;\n";
    for(my $i=0;$i<=$#DO_STEP;$i++) {
	my $step = $DO_STEP[$i];
	$step =~ /^(.+):[^:]+$/; 
	my $module_set = $1;
	push @{$M{$module_set}},$i; 
    }
    my $i = 0;
    my (@G,%GIVEN_NUMBER);
    foreach (values %GIVEN) {
	push @G,$_;
	$GIVEN_NUMBER{$_} = $#G;
	/^(.+):[^:]+$/;
	my $module_set = $1;
	push @{$M{$module_set}},"g".($#G);
    }
    my $m = 0;
    foreach my $module (keys %M) {
	print DOT "  subgraph cluster_".($m++)." {\n";
	print DOT "    fillcolor=\"lightyellow\";\n";
	print DOT "    shape=box;\n";
	print DOT "    style=filled;\n";
	print DOT "    fontsize=10;\n";
	print DOT "    label=\"$module\";\n";
	foreach my $i (@{$M{$module}}) {
	    if ($i =~ /g(\d+)/) {
		my $step = $G[$1];
		$step =~ /^.+:([^:]+)$/;
		print DOT "    $i [label=\"$1\",shape=box,fontsize=10,height=0,style=filled,fillcolor=\"#c0b060\"];\n";
	    }
	    else {
		my $step = $DO_STEP[$i];
		$step =~ s/^.+:([^:]+)$/$1/; 
		$step .= " (".$RE_USE[$i].")" if $RE_USE[$i];

		my $color = "green";
		$color = "#0000ff" if defined($DO{$i}) && $DO{$i} >= 1;
		$color = "#8080ff" if defined($DONE{$i}) || ($RE_USE[$i] && $RE_USE[$i] == $VERSION);
		$color = "lightblue" if $RE_USE[$i] && $RE_USE[$i] != $VERSION;
		$color = "red" if defined($CRASHED{$i});
		$color = "lightyellow" if defined($PASS{$i});
		
		print DOT "    $i [label=\"$step\",shape=box,fontsize=10,height=0,style=filled,fillcolor=\"$color\"];\n";
	    }
	}
	print DOT "  }\n";
    }
    for(my $i=0;$i<=$#DO_STEP;$i++) {
	foreach (@{$DEPENDENCY[$i]}) {
	    print DOT "  $_ -> $i;\n";
	}
    }

    # steps that do not have to be performed, because
    # their output is given
    foreach my $out (keys %GIVEN) {
	foreach my $needed_by (@{$NEEDED{$out}}) {
	    print DOT "  g".$GIVEN_NUMBER{$GIVEN{$out}}." -> $needed_by;\n";
	}
    }

    print DOT "}\n";
    close(DOT);
    my $graph_file = &steps_file("graph.$VERSION",$VERSION);
    `dot -Tps $graph_file.dot >$graph_file.ps`;
    `convert -alpha off $graph_file.ps $graph_file.png`;
}

sub define_step {
    my ($step) = @_;
    my $dir = &check_and_get("GENERAL:working-dir");    
    `mkdir -p $dir` if ! -e $dir;
    my @STEP;
    if ($step eq "all") {
	for(my $i=0;$i<=$#DO_STEP;$i++) {
	    push @STEP,$i;
	}
    }
    else {
	@STEP = ($step);
    }
    foreach my $i (@STEP) {
	next if $RE_USE[$i];
	next if defined($PASS{$i});
	next if &define_template($i);
        if ($DO_STEP[$i] =~ /^CORPUS:(.+):factorize$/) {
            &define_corpus_factorize($i);
        }	
	elsif ($DO_STEP[$i] eq 'SPLITTER:train') {
	    &define_splitter_train($i);
	}	
        elsif ($DO_STEP[$i] =~ /^LM:(.+):factorize$/) {
            &define_lm_factorize($i,$1);
        }
	elsif ($DO_STEP[$i] =~ /^LM:(.+):randomize$/ || 
	       $DO_STEP[$i] eq 'INTERPOLATED-LM:randomize') {
            &define_lm_randomize($i,$1);
        }
	elsif ($DO_STEP[$i] =~ /^LM:(.+):train-randomized$/) {
	    &define_lm_train_randomized($i,$1);
	}
        elsif ($DO_STEP[$i] eq 'TRAINING:prepare-data') {
            &define_training_prepare_data($i);
        }
        elsif ($DO_STEP[$i] eq 'TRAINING:prepare-data-fast-align') {
            &define_training_prepare_data_fast_align($i);
        }
        elsif ($DO_STEP[$i] eq 'TRAINING:run-giza') {
            &define_training_run_giza($i);
        }
        elsif ($DO_STEP[$i] eq 'TRAINING:run-giza-inverse') {
            &define_training_run_giza_inverse($i);
        }
        elsif ($DO_STEP[$i] eq 'TRAINING:symmetrize-giza') {
            &define_training_symmetrize_giza($i);
        }
	elsif ($DO_STEP[$i] eq 'TRAINING:build-biconcor') {
            &define_training_build_biconcor($i);
	}
	elsif ($DO_STEP[$i] eq 'TRAINING:build-suffix-array') {
            &define_training_build_suffix_array($i);
	}

        elsif ($DO_STEP[$i] eq 'TRAINING:build-lex-trans') {
            &define_training_build_lex_trans($i);
        }
        elsif ($DO_STEP[$i] eq 'TRAINING:extract-phrases') {
            &define_training_extract_phrases($i);
        }
        elsif ($DO_STEP[$i] eq 'TRAINING:build-reordering') {
            &define_training_build_reordering($i);
        }
	elsif ($DO_STEP[$i] eq 'TRAINING:build-ttable') {
	    &define_training_build_ttable($i);
        }
        elsif ($DO_STEP[$i] eq 'TRAINING:build-transliteration-model') {
            &define_training_build_transliteration_model($i);
        }
	elsif ($DO_STEP[$i] eq 'TRAINING:build-generation') {
            &define_training_build_generation($i);
        }
	elsif ($DO_STEP[$i] eq 'TRAINING:sigtest-filter-ttable' ||
	       $DO_STEP[$i] eq 'TRAINING:sigtest-filter-reordering') {
            &define_training_sigtest_filter($i);
        }
	elsif ($DO_STEP[$i] eq 'TRAINING:create-config' || $DO_STEP[$i] eq 'TRAINING:create-config-interpolated-lm') {
	    &define_training_create_config($i);
	}
	elsif ($DO_STEP[$i] eq 'INTERPOLATED-LM:factorize-tuning') {
	    &define_interpolated_lm_factorize_tuning($i);
	}
	elsif ($DO_STEP[$i] eq 'INTERPOLATED-LM:interpolate') {
	    &define_interpolated_lm_interpolate($i);
	}
	elsif ($DO_STEP[$i] eq 'INTERPOLATED-LM:binarize' ||
         $DO_STEP[$i] eq 'INTERPOLATED-LM:quantize' ||
         $DO_STEP[$i] eq 'INTERPOLATED-LM:randomize') {
	    &define_interpolated_lm_process($i);
	}
	elsif ($DO_STEP[$i] eq 'TUNING:factorize-input') {
            &define_tuningevaluation_factorize($i);
        }	
	elsif ($DO_STEP[$i] eq 'TUNING:factorize-input-devtest') {
            &define_tuningevaluation_factorize($i);
        }
 	elsif ($DO_STEP[$i] eq 'TUNING:filter') {
	    &define_tuningevaluation_filter(undef,$i);
	}
	elsif ($DO_STEP[$i] eq 'TUNING:filter-devtest') {
	    &define_tuningevaluation_filter(undef,$i);
	}
 	elsif ($DO_STEP[$i] eq 'TUNING:tune') {
	    &define_tuning_tune($i);
	}
        elsif ($DO_STEP[$i] =~ /^EVALUATION:(.+):factorize-input$/) {
            &define_tuningevaluation_factorize($i);
        }	
	elsif ($DO_STEP[$i] =~ /^EVALUATION:(.+):filter$/) {
	    &define_tuningevaluation_filter($1,$i);
	}
	elsif ($DO_STEP[$i] =~ /^EVALUATION:(.+):decode$/) {
	    &define_evaluation_decode($1,$i);
	}
	elsif ($DO_STEP[$i] =~ /^EVALUATION:(.+):analysis$/) {
	    &define_evaluation_analysis($1,$i);
	}
	elsif ($DO_STEP[$i] =~ /^EVALUATION:(.+):analysis-precision$/) {
	    &define_evaluation_analysis_precision($1,$i);
	}
	elsif ($DO_STEP[$i] =~ /^EVALUATION:(.+):analysis-coverage$/) {
	    &define_evaluation_analysis_coverage($1,$i);
	}
	elsif ($DO_STEP[$i] =~ /^EVALUATION:(.+):meteor$/) {
#	    &define_evaluation_meteor($1);
	}
	elsif ($DO_STEP[$i] =~ /^EVALUATION:(.+):ter$/) {
#	    &define_evaluation_ter($1);
	}
	elsif ($DO_STEP[$i] eq 'REPORTING:report') {
	    &define_reporting_report($i);
	}
	else {
	    print STDERR "ERROR: unknown step $DO_STEP[$i]\n";
	    exit;
	}
    }
}

# LOOP that executes the steps 
# including checks, if needed to be executed, waiting for completion, and error detection

sub execute_steps {
    my $running_file = &steps_file("running.$VERSION",$VERSION);
    `touch $running_file`;

    for(my $i=0;$i<=$#DO_STEP;$i++) {
	$DONE{$i}++ if $RE_USE[$i];
    }

    my $active = 0;
    while(1) {

	# find steps to be done
  my $repeat_if_passed = 1;
  while($repeat_if_passed) {
    $repeat_if_passed = 0;
	  for(my $i=0;$i<=$#DO_STEP;$i++) {
	    next if (defined($DONE{$i}));
	    next if (defined($DO{$i}));
	    next if (defined($CRASHED{$i}));
	    my $doable = 1;
	    # can't do steps whose predecedents are not done yet
	    foreach my $prev_step (@{$DEPENDENCY[$i]}) {
		$doable = 0 if !defined($DONE{$prev_step});
	    }
      next unless $doable;
      $DO{$i} = 1;

      # immediately label pass steps as done
	    next unless defined($PASS{$i});
      $DONE{$i} = 1;
		  delete($DO{$i});
      $repeat_if_passed = 1;
    }
  }

  print "number of steps doable or running: ".(scalar keys %DO)." at ".`date`;
  foreach my $step (keys %DO) { print "\t".($DO{$step}==2?"running: ":"doable: ").$DO_STEP[$step]."\n"; }
	return unless scalar keys %DO;
	
	# execute new step
	my $done = 0;
	foreach my $i (keys %DO) {
	    next unless $DO{$i} == 1;
	    if (defined($PASS{$i})) { # immediately label pass steps as done
		$DONE{$i}++;
		delete($DO{$i});
		$done++;
	    }
	    elsif (! -e &versionize(&step_file($i)).".DONE") {
		my $step = &versionize(&step_file($i));
		&define_step($i);
		&write_info($i);

		# cluster job submission
		if ($CLUSTER && ! &is_qsub_script($i)) {
		    $DO{$i}++;
		    my $qsub_args = &get_qsub_args($DO_STEP[$i]);		    
		    print "\texecuting $step via qsub ($active active)\n";
		    my $qsub_command="qsub $qsub_args -S /bin/bash -e $step.STDERR -o $step.STDOUT $step";
		    print "\t$qsub_command\n" if $VERBOSE;
		    `$qsub_command`;
		}

		# execute in fork
		elsif ($CLUSTER || $active < $MAX_ACTIVE) {
		    $active++;
		    $DO{$i}++;
		    print "\texecuting $step via sh ($active active)\n";
		    sleep(5);
		    if (!fork) {
		        `sh $step >$step.STDOUT 2> $step.STDERR`;
		         exit;
		    }
		}
	    }
	}

	# update state
	&draw_agenda_graph() unless $done;	
	
	# sleep until one more step is done
	while(! $done) {
	    sleep($SLEEP);
	    my $dir = &check_and_get("GENERAL:working-dir");
	    `ls $dir/steps > /dev/null`; # nfs bug
	    foreach my $i (keys %DO) {
		if (-e &versionize(&step_file($i)).".DONE") {
		    delete($DO{$i});
		    if (&check_if_crashed($i)) {
			$CRASHED{$i}++;
			print "step $DO_STEP[$i] crashed\n";
		    }
		    else {
			$DONE{$i}++;
		    }
		    $done++;
		    $active--;
		}
	    }
	    `touch $running_file`;
	}    
    }
}

# a number of arguments to the job submission may be specified
# note that this is specific to your gridengine implementation
# and some options may not work.

sub get_qsub_args {
    my ($step) = @_;
    my $qsub_args = &get("$step:qsub-settings");
    $qsub_args = &get("GENERAL:qsub-settings") unless defined($qsub_args);
    $qsub_args = "" unless defined($qsub_args);
    my $memory = &get("$step:qsub-memory");
    $qsub_args .= " -pe memory $memory" if defined($memory);
    my $hours = &get("$step:qsub-hours");
    $qsub_args .= " -l h_rt=$hours:0:0" if defined($hours);
    my $project = &backoff_and_get("$step:qsub-project");
    $qsub_args = "-P $project" if defined($project);
    print "qsub args: $qsub_args\n" if $VERBOSE;
    return $qsub_args;
}

# certain scripts when run on the clusters submit jobs
# themselves, hence they are executed regularly ("sh script")
# instead of submited as jobs. here we check for that.
sub is_qsub_script {
    my ($i) = @_;
    return (defined($QSUB_STEP{$i}) || 
	    defined($QSUB_SCRIPT{&defined_step($DO_STEP[$i])}));
}

# write the info file that is consulted to check if 
# a steps has to be redone, even if it was run before
sub write_info {
    my ($i) = @_;
    my $step = $DO_STEP[$i];
    my $module_set = $step; $module_set =~ s/:[^:]+$//;
    
    open(INFO,">".&versionize(&step_file($i)).".INFO") or die "Cannot open: $!";
    my %VALUE = &get_parameters_relevant_for_re_use($i);
    foreach my $parameter (keys %VALUE) {
	print INFO "$parameter = $VALUE{$parameter}\n";
    }

    # record re-use for recursive re-use
    foreach my $parent (@{$DEPENDENCY[$i]}) {
	my $p = $parent;
	while (defined($PASS{$p}) && scalar @{$DEPENDENCY[$p]}) {
	    $p = $DEPENDENCY[$p][0];
	}
	if ($RE_USE[$p]) {
	    print INFO "# reuse run $RE_USE[$p] for $DO_STEP[$p]\n";
	}
    }

    close(INFO);
}

# check the info file...
sub check_info {
    my ($i,$version) = @_;
    $version = $VERSION unless $version; # default: current version
    my %VALUE = &get_parameters_relevant_for_re_use($i);
    my ($module,$set,$step) = &deconstruct_name($DO_STEP[$i]);

    my %INFO;
    open(INFO,&versionize(&step_file($i),$version).".INFO") or die "Cannot open: $!";
    while(<INFO>) {
	chop;
	if (/ = /) {
	    my ($parameter,$value) = split(/ = /,$_,2);
	    $INFO{$parameter} = $value;
	}
	elsif (/^\# reuse run (\d+) for (\S+)/) {
	    if ($1>0 && defined($STEP_LOOKUP{$2})) {
		print "\tRECURSIVE_RE_USE{$i,$version,$2} = $1\n" if $VERBOSE;
		$RECURSIVE_RE_USE{$i,$version,$2} = $1;
	    }
	    else {
		print "\tnot using '$_', step $2 not required\n" if $VERBOSE;
		return 0;
	    }
	}
    }
    close(INFO);

    print "\tcheck parameter count current: ".(scalar keys %VALUE).", old: ".(scalar keys %INFO)."\n" if $VERBOSE;
    return 0 unless scalar keys %INFO == scalar keys %VALUE;
    foreach my $parameter (keys %VALUE) {
        if (! defined($INFO{$parameter})) {
          print "\told has no '$parameter' -> not re-usable\n" if $VERBOSE;
          return 0;
        }
	print "\tcheck '$VALUE{$parameter}' eq '$INFO{$parameter}' -> " if $VERBOSE;
        if (defined($ONLY_EXISTENCE_MATTERS{"$module:$step"}{$parameter})) {
            print "existence ok\n" if $VERBOSE;
        }
        elsif (&match_info_strings($VALUE{$parameter},$INFO{$parameter})) { 
            print "ok\n" if $VERBOSE; 
        }
        else { 
            print "mismatch\n" if $VERBOSE;
            return 0; 
        }
    }
    print "\tall parameters match\n" if $VERBOSE;
    return 1;
}

sub match_info_strings { 
  my ($current,$old) = @_;
  $current =~ s/ $//;
  $old =~ s/ $//;
  return 1 if $current eq $old;
  # ignore time stamps, if that option is used
  if (defined($IGNORE_TIME)) {
    $current =~ s/\[\d{10}\]//g;
    $old     =~ s/\[\d{10}\]//g;
  }
  return 1 if $current eq $old;
  # allowing stars to substitute numbers
  while($current =~ /^([^\*]+)\*(.*)$/) {
    return 0 unless $1 eq substr($old,0,length($1)); # prefix must match
    $current = $2;
    return 0 unless substr($old,length($1)) =~ /^\d+(.*)$/; # must start with number
    $old = $1;
    return 1 if $old eq $current; # done if rest matches
  }
  return 0;
}

sub get_parameters_relevant_for_re_use {
    my ($i) = @_;

    my %VALUE;
    my $step = $DO_STEP[$i];
    #my $module_set = $step; $module_set =~ s/:[^:]+$//;
    my ($module,$set,$dummy) = &deconstruct_name($step);
    foreach my $parameter (@{$RERUN_ON_CHANGE{&defined_step($step)}}) {
       #if ($parameter =~ /\//) {
       # TODO: handle scripts that need to be checked for time stamps
       #}
	my $value = &backoff_and_get_array(&extend_local_name($module,$set,$parameter));
        $value = join(" ",@{$value}) if ref($value) eq 'ARRAY';
	$VALUE{$parameter} = $value if $value;
    }

    my ($out,@INPUT) = &get_output_and_input($i);
    my $actually_used = "USED";
    foreach my $in_file (@INPUT) {
	$actually_used .= " ".$in_file; 
    }
    $VALUE{"INPUT"} = $actually_used;

    foreach my $in_file (@{$USES_INPUT{$i}}) {
	my $value = &backoff_and_get($in_file);
	$VALUE{$in_file} = $value if $value;
    }

    # add timestamp to files
    foreach my $value (values %VALUE) {
	if ($value =~ /^\//) { # file name
	    my $file = $value;
	    $file =~ s/ .+//; # ignore switches
            if (-e $file) {
	        my @filestat = stat($file);
	        $value .= " [".$filestat[9]."]";
	    }
	}
    }
#    foreach my $parameter (keys %VALUE) {
#	print "\t$parameter = $VALUE{$parameter}\n";
#    }
    return %VALUE;
}

sub check_if_crashed {
    my ($i,$version,$no_wait) = @_;
    $version = $VERSION unless $version; # default: current version
    my $file = &versionize(&step_file($i),$version).".STDERR";

    # while running, sometimes the STDERR file is slow in appearing - wait a bit just in case
    if ($version == $VERSION && !$no_wait) {
      my $j = 0;
      while (! -e $file && $j < 100) {
        sleep(5);
        $j++;
      }
    }

    #print "checking if $DO_STEP[$i]($version) crashed -> $file...\n";
    return 1 if ! -e $file;

    # check digest file (if it exists)
    if (-e $file.".digest") {
        my $error = 0;
	open(DIGEST,$file.".digest") or die "Cannot open: $!";
	while(<DIGEST>) {
	    print "\t$DO_STEP[$i]($version) crashed: $_" if $VERBOSE;
            $error++;
	}
	close(DIGEST);
	return $error;
    }

    # check against specified error patterns
    my @DIGEST;
    open(ERROR,$file) or die "Cannot open: $!";
    while(<ERROR>) {
	foreach my $pattern (@{$ERROR{&defined_step_id($i)}},
			     'error','killed','core dumped','can\'t read',
			     'no such file or directory','unknown option',
			     'died at','exit code','permission denied',
			     'segmentation fault','abort',
			     'no space left on device', ': not found',
			     'can\'t locate', 'unrecognized option', 'Exception') {
	    if (/$pattern/i) {
		my $not_error = 0;
		if (defined($NOT_ERROR{&defined_step_id($i)})) {
		    foreach my $override (@{$NOT_ERROR{&defined_step_id($i)}}) {
			$not_error++ if /$override/i;
		    }
		}
		if (!$not_error) {
		        push @DIGEST,$pattern;
			print "\t$DO_STEP[$i]($version) crashed: $pattern\n" if $VERBOSE;
		}
	    }
	}
        last if scalar(@DIGEST)>10
    }
    close(ERROR);

    # check if output file empty
    my $output = &get_default_file(&deconstruct_name($DO_STEP[$i]));
    # currently only works for single output file
    if (-e $output && -z $output) {
      push @DIGEST,"output file $output is empty";
    }

    # save digest file
    open(DIGEST,">$file.digest") or die "Cannot open: $!";
    foreach (@DIGEST) {
	print DIGEST $_."\n";
    }
    close(DIGEST);
    return scalar(@DIGEST);
}

# returns the name of the file where the step job is defined in
sub step_file {
    my ($i) = @_;
    my $step = $DO_STEP[$i];
    $step =~ s/:/_/g;
    my $dir = &check_and_get("GENERAL:working-dir");
    return "$dir/steps/$step";
}

sub step_file2 {
    my ($module,$set,$step) = @_;
    my $dir = &check_and_get("GENERAL:working-dir");
    `mkdir -p $dir/steps` if ! -e "$dir/steps";
    my $file = "$dir/steps/$module" . ($set ? ("_".$set) : "") . "_$step";    
    return $file;
}

sub versionize {
    my ($file,$version) = @_;
    $version = $VERSION unless $version;
    $file =~ s/steps\//steps\/$version\//;
    return $file.".".$version;
}

sub defined_step_id {
    my ($i) = @_;
    return &defined_step($DO_STEP[$i]);
}

sub defined_step {
    my ($step) = @_;
    my $defined_step = $step; 
    $defined_step =~ s/:.+:/:/;
    return $defined_step;
}

sub construct_name {
    my ($module,$set,$step) = @_;
    if (!defined($set) || $set eq "") {
	return "$module:$step";
    }
    return "$module:$set:$step";
}

sub deconstruct_name {
    my ($name) = @_;
    my ($module,$set,$step);
    if ($name !~ /:.+:/) {
        ($module,$step) = split(/:/,$name);
        $set = "";
    }
    else {
        ($module,$set,$step) = split(/:/,$name);
    }
#    print "deconstruct_name $name -> ($module,$set,$step)\n";
    return ($module,$set,$step);
}

sub deconstruct_local_name {
    my ($module,$set,$name) = @_;
    if ($name =~ /^(.+):(.+)$/) {
	$module = $1;
	$name = $2;
    }
    return ($module,$set,$name);
}

sub extend_local_name {
    my ($module,$set,$name) = @_;
    return &construct_name(&deconstruct_local_name($module,$set,$name));
}

### definition of steps

sub define_corpus_factorize {
    my ($step_id) = @_;
    my $scripts = &check_backoff_and_get("TUNING:moses-script-dir");

    my ($output,$input) = &get_output_and_input($step_id);
    my $input_extension = &check_backoff_and_get("TRAINING:input-extension");
    my $output_extension = &check_backoff_and_get("TRAINING:output-extension");
    
    my $dir = &check_and_get("GENERAL:working-dir");
    my $temp_dir = &check_and_get("INPUT-FACTOR:temp-dir") . ".$VERSION";
    my $cmd = "mkdir -p $temp_dir\n"
	. &factorize_one_language("INPUT-FACTOR",
				  "$input.$input_extension",
				  "$output.$input_extension",
				  &check_backoff_and_get_array("TRAINING:input-factors"),
				  $step_id)
	. &factorize_one_language("OUTPUT-FACTOR",
				  "$input.$output_extension",
				  "$output.$output_extension",
				  &check_backoff_and_get_array("TRAINING:output-factors"),
				  $step_id);
    
    &create_step($step_id,$cmd);
}

sub define_tuningevaluation_factorize {
    my ($step_id) = @_;
    my $scripts = &check_backoff_and_get("TUNING:moses-script-dir");

    my $dir = &check_and_get("GENERAL:working-dir");
    my ($output,$input) = &get_output_and_input($step_id);

    my $temp_dir = &check_and_get("INPUT-FACTOR:temp-dir") . ".$VERSION";
    my $cmd = "mkdir -p $temp_dir\n"
	. &factorize_one_language("INPUT-FACTOR",$input,$output,
				  &check_backoff_and_get_array("TRAINING:input-factors"),
				  $step_id);
    
    &create_step($step_id,$cmd);
}

sub define_lm_factorize {
    my ($step_id,$set) = @_;
    my $scripts = &check_backoff_and_get("TUNING:moses-script-dir");

    my ($output,$input) = &get_output_and_input($step_id);
    print "LM:$set:factors\n" if $VERBOSE;
    my $factor = &check_backoff_and_get_array("LM:$set:factors");
    
    my $dir = &check_and_get("GENERAL:working-dir");
    my $temp_dir = &check_and_get("INPUT-FACTOR:temp-dir") . ".$VERSION";
    my $cmd = "mkdir -p $temp_dir\n"
	. &factorize_one_language("OUTPUT-FACTOR",$input,$output,$factor,$step_id);
    
    &create_step($step_id,$cmd);
}

sub define_interpolated_lm_factorize_tuning {
    my ($step_id) = @_;
    my $scripts = &check_backoff_and_get("TUNING:moses-script-dir");

    my ($output,$input) = &get_output_and_input($step_id);
    my $factor = &check_backoff_and_get_array("TRAINING:output-factors");
    
    my $dir = &check_and_get("GENERAL:working-dir");
    my $temp_dir = &check_and_get("INPUT-FACTOR:temp-dir") . ".$VERSION";
    my $cmd = "mkdir -p $temp_dir\n"
	. &factorize_one_language("OUTPUT-FACTOR",$input,$output,$factor,$step_id);
    
    &create_step($step_id,$cmd);
}

sub define_splitter_train {
    my ($step_id,$set) = @_;

    my ($output,$input) = &get_output_and_input($step_id);
    my $input_splitter  = &get("GENERAL:input-splitter");
    my $output_splitter = &get("GENERAL:output-splitter");
    my $input_extension = &check_backoff_and_get("SPLITTER:input-extension");
    my $output_extension = &check_backoff_and_get("SPLITTER:output-extension");
    
    my $cmd = "";
    if ($input_splitter) {
	$cmd .= "$input_splitter -train -model $output.$input_extension -corpus $input.$input_extension\n";
    }
    if ($output_splitter) {
	$cmd .= "$output_splitter -train -model $output.$output_extension -corpus $input.$output_extension\n";
    }

    &create_step($step_id,$cmd);
}

sub define_lm_train_randomized {
    my ($step_id,$set) = @_;
    my $training = &check_backoff_and_get("LM:$set:rlm-training");
    my $order = &check_backoff_and_get("LM:$set:order"); 
    my ($output,$input) = &get_output_and_input($step_id);

    $output =~ /^(.+)\/([^\/]+)$/;
    my ($output_dir,$output_prefix) = ($1,$2);
    my $cmd = "gzip $input\n";
    $cmd .= "$training -struct BloomMap -order $order -output-prefix $output_prefix -output-dir $output_dir -input-type corpus -input-path $input\n";
    $cmd .= "gunzip $input\n";
    $cmd .= "mv $output.BloomMap $output\n";

    &create_step($step_id,$cmd);
}

sub define_lm_randomize {
    my ($step_id,$set_dummy) = @_;

    my ($module,$set,$stepname) = &deconstruct_name($DO_STEP[$step_id]);
    my $randomizer = &check_backoff_and_get("$module:$set:lm-randomizer");
    my $order = &check_backoff_and_get("$module:$set:order"); 
    my ($output,$input) = &get_output_and_input($step_id);

    $output =~ /^(.+)\/([^\/]+)$/;
    my ($output_dir,$output_prefix) = ($1,$2);
    my $cmd = "$randomizer -struct BloomMap -order $order -output-prefix $output_prefix -output-dir $output_dir -input-type arpa -input-path $input\n";
    $cmd .= "mv $output.BloomMap $output\n";

    &create_step($step_id,$cmd);
}

sub factorize_one_language {
    my ($type,$infile,$outfile,$FACTOR,$step_id) = @_;
    my $scripts = &check_backoff_and_get("TUNING:moses-script-dir");
    my $temp_dir = &check_and_get("INPUT-FACTOR:temp-dir") . ".$VERSION";
    my $parallelizer = &get("GENERAL:generic-parallelizer");
    my ($module,$set,$stepname) = &deconstruct_name($DO_STEP[$step_id]);
    
    my ($cmd,$list) = ("");
    foreach my $factor (@{$FACTOR}) {
	if ($factor eq "word") {
	    $list .= " $infile";
	}
	else {
	    my $script = &check_and_get("$type:$factor:factor-script");
	    my $out = "$outfile.$factor";
	    if ($parallelizer && defined($PARALLELIZE{&defined_step($DO_STEP[$step_id])}) 
		&& (  (&get("$module:jobs") && $CLUSTER)
		   || (&get("$module:cores") && $MULTICORE))) {
		my $subdir = $module;
		$subdir =~ tr/A-Z/a-z/;
		$subdir .= "/tmp.$set.$stepname.$type.$factor.$VERSION";
		if ($CLUSTER) {
		    my $qflags = "";
		    my $qsub_args = &get_qsub_args($DO_STEP[$step_id]);
		    $qflags="--queue-flags \"$qsub_args\"" if ($CLUSTER && $qsub_args);
		    $cmd .= "$parallelizer $qflags -in $infile -out $out -cmd '$script %s %s $temp_dir/$subdir' -jobs ".&get("$module:jobs")." -tmpdir $temp_dir/$subdir\n";
		    $QSUB_STEP{$step_id}++;
		}	
		elsif ($MULTICORE) {
		    $cmd .= "$parallelizer -in $infile -out $out -cmd '$script %s %s $temp_dir/$subdir' -cores ".&get("$module:cores")." -tmpdir $temp_dir/$subdir\n";
		}
	    }
	    else {
		$cmd .= "$script $infile $out $temp_dir\n";
	    }
	    $list .= " $out";
	}
    }
    return $cmd . "$scripts/training/combine_factors.pl $list > $outfile\n";
}

sub define_tuning_tune {
    my ($step_id) = @_;
    my $dir = &check_and_get("GENERAL:working-dir");
    my $hierarchical = &get("TRAINING:hierarchical-rule-set");
    my $tuning_script = &check_and_get("TUNING:tuning-script");
    my $use_mira = &backoff_and_get("TUNING:use-mira", 0);
    my $word_alignment = &backoff_and_get("TRAINING:include-word-alignment-in-rules");
    my $tmp_dir = &get_tmp_file("TUNING","","tune");
    
    # the last 3 variables are only used for mira tuning 
    my ($tuned_config,$config,$input,$reference,$config_devtest,$input_devtest,$reference_devtest, $filtered_config) = &get_output_and_input($step_id); 
    $config = $filtered_config if $filtered_config;


    my $cmd = "";
    if ($use_mira) {
	my $addTags = &backoff_and_get("TUNING:add-tags");
	my $use_jackknife = &backoff_and_get("TUNING:use-jackknife");
	if ($addTags && !$use_jackknife) {
	    my $input_with_tags = $input.".".$VERSION.".tags";
	    `$addTags < $input > $input_with_tags`;
	    $input = $input_with_tags;
	}

	my $addTagsDevtest = &backoff_and_get("TUNING:add-tags-devtest");
	if ($addTagsDevtest) {
	    my $input_devtest_with_tags = $input_devtest.".".$VERSION.".tags";
	    `$addTagsDevtest < $input_devtest > $input_devtest_with_tags`;
	    $input_devtest = $input_devtest_with_tags;
	}

	system("mkdir -p $tmp_dir");

	my $mira_config = "$tmp_dir/mira-config.$VERSION.";
	my $mira_config_log = $mira_config."log";
	$mira_config .= "cfg";
	
       	write_mira_config($mira_config,$tmp_dir,$config,$input,$reference,$config_devtest,$input_devtest,$reference_devtest);
	#$cmd = "$tuning_script -config $mira_config -exec >& $mira_config_log";
	# we want error messages in top-level log file
	$cmd = "$tuning_script -config $mira_config -exec ";

	# write script to select the best set of weights after training for the specified number of epochs --> 
	# cp to tuning/tmp.?/moses.ini
	my $script_filename = "$tmp_dir/selectBestWeights.";
	my $script_filename_log = $script_filename."log";
	$script_filename .= "perl";
	my $weight_output_file = "$tmp_dir/moses.ini";
	write_selectBestMiraWeights($tmp_dir, $script_filename, $weight_output_file);
	$cmd .= "\n$script_filename >& $script_filename_log";
    }
    else {
	my $scripts = &check_backoff_and_get("TUNING:moses-script-dir");
	my $nbest_size = &check_and_get("TUNING:nbest");
	my $lambda = &backoff_and_get("TUNING:lambda");
	my $tune_continue = &backoff_and_get("TUNING:continue");
	my $skip_decoder = &backoff_and_get("TUNING:skip-decoder");
	my $tune_inputtype = &backoff_and_get("TUNING:inputtype");
	my $jobs = &backoff_and_get("TUNING:jobs");
	my $decoder = &check_backoff_and_get("TUNING:decoder");

	my $decoder_settings = &backoff_and_get("TUNING:decoder-settings");
	$decoder_settings = "" unless $decoder_settings;
	$decoder_settings .= " -v 0 " unless $CLUSTER && $jobs;
	
	my $tuning_settings = &backoff_and_get("TUNING:tuning-settings");
	$tuning_settings = "" unless $tuning_settings;

	$cmd = "$tuning_script $input $reference $decoder $config --nbest $nbest_size --working-dir $tmp_dir --decoder-flags \"$decoder_settings\" --rootdir $scripts $tuning_settings --no-filter-phrase-table";
	$cmd .= " --lambdas \"$lambda\"" if $lambda;
	$cmd .= " --continue" if $tune_continue;
	$cmd .= " --skip-decoder" if $skip_decoder;
	$cmd .= " --inputtype $tune_inputtype" if defined($tune_inputtype);
    
	my $qsub_args = &get_qsub_args("TUNING");
	$cmd .= " --queue-flags=\"$qsub_args\"" if ($CLUSTER && $qsub_args);
	$cmd .= " --jobs $jobs" if $CLUSTER && $jobs;
	my $tuning_dir = $tuned_config;
	$tuning_dir =~ s/\/[^\/]+$//;
	$cmd .= "\nmkdir -p $tuning_dir";
    }
    
    $cmd .= "\ncp $tmp_dir/moses.ini $tuned_config";

    &create_step($step_id,$cmd);
}

sub write_mira_config {
    my ($config_filename,$expt_dir,$tune_filtered_ini,$input,$reference,$devtest_filtered_ini,$input_devtest,$reference_devtest) = @_;  
    my $moses_src_dir = &check_and_get("GENERAL:moses-src-dir");
    my $mira_src_dir = &backoff_and_get("GENERAL:mira-src-dir");
    my $tuning_decoder_settings = &check_and_get("TUNING:decoder-settings");
    my $start_weights = &backoff_and_get("TUNING:start-weight-config");
    my $tuning_settings = &check_and_get("TUNING:tuning-settings");

    my $parallel_settings = &backoff_and_get("TUNING:parallel-settings");
    my $use_jackknife = &backoff_and_get("TUNING:use-jackknife");

    # are we tuning a meta feature?
    my $tune_meta_feature = &backoff_and_get("TUNING:tune-meta-feature"); 

    my $tune_filtered_ini_start;
    if (!$use_jackknife) {
	$tune_filtered_ini =~ /.*\/([A-Za-z0-9\.\-\_]*)$/;
	$tune_filtered_ini_start = $1;
	$tune_filtered_ini_start =  $expt_dir."/".$tune_filtered_ini_start.".start";
	if ($start_weights) {
	    # apply start weights to filtered ini file, and pass the new ini to mira
	    print "DEBUG: $RealBin/support/substitute-weights.perl $start_weights $tune_filtered_ini $tune_filtered_ini_start \n";
	    system("$RealBin/support/substitute-weights.perl $start_weights $tune_filtered_ini $tune_filtered_ini_start");
	} 
    }

    # do we want to continue an interrupted experiment?
    my $continue_expt = &backoff_and_get("TUNING:continue-expt");
    my $continue_epoch = &backoff_and_get("TUNING:continue-epoch");
    my $continue_weights = &backoff_and_get("TUNING:continue-weights");    

    # mira config file
    open(CFG, ">$config_filename");
    print CFG "[general] \n";
    print CFG "name=expt \n";
    print CFG "fold=0 \n";
    print CFG "mpienv=openmpi_fillup_mark2 \n";
    if ($mira_src_dir) {
	print CFG "moses-home=".$mira_src_dir."\n";
    }
    else {
	print CFG "moses-home=".$moses_src_dir."\n";
    }
    print CFG "working-dir=".$expt_dir."\n";
    if ($continue_expt && $continue_expt > 0) {
	print CFG "continue-expt=".$continue_expt."\n";
	print CFG "continue-epoch=".$continue_epoch."\n";
	print CFG "continue-weights=".$continue_weights."\n";
    }
    print CFG "tune-meta-feature=1 \n" if ($tune_meta_feature);
    print CFG "jackknife=1 \n" if ($use_jackknife);
    print CFG "wait-for-bleu=1 \n\n";
    #print CFG "decoder-settings=".$tuning_decoder_settings."\n\n";   
    print CFG "[train] \n";
    print CFG "trainer=\${moses-home}/bin/mira \n";
    if ($use_jackknife) {
	print CFG "input-files-folds=";
	for my $i (0..9) {
	    my $addTags = &backoff_and_get("TUNING:add-tags");
	    if ($addTags) {
		my $input_with_tags = $input.".".$VERSION.".tags";
		`$addTags.only$i < $input.only$i > $input_with_tags.only$i`;

		print CFG $input_with_tags.".only$i, " if $i<9;
		print CFG $input_with_tags.".only$i" if $i==9;
	    }
	    else {
		print CFG $input.".only$i, " if $i<9;
		print CFG $input.".only$i" if $i==9;	    
	    }
	}
	print CFG "\n";
	print CFG "reference-files-folds=";
	for my $i (0..9) {
	    print CFG $reference.".only$i, " if $i<9;
	    print CFG $reference.".only$i" if $i==9;
	}
	print CFG "\n";
	print CFG "moses-ini-files-folds=";
	for my $i (0..9) {
	    print CFG $start_weights.".wo$i, " if $i<9;
	    print CFG $start_weights.".wo$i" if $i==9;
	}
	print CFG "\n";
    }
    else {
	print CFG "input-file=".$input."\n";
	print CFG "reference-files=".$reference."\n";
	if ($start_weights) {
	    print CFG "moses-ini-file=".$tune_filtered_ini_start."\n";
	}
	else {
	    print CFG "moses-ini-file=".$tune_filtered_ini."\n";
	}
    }
    print CFG "decoder-settings=".$tuning_decoder_settings." -text-type \"dev\"\n";    
    print CFG "hours=48 \n"; 
    if ($parallel_settings) {
	    foreach my $setting (split(" ", $parallel_settings)) {
	      print  CFG $setting."\n";
	    }
    }
    print CFG "extra-args=".$tuning_settings."\n\n";   
    print CFG "[devtest] \n";
    if (&get("TRAINING:hierarchical-rule-set")) {
	print CFG "moses=\${moses-home}/bin/moses_chart \n";
    }
    else {
	print CFG "moses=\${moses-home}/bin/moses \n";
    }
    # use multi-bleu to select the best set of weights
    print CFG "bleu=\${moses-home}/scripts/generic/multi-bleu.perl \n";
    print CFG "input-file=".$input_devtest."\n";
    print CFG "reference-file=".$reference_devtest."\n";
    print CFG "moses-ini-file=".$devtest_filtered_ini."\n";
    print CFG "decoder-settings=".$tuning_decoder_settings." -text-type \"devtest\"\n";   
    print CFG "hours=12 \nextra-args= \nskip-dev=1 \nskip-devtest=0 \nskip-submit=0 \n";
    close(CFG);
}

sub write_selectBestMiraWeights {
    my ($expt_dir, $script_filename, $weight_out_file) = @_;
    open(SCR, ">$script_filename");

    print SCR "#!/usr/bin/perl -w \nuse strict; \n\n";
    print SCR "my \@devtest_bleu = glob(\"$expt_dir/*_devtest.bleu\"); \# expt_00_0_devtest.bleu \n";
    print SCR "if (scalar(\@devtest_bleu) == 0) { \n";
    print SCR "\tprint STDERR \"ERROR: no bleu files globbed, cannot find best weights.\\n\"; \n";
    print SCR "\texit(1); \n";
    print SCR "} \n\n";
    print SCR "my (\$best_weights, \$best_id); \n";
    print SCR "my \$best_bleu = -1; \n";
    print SCR "my \$best_ratio = 0; \n";
    print SCR "foreach my \$bleu_file (\@devtest_bleu) { \n";
    print SCR "\t\$bleu_file =~ /_([\\d_]+)_devtest.bleu/; \n";
    print SCR "\tmy \$id = \$1; \n";
    print SCR "\topen(BLEU, \$bleu_file); \n";
    print SCR "\tmy \$bleu = <BLEU>; \n";
    print SCR "\t\$bleu =~ /BLEU = ([\\d\\.]+), .*ratio=([\\d\\.]+), /; \n";
    print SCR "\tif (\$1 > \$best_bleu || (\$1 == \$best_bleu && (abs(1-\$2) < abs(1-\$best_ratio)))) { \n";
    print SCR "\t\t\$best_bleu = \$1; \n";
    print SCR "\t\t\$best_ratio = \$2; \n";
    print SCR "\t\t# expt1-devtest.00_0.ini (incl. path to sparse weights) \n";
    print SCR "\t\t(\$best_weights) = glob(\"$expt_dir/*devtest.\$id.ini\"); \n";
    print SCR "\t} \n";
    print SCR "} \n\n";
    print SCR "print STDERR \"Best weights according to BLEU on devtest set: \$best_weights \\n\"; \n";
    print SCR "system(\"cp \$best_weights $weight_out_file\"); \n\n";
    
    close(SCR);
    system("chmod u+x $script_filename");
}

sub define_training_prepare_data_fast_align {
    my ($step_id) = @_;

    my ($prepared, $corpus) = &get_output_and_input($step_id);
    my $scripts = &check_and_get("GENERAL:moses-script-dir");
    my $input_extension  = &check_backoff_and_get("TRAINING:input-extension");
    my $output_extension = &check_backoff_and_get("TRAINING:output-extension");

    my $alignment_factors = "";
    if (&backoff_and_get("TRAINING:input-factors")) {
      my %IN = &get_factor_id("input");
      my %OUT = &get_factor_id("output");
      $alignment_factors = &encode_factor_definition("alignment-factors",\%IN,\%OUT);
    }
    my $cmd = "$scripts/ems/support/prepare-fast-align.perl $corpus.$input_extension $corpus.$output_extension $alignment_factors > $prepared";

    &create_step($step_id,$cmd);
}

sub define_training_prepare_data {
    my ($step_id) = @_;

    my ($prepared, $corpus) = &get_output_and_input($step_id);
    my $cmd = &get_training_setting(1);
    $cmd .= "-corpus $corpus ";
    $cmd .= "-corpus-dir $prepared ";

    &create_step($step_id,$cmd);
}

sub define_training_run_giza {
    my ($step_id) = @_;

    my ($giza, $prepared) = &get_output_and_input($step_id);
    my $cmd = &get_training_setting(2);
    $cmd .= "-corpus-dir $prepared ";
    $cmd .= "-giza-e2f $giza ";
    $cmd .= "-direction 2 ";

    &create_step($step_id,$cmd);
}

sub define_training_run_giza_inverse {
    my ($step_id) = @_;

    my ($giza, $prepared) = &get_output_and_input($step_id);
    my $cmd = &get_training_setting(2);
    $cmd .= "-corpus-dir $prepared ";
    $cmd .= "-giza-f2e $giza ";
    $cmd .= "-direction 1 ";

    &create_step($step_id,$cmd);
}

sub define_training_symmetrize_giza {
    my ($step_id) = @_;

    my ($aligned, $giza,$giza_inv) = &get_output_and_input($step_id);
    my $method = &check_and_get("TRAINING:alignment-symmetrization-method");
    my $cmd = &get_training_setting(3);
    
    $cmd .= "-giza-e2f $giza -giza-f2e $giza_inv ";
    $cmd .= "-alignment-file $aligned ";
    $cmd .= "-alignment-stem ".&versionize(&long_file_name("aligned","model",""))." ";
    $cmd .= "-alignment $method ";

    &create_step($step_id,$cmd);
}

sub define_training_build_suffix_array {
		my ($step_id) = @_;
	
		my $scripts = &check_and_get("GENERAL:moses-script-dir");
	
		my ($model, $aligned,$corpus) = &get_output_and_input($step_id);
		my $sa_exec_dir = &check_and_get("TRAINING:suffix-array");
		my $input_extension = &check_backoff_and_get("TRAINING:input-extension");
		my $output_extension = &check_backoff_and_get("TRAINING:output-extension");
		my $method = &check_and_get("TRAINING:alignment-symmetrization-method");
	
		my $glue_grammar_file = &versionize(&long_file_name("glue-grammar","model",""));
	
		my $cmd = "$scripts/training/wrappers/adam-suffix-array/suffix-array-create.sh $sa_exec_dir $corpus.$input_extension $corpus.$output_extension $aligned.$method $model $glue_grammar_file";

		&create_step($step_id,$cmd);
}

sub define_training_build_biconcor {
    my ($step_id) = @_;

    my ($model, $aligned,$corpus) = &get_output_and_input($step_id);
    my $biconcor = &check_and_get("TRAINING:biconcor");
    my $input_extension = &check_backoff_and_get("TRAINING:input-extension");
    my $output_extension = &check_backoff_and_get("TRAINING:output-extension");
    my $method = &check_and_get("TRAINING:alignment-symmetrization-method");

    my $cmd = "$biconcor -c $corpus.$input_extension -t $corpus.$output_extension -a $aligned.$method -s $model";
    &create_step($step_id,$cmd);
}

sub define_training_build_lex_trans {
    my ($step_id) = @_;

    my ($lex, $aligned,$corpus) = &get_output_and_input($step_id);
    my $baseline_alignment = &get("TRAINING:baseline-alignment");
    my $baseline_corpus = &get("TRAINING:baseline-corpus");

    my $cmd = &get_training_setting(4);
    $cmd .= "-lexical-file $lex ";
    $cmd .= "-alignment-file $aligned ";
    $cmd .= "-alignment-stem ".&versionize(&long_file_name("aligned","model",""))." ";
    $cmd .= "-corpus $corpus ";
    $cmd .= "-baseline-corpus $baseline_corpus " if defined($baseline_corpus) && defined($baseline_alignment);
    $cmd .= "-baseline-alignment $baseline_alignment " if defined($baseline_corpus) && defined($baseline_alignment);

    &create_step($step_id,$cmd);
}

sub define_training_build_transliteration_model {
    my ($step_id) = @_;

    my ($model, $corpus, $alignment) = &get_output_and_input($step_id);

    my $moses_script_dir = &check_and_get("GENERAL:moses-script-dir");
    my $input_extension = &check_backoff_and_get("TRAINING:input-extension");
    my $output_extension = &check_backoff_and_get("TRAINING:output-extension");
    my $sym_method = &check_and_get("TRAINING:alignment-symmetrization-method");
    my $moses_src_dir = &check_and_get("GENERAL:moses-src-dir");
    my $external_bin_dir = &check_and_get("GENERAL:external-bin-dir");
    my $srilm_dir = &check_and_get("TRAINING:srilm-dir");
    my $decoder = &get("TRAINING:transliteration-decoder");

    my $cmd = "$moses_script_dir/Transliteration/train-transliteration-module.pl";
    $cmd .= " --corpus-f $corpus.$input_extension";
    $cmd .= " --corpus-e $corpus.$output_extension";
    $cmd .= " --alignment $alignment.$sym_method";
    $cmd .= " --out-dir $model";
    $cmd .= " --moses-src-dir $moses_src_dir";
    $cmd .= " --decoder $decoder" if defined($decoder);
    $cmd .= " --external-bin-dir $external_bin_dir";
    $cmd .= " --srilm-dir $srilm_dir";
    $cmd .= " --input-extension $input_extension";
    $cmd .= " --output-extension $output_extension";
    $cmd .= " --factor 0-0";
    $cmd .= " --source-syntax " if &get("GENERAL:input-parser");
    $cmd .= " --target-syntax " if &get("GENERAL:output-parser");

    &create_step($step_id, $cmd);
}

sub define_training_extract_phrases {
    my ($step_id) = @_;

    my ($extract, $aligned,$corpus) = &get_output_and_input($step_id);
    my $cmd = &get_training_setting(5);
    $cmd .= "-alignment-file $aligned ";
    $cmd .= "-alignment-stem ".&versionize(&long_file_name("aligned","model",""))." ";
    $cmd .= "-extract-file $extract ";
    $cmd .= "-corpus $corpus ";
    
    if (&get("TRAINING:hierarchical-rule-set")) {
      my $glue_grammar_file = &get("TRAINING:glue-grammar");
      $glue_grammar_file = &versionize(&long_file_name("glue-grammar","model","")) 
        unless $glue_grammar_file;
      $cmd .= "-glue-grammar-file $glue_grammar_file ";

      if (&get("GENERAL:output-parser") && (&get("TRAINING:use-unknown-word-labels") || &get("TRAINING:use-unknown-word-soft-matches"))) {
        my $unknown_word_label = &versionize(&long_file_name("unknown-word-label","model",""));
        $cmd .= "-unknown-word-label $unknown_word_label ";
      }

      if (&get("GENERAL:output-parser") && &get("TRAINING:use-unknown-word-soft-matches")) {
          my $unknown_word_soft_matches = &versionize(&long_file_name("unknown-word-soft-matches","model",""));
          $cmd .= "-unknown-word-soft-matches $unknown_word_soft_matches ";
      }

      if (&get("TRAINING:use-ghkm")) {
        $cmd .= "-ghkm ";
      }

      if (&get("TRAINING:ghkm-tree-fragments")) {
        $cmd .= "-ghkm-tree-fragments ";
      }

      if (&get("TRAINING:ghkm-phrase-orientation")) {
        $cmd .= "-ghkm-phrase-orientation ";
        my $phrase_orientation_priors_file = &versionize(&long_file_name("phrase-orientation-priors","model",""));
        $cmd .= "-phrase-orientation-priors-file $phrase_orientation_priors_file ";
      }

      if (&get("TRAINING:ghkm-source-labels")) {
        $cmd .= "-ghkm-source-labels ";
      }
    }

    my $extract_settings = &get("TRAINING:extract-settings");
    $extract_settings .= " --IncludeSentenceId " if &get("TRAINING:domain-features");
    $cmd .= "-extract-options '".$extract_settings."' " if defined($extract_settings);

    my $baseline_extract = &get("TRAINING:baseline-extract");
    $cmd .= "-baseline-extract $baseline_extract" if defined($baseline_extract);

    &create_step($step_id,$cmd);
}

sub define_training_build_ttable {
    my ($step_id) = @_;

    my ($phrase_table, $extract,$lex,$domains) = &get_output_and_input($step_id);
    my $word_report = &backoff_and_get("EVALUATION:report-precision-by-coverage");
    my $word_alignment = &backoff_and_get("TRAINING:include-word-alignment-in-rules");

    my $cmd = &get_training_setting(6);
    $cmd .= "-extract-file $extract ";
    $cmd .= "-lexical-file $lex ";
    $cmd .= &get_table_name_settings("translation-factors","phrase-translation-table",$phrase_table);

    $cmd .=  "-no-word-alignment " if  defined($word_alignment) && $word_alignment eq "no";

    $cmd .= &define_domain_feature_score_option($domains) if &get("TRAINING:domain-features");

    if (&get("TRAINING:hierarchical-rule-set")) {
      if (&get("TRAINING:ghkm-tree-fragments")) {
        $cmd .= "-ghkm-tree-fragments ";
      }
      if (&get("TRAINING:ghkm-phrase-orientation")) {
        $cmd .= "-ghkm-phrase-orientation ";
        my $phrase_orientation_priors_file = &versionize(&long_file_name("phrase-orientation-priors","model",""));
        $cmd .= "-phrase-orientation-priors-file $phrase_orientation_priors_file ";
      }
      if (&get("TRAINING:ghkm-source-labels")) {
        $cmd .= "-ghkm-source-labels ";
        my $source_labels_file = &versionize(&long_file_name("source-labels","model",""));
        $cmd .= "-ghkm-source-labels-file $source_labels_file ";
      }
    }
    
    &create_step($step_id,$cmd);
}

sub define_domain_feature_score_option {
    my ($domains) = @_;
    my $spec = &backoff_and_get("TRAINING:domain-features");
    my ($method,$restricted_to_table) = ("","");
    $method = "Indicator" if $spec =~ /indicator/;
    $method = "Ratio" if $spec =~ /ratio/;
    $method = "Subset" if $spec =~ /subset/;
    $restricted_to_table = $1 if $spec =~ /( table \S+)/;
    die("ERROR: faulty TRAINING:domain-features spec (no method): $spec\n") unless defined($method);
    if ($spec =~ /sparse/) {
      return "-score-options '--SparseDomain$method $domains$restricted_to_table' ";
    }
    else {
      return "-score-options '--Domain$method $domains' ";
    }
}    

sub define_training_build_reordering {
    my ($step_id) = @_;

    my ($reordering_table, $extract) = &get_output_and_input($step_id);
    my $cmd = &get_training_setting(7);
    $cmd .= "-extract-file $extract ";
    $cmd .= &get_table_name_settings("reordering-factors","reordering-table",$reordering_table);

    &create_step($step_id,$cmd);
}

sub define_training_build_generation {
    my ($step_id) = @_;

    my ($generation_table, $corpus) = &get_output_and_input($step_id);
    my $cmd = &get_training_setting(8);
    $cmd .= "-corpus $corpus ";
    $cmd .= &get_table_name_settings("generation-factors","generation-table",$generation_table);

    &create_step($step_id,$cmd);
}

sub define_training_build_custom_generation {
    my ($step_id) = @_;

    my ($generation_table, $generation_corpus) = &get_output_and_input($step_id);
    my $cmd = &get_training_setting(8);
    $cmd .= "-generation-corpus $generation_corpus ";
    $cmd .= &get_table_name_settings("generation-factors","generation-table",$generation_table);

    &create_step($step_id,$cmd);
}

sub define_training_sigtest_filter {
    my ($step_id) = @_;
    my ($filtered_table, $raw_table,$suffix_array) = &get_output_and_input($step_id);

    my $hierarchical_flag = &get("TRAINING:hierarchical-rule-set") ? "-h" : "";
    my $sigtest_filter = &get("TRAINING:sigtest-filter");
    my $input_extension = &check_backoff_and_get("TRAINING:input-extension");
    my $output_extension = &check_backoff_and_get("TRAINING:output-extension");
    my $moses_src_dir = &check_and_get("GENERAL:moses-src-dir");

    if ($DO_STEP[$step_id] =~ /reordering/) {
      $raw_table = &get_table_name_settings("reordering-factors","reordering-table", $raw_table);
      $filtered_table = &get_table_name_settings("reordering-factors","reordering-table", $filtered_table);
      chop($raw_table);
      chop($filtered_table);
      $raw_table .= ".wbe-".&get("TRAINING:lexicalized-reordering"); # a bit of a hack
      $filtered_table .= ".wbe-".&get("TRAINING:lexicalized-reordering");
    }
    else {
      $raw_table = &get_table_name_settings("translation-factors","phrase-translation-table", $raw_table);
      $filtered_table = &get_table_name_settings("translation-factors","phrase-translation-table", $filtered_table);
      chop($raw_table);
      chop($filtered_table);
    }
    $raw_table =~ s/\s*\-\S+\s*//; # remove switch
    $filtered_table =~ s/\s*\-\S+\s*//; 
    
    my $cmd = "zcat $raw_table.gz | $moses_src_dir/contrib/sigtest-filter/filter-pt -e $suffix_array.$output_extension -f $suffix_array.$input_extension $sigtest_filter $hierarchical_flag | gzip - > $filtered_table.gz\n";
    &create_step($step_id,$cmd);
}

sub get_config_tables {
    my ($config,$reordering_table,$phrase_translation_table,$generation_table,$domains) = @_;

    my $moses_src_dir = &check_and_get("GENERAL:moses-src-dir");
    my $cmd = &get_training_setting(9);

    # get model, and whether suffix array is used. Determines the pt implementation.
    my $hierarchical = &get("TRAINING:hierarchical-rule-set");
    $cmd .= "-hierarchical " if $hierarchical;

    my $sa_exec_dir = &get("TRAINING:suffix-array");
    my ($ptImpl, $numFF) = (0);
    if ($hierarchical) {
      if ($sa_exec_dir) {
        $ptImpl = 10;  # suffix array
        $numFF = 7;
      }
      else {
        $ptImpl = 6; # in-mem SCFG
      }
    }

    # memory mapped suffix array phrase table
    my $mmsapt = &get("TRAINING:mmsapt");
    if (defined($mmsapt)) {
      $ptImpl = 11; # mmsapt
      $mmsapt =~ s/num-features=(\d+) // || die("ERROR: mmsapt setting needs to set num-features");
      $numFF = $1;
      $cmd .= "-mmsapt '$mmsapt' ";
    }

    # additional settings for factored models
    $cmd .= &get_table_name_settings("translation-factors","phrase-translation-table", $phrase_translation_table);
    $cmd = trim($cmd);
    $cmd .= ":$ptImpl" if $ptImpl>0;
    $cmd .= ":$numFF" if defined($numFF);
    $cmd .= " ";

    $cmd .= &get_table_name_settings("reordering-factors","reordering-table",$reordering_table) if $reordering_table;
    $cmd .= &get_table_name_settings("generation-factors","generation-table",$generation_table)	if $generation_table;
    $cmd .= "-config $config ";

    my $decoding_graph_backoff = &get("TRAINING:decoding-graph-backoff");
    if ($decoding_graph_backoff) {
      $cmd .= "-decoding-graph-backoff \"$decoding_graph_backoff\" ";
    }

    # additional settings for hierarchical models
    my $extract_version = $VERSION;
    if (&get("TRAINING:hierarchical-rule-set")) {
      $extract_version = $RE_USE[$STEP_LOOKUP{"TRAINING:extract-phrases"}] 
	  if defined($STEP_LOOKUP{"TRAINING:extract-phrases"});
      my $glue_grammar_file = &get("TRAINING:glue-grammar");
      $glue_grammar_file = &versionize(&long_file_name("glue-grammar","model",""),$extract_version) 
        unless $glue_grammar_file;
      $cmd .= "-glue-grammar-file $glue_grammar_file ";
    }

    # additional settings for syntax models
    if (&get("GENERAL:output-parser") && (&get("TRAINING:use-unknown-word-labels") || &get("TRAINING:use-unknown-word-soft-matches"))) {
	my $unknown_word_label = &versionize(&long_file_name("unknown-word-label","model",""),$extract_version);
	$cmd .= "-unknown-word-label $unknown_word_label ";
    }
    if (&get("GENERAL:output-parser") && &get("TRAINING:use-unknown-word-soft-matches")) {
        my $unknown_word_soft_matches = &versionize(&long_file_name("unknown-word-soft-matches","model",""),$extract_version);
        $cmd .= "-unknown-word-soft-matches $unknown_word_soft_matches ";
    }
    # configuration due to domain features
    $cmd .= &define_domain_feature_score_option($domains) if &get("TRAINING:domain-features");
    # additional specified items from config
    my $additional_ini = &get("TRAINING:additional-ini");
    $cmd .= "-additional-ini '$additional_ini' " if defined($additional_ini);

    return $cmd;
}

sub define_training_create_config {
    my ($step_id) = @_;

    my ($config,$reordering_table,$phrase_translation_table,$transliteration_pt,$generation_table,$sparse_lexical_features,$domains,$osm, @LM)
			= &get_output_and_input($step_id);

    my $cmd = &get_config_tables($config,$reordering_table,$phrase_translation_table,$generation_table,$domains);

    if($transliteration_pt){
	 $cmd .= "-transliteration-phrase-table $transliteration_pt ";
    }	

    if ($osm) {
      my $osm_settings = &get("TRAINING:operation-sequence-model-settings"); 
      if ($osm_settings =~ /-factor *(\S+)/){
        $cmd .= "-osm-model $osm/ -osm-setting $1 ";
      }
      else {
        $cmd .= "-osm-model $osm/operationLM.bin ";
      }
    }

    if (&get("TRAINING:ghkm-source-labels")) {
      $cmd .= "-ghkm-source-labels ";
      my $source_labels_file = &versionize(&long_file_name("source-labels","model",""));
      $cmd .= "-ghkm-source-labels-file $source_labels_file ";
    }

    # sparse lexical features provide additional content for config file
    $cmd .= "-additional-ini-file $sparse_lexical_features.ini " if $sparse_lexical_features;

    my @LM_SETS = &get_sets("LM");
    my %INTERPOLATED_AWAY;
    my %OUTPUT_FACTORS;
    %OUTPUT_FACTORS = &get_factor_id("output") if &backoff_and_get("TRAINING:output-factors");

    if (&get("INTERPOLATED-LM:script")) {
	my $type = 0;
	# binarizing the lm?
	$type = 1 if (&get("INTERPOLATED-LM:binlm") ||
		      &backoff_and_get("INTERPOLATED-LM:lm-binarizer"));
	# randomizing the lm?
	$type = 5 if (&get("INTERPOLATED-LM:rlm") ||
		      &backoff_and_get("INTERPOLATED-LM:lm-randomizer"));

  # manually set type 
  $type = &get("INTERPOLATED-LM:type") if &get("INTERPOLATED-LM:type");

  # go through each interpolated language model
  my ($icount,$ILM_SETS) = &get_interpolated_lm_sets();
  my $FACTOR = &backoff_and_get_array("TRAINING:output-factors");
  foreach my $factor (keys %{$ILM_SETS}) {
    foreach my $order (keys %{$$ILM_SETS{$factor}}) {
      next unless scalar(@{$$ILM_SETS{$factor}{$order}}) > 1;
      my $suffix = "";
      $suffix = ".$$FACTOR[$factor]" if $icount > 1 && defined($FACTOR);
      $suffix .= ".order$order" if $icount > 1;
	    $cmd .= "-lm $factor:$order:$LM[0]$suffix:$type ";
      foreach my $id_set (@{$$ILM_SETS{$factor}{$order}}) {
        my ($id,$set) = split(/ /,$id_set,2);
        $INTERPOLATED_AWAY{$set} = 1;
      }
    }
  }
  }
  shift @LM; # remove interpolated lm

	die("ERROR: number of defined LM sets (".(scalar @LM_SETS).":".join(",",@LM_SETS).") and LM files (".(scalar @LM).":".join(",",@LM).") does not match")
	    unless scalar @LM == scalar @LM_SETS;
	foreach my $lm (@LM) {
	    my $set = shift @LM_SETS;
      next if defined($INTERPOLATED_AWAY{$set});
	    my $order = &check_backoff_and_get("LM:$set:order");
	    my $lm_file = "$lm";
	    my $type = 0; # default: SRILM

	    # binarized language model?
	    $type = 1 if (&get("LM:$set:binlm") ||
			  &backoff_and_get("LM:$set:lm-binarizer"));

	    # using a randomized lm?
	    $type = 5 if (&get("LM:$set:rlm") ||
			  &backoff_and_get("LM:$set:rlm-training") ||
			  &backoff_and_get("LM:$set:lm-randomizer"));

      # manually set type 
      $type = &backoff_and_get("LM:$set:type") if (&backoff_and_get("LM:$set:type"));

	    # which factor is the model trained on?
	    my $factor = 0;
	    if (&backoff_and_get("TRAINING:output-factors") &&
		&backoff_and_get("LM:$set:factors")) {
		$factor = $OUTPUT_FACTORS{&backoff_and_get("LM:$set:factors")};
	    }

	    $cmd .= "-lm $factor:$order:$lm_file:$type ";
    }

    &create_step($step_id,$cmd);
}

sub define_interpolated_lm_interpolate {
    my ($step_id) = @_;

    my ($interpolated_lm,
	$interpolation_script, $tuning, @LM) = &get_output_and_input($step_id);
    my $srilm_dir = &check_backoff_and_get("INTERPOLATED-LM:srilm-dir");
    my $group = &get("INTERPOLATED-LM:group");
    my $weights = &get("INTERPOLATED-LM:weights");
    my $scripts = &check_backoff_and_get("TUNING:moses-script-dir");

    my $cmd = "";

    my %WEIGHT;
    if (defined($weights)) {
      foreach (split(/ *, */,$weights)) {
        /^ *(\S+) *= *(\S+)/ || die("ERROR: wrong interpolation weight specification $_ ($weights)");
        $WEIGHT{$1} = $2;
      }
    }

    # go through language models by factor and order 
    my ($icount,$ILM_SETS) = &get_interpolated_lm_sets();
    foreach my $factor (keys %{$ILM_SETS}) {
      foreach my $order (keys %{$$ILM_SETS{$factor}}) {
        next unless scalar(@{$$ILM_SETS{$factor}{$order}}) > 1;

        # get list of language model files
        my $lm_list = "";
        my $weight_list = "";
        foreach my $id_set (@{$$ILM_SETS{$factor}{$order}}) {
          my ($id,$set) = split(/ /,$id_set,2);
          $lm_list .= $LM[$id].",";
          if (defined($weights)) { 
            die("ERROR: no interpolation weight set for $factor:$order:$set (factor:order:set)") 
              unless defined($WEIGHT{"$factor:$order:$set"});
            $weight_list .= $WEIGHT{"$factor:$order:$set"}.",";
          }
        }
        chop($lm_list);
        chop($weight_list);

        # if grouping, identify position in list
        my $numbered_string = "";
        if (defined($group)) {
          my %POSITION;
          foreach my $id_set (@{$$ILM_SETS{$factor}{$order}}) {
            my ($id,$set) = split(/ /,$id_set,2);
            $POSITION{$set} = scalar keys %POSITION;
          }
          my $group_string = $group;
          $group_string =~ s/\s+/ /g;
          $group_string =~ s/ *, */,/g;
          $group_string =~ s/^ //;
          $group_string =~ s/ $//;
          $group_string .= " ";
          while($group_string =~ /^([^ ,]+)([ ,]+)(.*)$/) {
        #    die("ERROR: unknown set $1 in INTERPOLATED-LM:group definition")
        #  if ! defined($POSITION{$1});
# detect that elsewhere!
            if (defined($POSITION{$1})) {
              $numbered_string .= $POSITION{$1}.$2;
            }
            $group_string = $3;
          }
          chop($numbered_string);
        }

        my $FACTOR = &backoff_and_get_array("TRAINING:output-factors");
        my $name = $interpolated_lm;
        if ($icount > 1) {
          $name .= ".$$FACTOR[$factor]" if defined($FACTOR);
          $name .= ".order$order";
        }
        my $factored_tuning = $tuning;
        if (&backoff_and_get("TRAINING:output-factors")) {
          $factored_tuning = "$tuning.factor$factor";
          $cmd .= "$scripts/training/reduce-factors.perl --corpus $tuning --reduced $factored_tuning --factor $factor\n";
        }
        $cmd .= "$interpolation_script --tuning $factored_tuning --name $name --srilm $srilm_dir --lm $lm_list";
        $cmd .= " --group \"$numbered_string\"" if defined($group);
        $cmd .= " --weights \"$weight_list\"" if defined($weights);
        $cmd .= "\n";
      }
    }

    die("ERROR: Nothing to interpolate, remove interpolation step!") if $cmd eq "";
    &create_step($step_id,$cmd);
}

sub define_interpolated_lm_process {
  my ($step_id) = @_;

  my ($processed_lm, $interpolatd_lm) = &get_output_and_input($step_id);
  my ($module,$set,$stepname) = &deconstruct_name($DO_STEP[$step_id]);
  my $tool = &check_backoff_and_get("INTERPOLATED-LM:lm-${stepname}r");
  my $FACTOR = &backoff_and_get_array("TRAINING:output-factors");

  # go through language models by factor and order 
  my ($icount,$ILM_SETS) = &get_interpolated_lm_sets();
  my $cmd = "";
  foreach my $factor (keys %{$ILM_SETS}) {
    foreach my $order (keys %{$$ILM_SETS{$factor}}) {
      next unless scalar(@{$$ILM_SETS{$factor}{$order}}) > 1;
      my $suffix = "";
      $suffix = ".$$FACTOR[$factor]" if $icount > 1 && defined($FACTOR);
      $suffix .= ".order$order" if $icount > 1;
      $cmd .= "$tool $interpolatd_lm$suffix $processed_lm$suffix\n"; 
    }
  }

  &create_step($step_id,$cmd);
}

sub get_interpolated_lm_processed_names {
  my ($processed_lm) = @_;
  my @ILM_NAME;
  my ($icount,$ILM_SETS) = &get_interpolated_lm_sets();
  my $FACTOR = &backoff_and_get_array("TRAINING:output-factors");
  foreach my $factor (keys %{$ILM_SETS}) {
    foreach my $order (keys %{$$ILM_SETS{$factor}}) {
      if (scalar(@{$$ILM_SETS{$factor}{$order}}) > 1) {
        my $suffix = "";
        $suffix = ".$$FACTOR[$factor]" if $icount > 1 && defined($FACTOR);
        $suffix .= ".order$order" if $icount > 1;
        push @ILM_NAME,"$processed_lm$suffix";
      }
      else {
        push @ILM_NAME,"$processed_lm.".($FACTOR?"":".$$FACTOR[$factor]").".order$order";
      }
    }
  }
  return @ILM_NAME;
}

sub get_interpolated_lm_sets {
  my %ILM_SETS;

  my @LM_SETS = &get_sets("LM");
  my %OUTPUT_FACTORS;
  %OUTPUT_FACTORS = &get_factor_id("output") if &backoff_and_get("TRAINING:output-factors");

  my $count=0;
  my $icount=0;
  foreach my $set (@LM_SETS) {
    my $order = &check_backoff_and_get("LM:$set:order");
	
    my $factor = 0;
	  if (&backoff_and_get("TRAINING:output-factors") &&
	      &backoff_and_get("LM:$set:factors")) {
      $factor = $OUTPUT_FACTORS{&backoff_and_get("LM:$set:factors")};
    }

    push @{$ILM_SETS{$factor}{$order}}, ($count++)." ".$set;
    $icount++ if scalar(@{$ILM_SETS{$factor}{$order}}) == 2;
  }
  return ($icount,\%ILM_SETS);
}

sub get_training_setting {
    my ($step) = @_;
    my $dir = &check_and_get("GENERAL:working-dir");
    my $training_script = &check_and_get("TRAINING:script");
    my $external_bin_dir = &check_backoff_and_get("TRAINING:external-bin-dir");
    my $scripts = &check_backoff_and_get("TUNING:moses-script-dir");
    my $reordering = &get("TRAINING:lexicalized-reordering");
    my $input_extension = &check_backoff_and_get("TRAINING:input-extension");
    my $output_extension = &check_backoff_and_get("TRAINING:output-extension");
    my $alignment = &check_and_get("TRAINING:alignment-symmetrization-method");
    my $parts = &get("TRAINING:run-giza-in-parts");
    my $options = &get("TRAINING:training-options");
    my $phrase_length = &get("TRAINING:max-phrase-length");
    my $hierarchical = &get("TRAINING:hierarchical-rule-set");
    my $source_syntax = &get("GENERAL:input-parser");
    my $target_syntax = &get("GENERAL:output-parser");
    my $score_settings = &get("TRAINING:score-settings");
    my $parallel = &get("TRAINING:parallel");
    my $pcfg = &get("TRAINING:use-pcfg-feature");
    my $baseline_alignment = &get("TRAINING:baseline-alignment-model");

    my $xml = $source_syntax || $target_syntax;

    my $cmd = "$training_script ";
    $cmd .= "$options " if defined($options);
    $cmd .= "-dont-zip ";
    $cmd .= "-first-step $step " if $step>1;
    $cmd .= "-last-step $step "  if $step<9;
    $cmd .= "-external-bin-dir $external_bin_dir " if defined($external_bin_dir);
    $cmd .= "-f $input_extension -e $output_extension ";
    $cmd .= "-alignment $alignment ";
    $cmd .= "-max-phrase-length $phrase_length " if $phrase_length;
    $cmd .= "-parts $parts " if $parts;
    $cmd .= "-reordering $reordering " if $reordering;
    $cmd .= "-temp-dir /disk/scratch2 " if `hostname` =~ /townhill/;
    $cmd .= "-hierarchical " if $hierarchical;
    $cmd .= "-xml " if $xml;
    $cmd .= "-target-syntax " if $target_syntax;
    $cmd .= "-source-syntax " if $source_syntax;
    $cmd .= "-glue-grammar " if $hierarchical;
    $cmd .= "-score-options '".$score_settings."' " if $score_settings;
    $cmd .= "-parallel " if $parallel;
    $cmd .= "-pcfg " if $pcfg;
    $cmd .= "-baseline-alignment-model $baseline_alignment " if defined($baseline_alignment) && ($step == 1 || $step == 2);

    # factored training
    if (&backoff_and_get("TRAINING:input-factors")) {
	my %IN = &get_factor_id("input");
	my %OUT = &get_factor_id("output");
	$cmd .= "-input-factor-max ".((scalar keys %IN)-1)." ";
	$cmd .= "-alignment-factors ".
	    &encode_factor_definition("alignment-factors",\%IN,\%OUT)." ";
	$cmd .= "-translation-factors ".
	    &encode_factor_definition("translation-factors",\%IN,\%OUT)." ";
	$cmd .= "-reordering-factors ".
	    &encode_factor_definition("reordering-factors",\%IN,\%OUT)." "
	    if &get("TRAINING:reordering-factors");
	$cmd .= "-generation-factors ".
	    &encode_factor_definition("generation-factors",\%OUT,\%OUT)." "
	    if &get("TRAINING:generation-factors");
	die("ERROR: define either both TRAINING:reordering-factors and TRAINING:reordering or neither")
	    if ((  &get("TRAINING:reordering-factors") && ! $reordering) ||
		(! &get("TRAINING:reordering-factors") &&   $reordering));
	my $decoding_steps = &check_and_get("TRAINING:decoding-steps");
	$decoding_steps =~ s/\s*//g;
	$cmd .= "-decoding-steps $decoding_steps ";
	my $generation_type = &get("TRAINING:generation-type");
	$cmd .= "-generation-type $generation_type " if $generation_type;
    }

    return $cmd;
}

sub get_table_name_settings {
    my ($factor,$table,$default) = @_;
    my $dir = &check_and_get("GENERAL:working-dir");

    my @NAME;
    if (!&backoff_and_get("TRAINING:input-factors")) {
	return "-$table $default ";
    }

    # define default names
    my %IN = &get_factor_id("input");
    my %OUT = &get_factor_id("output");
    %IN = %OUT if $factor eq "generation-factors";
    my $factors = &encode_factor_definition($factor,\%IN,\%OUT);
    foreach my $f (split(/\+/,$factors)) {
	push @NAME,"$default.$f";
#	push @NAME,"$dir/model/$table.$VERSION.$f";
    }
    
    # get specified names, if any
    if (&get("TRAINING:$table")) {
	my @SPECIFIED_NAME = @{$CONFIG{"TRAINING:$table"}};
	die("ERROR: specified more ${table}s than $factor")
	    if (scalar @SPECIFIED_NAME) > (scalar @NAME);
	for(my $i=0;$i<scalar(@SPECIFIED_NAME);$i++) {
	    $NAME[$i] = $SPECIFIED_NAME[$i];
	}
    }

    # create command
    my $cmd;
    foreach my $name (@NAME) {
	$cmd .= "-$table $name ";
    }
    return $cmd;
} 

sub get_factor_id {
    my ($type) = @_;
    my $FACTOR = &check_backoff_and_get_array("TRAINING:$type-factors");
    my %ID = ();
    foreach my $factor (@{$FACTOR}) {
	$ID{$factor} = scalar keys %ID;
    }
    return %ID;
}

sub encode_factor_definition {
    my ($parameter,$IN,$OUT) = @_;
    my $definition = &check_and_get("TRAINING:$parameter");
    my $encoded;
    foreach my $mapping (split(/,\s*/,$definition)) {
	my ($in,$out) = split(/\s*->\s*/,$mapping);
	$encoded .= 
	    &encode_factor_list($IN,$in)."-".
	    &encode_factor_list($OUT,$out)."+";
    }
    chop($encoded);
    return $encoded;
}

sub encode_factor_list {
    my ($ID,$list) = @_;
    my $id;
    foreach my $factor (split(/\s*\+\s*/,$list)) {
	die("ERROR: unknown factor type '$factor'\n") unless defined($$ID{$factor});
	$id .= $$ID{$factor}.",";
    }
    chop($id);
    return $id;
}

sub define_tuningevaluation_filter {
    my ($set,$step_id) = @_;
    my $scripts = &check_and_get("GENERAL:moses-script-dir");
    my $dir = &check_and_get("GENERAL:working-dir");
    my $word_alignment = &backoff_and_get("TRAINING:include-word-alignment-in-rules");
    my $tuning_flag = !defined($set);
    my $hierarchical = &get("TRAINING:hierarchical-rule-set");

    my ($filter_dir,$input,$phrase_translation_table,$reordering_table,$domains,$transliteration_table) = &get_output_and_input($step_id);

    my $binarizer;
    $binarizer = &backoff_and_get("EVALUATION:$set:ttable-binarizer") unless $tuning_flag;
    $binarizer = &backoff_and_get("TUNING:ttable-binarizer") if $tuning_flag;
    my $report_precision_by_coverage = !$tuning_flag && &backoff_and_get("EVALUATION:$set:report-precision-by-coverage");
    
    # occasionally, lattices and conf nets need to be able 
    # to filter phrase tables, we can provide sentences/ngrams 
    # in a separate file
    my $input_filter;
    $input_filter = &get("EVALUATION:$set:input-filter") unless $tuning_flag;
    $input_filter = &get("TUNING:input-filter") if $tuning_flag;
    #print "filter: $input_filter \n";
    $input_filter = $input unless $input_filter;
    
    my $settings = &backoff_and_get("EVALUATION:$set:filter-settings") unless $tuning_flag;
    $settings = &backoff_and_get("TUNING:filter-settings") if $tuning_flag;
    $settings = "" unless $settings;

    $binarizer .= " -no-alignment-info" if defined ($binarizer) && !$hierarchical && defined $word_alignment && $word_alignment eq "no";
        
    $settings .= " -Binarizer \"$binarizer\"" if $binarizer;
    $settings .= " --Hierarchical" if $hierarchical;

    # get model, and whether suffix array is used. Determines the pt implementation.
    my $sa_exec_dir = &get("TRAINING:suffix-array");
    my $sa_extractors = &get("GENERAL:sa_extractors");
    $sa_extractors = 1 unless $sa_extractors;

    my ($ptImpl, $numFF);
    if ($hierarchical) {
	if ($sa_exec_dir) {
	    $ptImpl = 10;  # suffix array
	    $numFF = 7;
	}
	else {
	    $ptImpl = 6; # in-mem SCFG
	}
    }
    else {
    	$ptImpl = 0; # phrase-based
    }

    # config file specified?
    my ($config,$cmd,$delete_config);
    if (&get("TUNING:config-with-reused-weights")) {
      $config = &get("TUNING:config-with-reused-weights");
    }
    elsif (&get("TRAINING:config")) {
      $config = &get("TRAINING:config");
    }
    # create pseudo-config file
    else {
      $config = $tuning_flag ? "$dir/tuning/moses.table.ini.$VERSION" : "$dir/evaluation/$set.moses.table.ini.$VERSION";
      $cmd = "touch $config\n";
      $delete_config = 1;
      
      $cmd .= &get_config_tables($config,$reordering_table,$phrase_translation_table,undef,$domains);

	if (&get("TRAINING:in-decoding-transliteration")) {

		$cmd .= "-transliteration-phrase-table $dir/model/transliteration-phrase-table.$VERSION ";
	}	


      $cmd .= "-lm 0:3:$config:8\n"; # dummy kenlm 3-gram model on factor 0

    }

    # filter command
    if ($sa_exec_dir) {
	# suffix array
	$cmd .= "$scripts/training/wrappers/adam-suffix-array/suffix-array-extract.sh $sa_exec_dir $phrase_translation_table $input_filter $filter_dir $sa_extractors \n";
	
	my $escaped_filter_dir = $filter_dir;
	$escaped_filter_dir =~ s/\//\\\\\//g;
	$cmd .= "cat $config | sed s/10\\ 0\\ 0\\ 7.*/10\\ 0\\ 0\\ 7\\ $escaped_filter_dir/g > $filter_dir/moses.ini \n";
    # kind of a hack -- the correct thing would be to make the generation of the config file ($filter_dir/moses.ini) 
    # set the PhraseDictionaryALSuffixArray's path to the filtered directory rather than to the suffix array itself 
    $cmd .= "sed -i 's%path=$phrase_translation_table%path=$filter_dir%' $filter_dir/moses.ini\n";
    }
    else {
	# normal phrase table
	$cmd .= "$scripts/training/filter-model-given-input.pl";
	$cmd .= " $filter_dir $config $input_filter $settings\n";
    }
    
    # clean-up
    $cmd .= "rm $config" if $delete_config;

    &create_step($step_id,$cmd);
}

sub define_evaluation_decode {
    my ($set,$step_id) = @_;
    my $scripts = &check_and_get("GENERAL:moses-script-dir");
    my $dir = &check_and_get("GENERAL:working-dir");
    
    my ($system_output,
	$config,$input,$filtered_config) = &get_output_and_input($step_id);
    $config = $filtered_config if $filtered_config;

    my $jobs = &backoff_and_get("EVALUATION:$set:jobs");
    my $decoder = &check_backoff_and_get("EVALUATION:$set:decoder");
    my $settings = &backoff_and_get("EVALUATION:$set:decoder-settings");
    $settings = "" unless $settings;
    my $nbest = &backoff_and_get("EVALUATION:$set:nbest");
    my $moses_parallel = &backoff_and_get("EVALUATION:$set:moses-parallel");
    my $report_segmentation = &backoff_and_get("EVALUATION:$set:report-segmentation");
    my $analyze_search_graph = &backoff_and_get("EVALUATION:$set:analyze-search-graph");
    my $report_precision_by_coverage = &backoff_and_get("EVALUATION:$set:report-precision-by-coverage");
    my $use_wade = &backoff_and_get("EVALUATION:$set:wade");
    my $hierarchical = &get("TRAINING:hierarchical-rule-set");
    my $word_alignment = &backoff_and_get("TRAINING:include-word-alignment-in-rules");
    my $post_decoding_transliteration = &get("TRAINING:post-decoding-transliteration");
		
   # If Transliteration Module is to be used as post-decoding step ...	
   if (defined($post_decoding_transliteration) && $post_decoding_transliteration eq "yes"){
	$settings .= " -output-unknowns $system_output.oov";
   }
  	

    # specify additional output for analysis
    if (defined($report_precision_by_coverage) && $report_precision_by_coverage eq "yes") {
      $settings .= " -alignment-output-file $system_output.wa";
      $report_segmentation = "yes";
    }
    if (defined($analyze_search_graph) && $analyze_search_graph eq "yes") {
      $settings .= " -unpruned-search-graph -include-lhs-in-search-graph -osg $system_output.graph";
    }
    if (defined($report_segmentation) && $report_segmentation eq "yes") {
      if ($hierarchical) {
        $settings .= " -T $system_output.trace";
      }
      else {
        $settings .= " -t";
      }
    }
    if ($use_wade) {
      $settings .= " -T $system_output.details";
    }
    $settings .= " -text-type \"test\"";

    my $addTags = &backoff_and_get("EVALUATION:$set:add-tags");
    if ($addTags) {
	my $input_with_tags = $input.".".$VERSION.".tags";
	`$addTags < $input > $input_with_tags`;
	$input = $input_with_tags;
    }

    # create command
    my $cmd;
    my $nbest_size;
    $nbest_size = $nbest if $nbest;
    $nbest_size =~ s/[^\d]//g if $nbest;
    if ($jobs && $CLUSTER) {
	$cmd .= "mkdir -p $dir/evaluation/tmp.$set.$VERSION\n";
	$cmd .= "cd $dir/evaluation/tmp.$set.$VERSION\n";
	if (defined $moses_parallel) {
	    $cmd .= $moses_parallel;                
	} else {
	    $cmd .= "$scripts/generic/moses-parallel.pl";
	}
	my $qsub_args = &get_qsub_args($DO_STEP[$step_id]);
	$cmd .= " -queue-parameters \"$qsub_args\"" if ($CLUSTER && $qsub_args);
	$cmd .= " -decoder $decoder";
	$cmd .= " -config $config";
	$cmd .= " -input-file $input";
	$cmd .= " --jobs $jobs";
	$cmd .= " -decoder-parameters \"$settings\" > $system_output";	
	$cmd .= " -n-best-file $system_output.best$nbest_size -n-best-size $nbest" if $nbest;
    }
    else {
	$cmd = "$decoder $settings -v 0 -f $config < $input > $system_output";
	$cmd .= " -n-best-list $system_output.best$nbest_size $nbest" if $nbest;
    }

    &create_step($step_id,$cmd);
}

sub define_evaluation_analysis {
    my ($set,$step_id) = @_;

    my ($analysis,
	$output,$reference,$input) = &get_output_and_input($step_id);
    my $script = &backoff_and_get("EVALUATION:$set:analysis");
    my $report_segmentation = &backoff_and_get("EVALUATION:$set:report-segmentation");
    my $analyze_search_graph = &backoff_and_get("EVALUATION:$set:analyze-search-graph");

    my $cmd = "$script -system $output -reference $reference -input $input -dir $analysis";
    if (defined($report_segmentation) && $report_segmentation eq "yes") {
        my $segmentation_file = &get_default_file("EVALUATION",$set,"decode");
	$cmd .= " -segmentation $segmentation_file";
    }
    if (defined($analyze_search_graph) && $analyze_search_graph eq "yes") {
      my $search_graph_file = &get_default_file("EVALUATION",$set,"decode");
      $cmd .= " -search-graph $search_graph_file.graph";
    }
    if (&get("TRAINING:hierarchical-rule-set")) {
	$cmd .= " -hierarchical";
    }
    &create_step($step_id,$cmd);
}

sub define_evaluation_analysis_precision {
    my ($set,$step_id) = @_;

    my ($analysis,
	$output,$reference,$input,$corpus,$ttable,$coverage) = &get_output_and_input($step_id);
    my $script = &backoff_and_get("EVALUATION:$set:analysis");
    my $input_extension = &check_backoff_and_get("TRAINING:input-extension");
    my $coverage_base = &backoff_and_get("EVALUATION:$set:precision-by-coverage-base");
    my $cmd = "$script -system $output -reference $reference -input $input -dir $analysis -precision-by-coverage";

    my $segmentation_file = &get_default_file("EVALUATION",$set,"decode");
    $cmd .= " -segmentation $segmentation_file";
    $cmd .= " -system-alignment $segmentation_file.wa";
    $coverage = $coverage_base if defined($coverage_base);
    $cmd .= " -coverage $coverage";

    # get table with surface factors
    if (&backoff_and_get("TRAINING:input-factors")) {
      my %IN = &get_factor_id("input");
      my %OUT = &get_factor_id("output");
      my $factors = &encode_factor_definition("translation-factors",\%IN,\%OUT);
      my @FACTOR = split(/\+/,$factors);
      my @SPECIFIED_NAME;
      if (&backoff_and_get("TRAINING:sigtest-filter-phrase-translation-table")) {
        @SPECIFIED_NAME = @{$CONFIG{"TRAINING:sigtest-filter-phrase-translation-table"}};
      }
      elsif (&backoff_and_get("TRAINING:phrase-translation-table")) {
        @SPECIFIED_NAME = @{$CONFIG{"TRAINING:phrase-translation-table"}};
      }
      for(my $i=0;$i<scalar(split(/\+/,$factors));$i++) {
        if ($FACTOR[$i] =~ /^0-/) {
	  if (scalar(@SPECIFIED_NAME) > $i) {
            $ttable = $SPECIFIED_NAME[$i];
	  }
	  else {
	    $ttable .= ".".$FACTOR[$i];
	  }
	  last;
        }
      }
      my $subreport = &backoff_and_get("EVALUATION:precision-by-coverage-factor");
      if (defined($subreport)) {
        die("unknown factor $subreport specified in EVALUATION:precision-by-coverage-factor") unless defined($IN{$subreport});
        $cmd .= " -precision-by-coverage-factor ".$IN{$subreport};
      }
    }
    $cmd .= " -ttable $ttable -input-corpus $corpus.$input_extension";

    &create_step($step_id,$cmd);
}

sub define_evaluation_analysis_coverage {
    my ($set,$step_id) = @_;

    my ($analysis,
	$input,$corpus,$ttable) = &get_output_and_input($step_id);
    my $script = &backoff_and_get("EVALUATION:$set:analysis");
    my $input_extension = &check_backoff_and_get("TRAINING:input-extension");
    my $score_settings = &get("TRAINING:score-settings");

    my $ttable_config;

    # translation tables for un-factored
    if (!&backoff_and_get("TRAINING:input-factors")) {
      $ttable_config = "-ttable $ttable";
    }
    # translation tables for factored
    else {
      my %IN = &get_factor_id("input");
      $ttable_config = "-input-factors ".(scalar(keys %IN));
      my %OUT = &get_factor_id("output");
      $ttable_config .= " -input-factor-names '".join(",",keys %IN)."'";
      $ttable_config .= " -output-factor-names '".join(",",keys %OUT)."'";
      my $factors = &encode_factor_definition("translation-factors",\%IN,\%OUT);
      my @FACTOR = split(/\+/,$factors);
      my @SPECIFIED_NAME;
      if (&backoff_and_get("TRAINING:sigtest-filter-phrase-translation-table")) {
        @SPECIFIED_NAME = @{$CONFIG{"TRAINING:sigtest-filter-phrase-translation-table"}};
      }
      elsif (&backoff_and_get("TRAINING:phrase-translation-table")) {
        @SPECIFIED_NAME = @{$CONFIG{"TRAINING:phrase-translation-table"}};
      }
      my $surface_ttable;
      for(my $i=0;$i<scalar(@FACTOR);$i++) {
	$FACTOR[$i] =~ /^([\d\,]+)/;
	my $input_factors = $1;

	my $ttable_name = $ttable.".".$FACTOR[$i];
	if (scalar(@SPECIFIED_NAME) > $i) {
	  $ttable_name = $SPECIFIED_NAME[$i];
	}

	$ttable_config .= " -factored-ttable $input_factors:".$ttable_name;
	if ($input_factors eq "0" && !defined($surface_ttable)) {
	    $surface_ttable = $ttable_name;
	    $ttable_config .= " -ttable $surface_ttable";
	}
      }
    }

    my $cmd = "$script -input $input -input-corpus $corpus.$input_extension $ttable_config -dir $analysis";
    $cmd .= " -score-options '$score_settings'" if $score_settings;
    &create_step($step_id,$cmd);
}

sub define_reporting_report {
    my ($step_id) = @_;

    my $score_file = &get_default_file("REPORTING","","report");

    my $scripts = &check_and_get("GENERAL:moses-script-dir");
    my $cmd = "$scripts/ems/support/report-experiment-scores.perl";
    
    # get scores that were produced
    foreach my $parent (@{$DEPENDENCY[$step_id]}) {
	my ($parent_module,$parent_set,$parent_step) 
	    = &deconstruct_name($DO_STEP[$parent]);
	
	my $file = &get_default_file($parent_module,$parent_set,$parent_step);
	$cmd .= " set=$parent_set,type=$parent_step,file=$file";
    }

    # maybe send email
    my $email = &get("REPORTING:email");
    if ($email) {
	$cmd .= " email='$email'";
    }

    $cmd .= " >$score_file";

    &create_step($step_id,$cmd);
}

### subs for step definition

sub get_output_and_input {
  my ($step_id) = @_;

  my $step = $DO_STEP[$step_id];
  my $output = &get_default_file(&deconstruct_name($step));

  my @INPUT;
  if (defined($USES_INPUT{$step_id})) { 
    for(my $i=0; $i<scalar @{$USES_INPUT{$step_id}}; $i++) {
	# get name of input file needed
	my $in_file = $USES_INPUT{$step_id}[$i];

	# if not directly specified, find step that produces this file.
	# note that if the previous step is passed than the grandparent's
	# outfile is used (done by &get_specified_or_default_file)
	my $prev_step = "";
#	print "\tlooking up in_file $in_file\n";
	foreach my $parent (@{$DEPENDENCY[$step_id]}) {
	    my ($parent_module,$parent_set,$parent_step) 
		= &deconstruct_name($DO_STEP[$parent]);
	    my $parent_file 
		= &construct_name($parent_module,$parent_set,
				  $STEP_OUT{&defined_step($DO_STEP[$parent])});
	    if ($in_file eq $parent_file) {
		$prev_step = $DO_STEP[$parent];
	    }
	}
#	print "\t\tfrom previous step: $prev_step ($in_file)\n";
	if ($prev_step eq "" && !defined($CONFIG{$in_file})) {
            # undefined (ignored previous step)
#	    print "ignored previous step to generate $USES_INPUT{$step_id}[$i]\n";
	    push @INPUT,"";
	    next;
	}

	# get the actual file name
	push @INPUT,&get_specified_or_default_file(&deconstruct_name($in_file),
						   &deconstruct_name($prev_step));
    }
  }
  return ($output,@INPUT);
}

sub define_template {
    my ($step_id) = @_;

    my $step = $DO_STEP[$step_id];
    print "building sh file for $step\n" if $VERBOSE;
    my $defined_step = &defined_step($step);
    return 0 unless (defined($TEMPLATE   {$defined_step}) ||
		     defined($TEMPLATE_IF{$defined_step}));

    my $parallelizer = &get("GENERAL:generic-parallelizer");
    my $dir = &check_and_get("GENERAL:working-dir");

    my ($module,$set,$stepname) = &deconstruct_name($step);

    my $multiref = undef;
    if ($MULTIREF{$defined_step} &&  # step needs to be run differently if multiple ref
        &backoff_and_get(&extend_local_name($module,$set,"multiref"))) { # there are multiple ref
      $multiref = $MULTIREF{$defined_step};
    }

    my ($output,@INPUT) = &get_output_and_input($step_id);

    my $cmd;
    if (defined($TEMPLATE{$defined_step})) {
	$cmd = $TEMPLATE{$defined_step};
    }
    else {
	foreach my $template_if (@{$TEMPLATE_IF{$defined_step}}) {
	    my ($command,$in,$out,@EXTRA) = @{$template_if};
	    my $extra = join(" ",@EXTRA);

	    if (&backoff_and_get(&extend_local_name($module,$set,$command))) {
		    $cmd .= "\$$command < $in > $out $extra\n";
	    }
	    else {
		$cmd .= "ln -s $in $out\n";
	    }
	}
    }

    if ($parallelizer && defined($PARALLELIZE{$defined_step}) &&
	((&get("$module:jobs")  && $CLUSTER)   ||
	 (&get("$module:cores") && $MULTICORE))) {
	my $new_cmd;
	my $i=0;
	foreach my $single_cmd (split(/\n/,$cmd)) {
	    if ($single_cmd =~ /^ln /) {
		$new_cmd .= $single_cmd."\n";
	    }
	    elsif ($single_cmd =~ /^.+$/) {		
		# find IN and OUT files
		$single_cmd =~ /(EMS_IN_EMS\S*)/
		  || die("ERROR: could not find EMS_IN_EMS in $single_cmd");
		my $in = $1;
		$single_cmd =~ /(EMS_OUT_EMS\S*)/ 
		    || die("ERROR: could not find OUT in $single_cmd");
		my $out = $1;
		#  replace IN and OUT with %s
		$single_cmd =~ s/EMS_IN_EMS\S*/\%s/;
		$single_cmd =~ s/EMS_OUT_EMS\S*/\%s/;
		$single_cmd =~ s/EMS_SLASH_OUT_EMS\S*/\%s/;
		# build tmp
		my $tmp_dir = $module;
		$tmp_dir =~ tr/A-Z/a-z/;
		$tmp_dir .= "/tmp.$set.$stepname.$VERSION-".($i++);
		if ($CLUSTER) {
		    my $qflags = "";
		    my $qsub_args = &get_qsub_args($DO_STEP[$step_id]);
		    $qflags="--queue-flags \"$qsub_args\"" if ($CLUSTER && $qsub_args);
		    $new_cmd .= "$parallelizer $qflags -in $in -out $out -cmd '$single_cmd' -jobs ".&get("$module:jobs")." -tmpdir $dir/$tmp_dir\n";
		}	
		if ($MULTICORE) {
		    $new_cmd .= "$parallelizer -in $in -out $out -cmd '$single_cmd' -cores ".&get("$module:cores")." -tmpdir $dir/$tmp_dir\n";
		}
	    }
	}
	
	$cmd = $new_cmd;
	$QSUB_STEP{$step_id}++;
    }

    # command to be run on multiple reference translations
    if (defined($multiref)) {
      $cmd =~ s/^(.*)EMS_IN_EMS (.+)EMS_OUT_EMS(.*)$/$multiref '$1 mref-input-file $2 mref-output-file $3' EMS_IN_EMS EMS_OUT_EMS/;
      $cmd =~ s/^(.+)EMS_OUT_EMS(.+)EMS_IN_EMS (.*)$/$multiref '$1 mref-output-file $2 mref-input-file $3' EMS_IN_EMS EMS_OUT_EMS/;
    }

    # input is array, but just specified as IN
    if ($cmd !~ /EMS_IN1_EMS/ && (scalar @INPUT) > 1 ) {
	my $in = join(" ",@INPUT);
	$cmd =~ s/EMS_IN_EMS/$in/;
    }
    # input is defined as IN or IN0, IN1, IN2
    else {
	if ($cmd =~ /EMS_IN\d*_EMS/ && scalar(@INPUT) == 0) {
	    die("ERROR: Step $step requires input from prior steps, but none defined.");
	}
	$cmd =~ s/EMS_IN(\d)_EMS/$INPUT[$1]/g;
	$cmd =~ s/EMS_IN_EMS/$INPUT[0]/g;
    }
    $cmd =~ s/EMS_OUT_EMS/$output/g;
    if (defined($STEP_TMPNAME{"$module:$stepname"})) {
      my $tmp = $dir."/".$STEP_TMPNAME{"$module:$stepname"}.".$VERSION";
      $cmd =~ s/EMS_TMP_EMS/$tmp/g;
    }
    $cmd =~ s/VERSION/$VERSION/g;
    print "\tcmd is $cmd\n" if $VERBOSE;
    while ($cmd =~ /^([\S\s]*)\$\{([^\s\/\"\']+)\}([\S\s]*)$/ ||
           $cmd =~ /^([\S\s]*)\$([^\s\/\"\']+)([\S\s]*)$/) {
	my ($pre,$variable,$post) = ($1,$2,$3);
	$cmd = $pre
	    . &check_backoff_and_get(&extend_local_name($module,$set,$variable))
	    . $post;
    }

    # deal with pipelined commands
    $cmd =~ s/\|(.*)(\<\s*\S+) /$2 \| $1 /g;

    # deal with gzipped input
    my $c = "";
    foreach my $cmd (split(/[\n\r]+/,$cmd)) {
      if ($cmd =~ /\<\s*(\S+) / && ! -e $1 && -e "$1.gz") {
        $cmd =~ s/([^\n\r]+)\s*\<\s*(\S+) /zcat $2.gz \| $1 /;
      }
      else {
        $cmd =~ s/([^\n\r]+)\s*\<\s*(\S+\.gz)/zcat $2 \| $1/;
      }
      $c .= $cmd."\n";
    }
    $cmd = $c;

    # create directory for output
    if ($output =~ /\//) { # ... but why would it not?
	my $out_dir = $output;
	$out_dir =~ s/^(.+)\/[^\/]+$/$1/;
	$cmd = "mkdir -p $out_dir\n$cmd";
    }

    &create_step($step_id,$cmd);
    return 1;
}

### SUBS for defining steps

sub create_step {
    my ($step_id,$cmd) = @_;
    my ($module,$set,$step) = &deconstruct_name($DO_STEP[$step_id]);
    my $file = &versionize(&step_file2($module,$set,$step));
    my $dir = &check_and_get("GENERAL:working-dir");
    my $subdir = $module;
    $subdir =~ tr/A-Z/a-z/; 
    $subdir = "evaluation" if $subdir eq "reporting";
    $subdir = "lm" if $subdir eq "interpolated-lm";
    open(STEP,">$file") or die "Cannot open: $!";
    print STEP "#!/bin/bash\n\n";
    print STEP "PATH=\"".$ENV{"PATH"}."\"\n";
    print STEP "cd $dir\n";
    print STEP "echo 'starting at '`date`' on '`hostname`\n";
    print STEP "mkdir -p $dir/$subdir\n\n";
    print STEP "$cmd\n\n";
    print STEP "echo 'finished at '`date`\n";
    print STEP "touch $file.DONE\n";
    close(STEP);
}    

sub get {
    return &check_and_get($_[0],"allow_undef");
}

sub check_and_get {
    my ($parameter,$allow_undef) = @_;
    if (!defined($CONFIG{$parameter})) {
	return if $allow_undef;
	print STDERR "ERROR: you need to define $parameter\n";
	exit;
    }
    return $CONFIG{$parameter}[0];
}

sub backoff_and_get {
    return &check_backoff_and_get($_[0],"allow_undef");
}

sub check_backoff_and_get {
    my $VALUE = &check_backoff_and_get_array(@_);
    return ${$VALUE}[0] if $VALUE;
}

sub backoff_and_get_array {
    return &check_backoff_and_get_array($_[0],"allow_undef");
}

sub check_backoff_and_get_array {
    my ($parameter,$allow_undef) = @_;
    return $CONFIG{$parameter} if defined($CONFIG{$parameter});

    # remove set -> find setting for module
    $parameter =~ s/:.*:/:/;
    return $CONFIG{$parameter} if defined($CONFIG{$parameter});

    # remove model -> find global setting
    $parameter =~ s/^[^:]+:/GENERAL:/;
    return $CONFIG{$parameter} if defined($CONFIG{$parameter});

    return if $allow_undef;
    print STDERR "ERROR: you need to define $parameter\n";
    exit;
}

# the following two functions deal with getting information about
# files that are passed between steps. this are either specified
# in the meta file (default) or in the configuration file (here called
# 'specified', in the step management referred to as 'given').

sub get_specified_or_default_file {
    my ($specified_module,$specified_set,$specified_parameter,
	$default_module,  $default_set,  $default_step) = @_;
    my $specified = 
	&construct_name($specified_module,$specified_set,$specified_parameter);
    if (defined($CONFIG{$specified})) {
	print "\t\texpanding $CONFIG{$specified}[0]\n" if $VERBOSE;
	return &long_file_name($CONFIG{$specified}[0],$default_module,$default_set);
    }
    return &get_default_file($default_module,  $default_set,  $default_step);
}

sub get_tmp_file {
    my ($module,$set,$step,$version) = @_;
    $version = $VERSION unless $version;
    my $tmp_file = $STEP_TMPNAME{"$module:$step"};
    if ($set) {
	$tmp_file =~ s/^(.+\/)([^\/]+)$/$1$set.$2/g;
    }
    $tmp_file = &versionize(&long_file_name($tmp_file,$module,$set), $version);
    return $tmp_file;
}

sub get_default_file {
    my ($default_module,  $default_set,  $default_step) = @_;
#    print "\tget_default_file($default_module,  $default_set,  $default_step)\n";

    # get step name
    my $step = &construct_name($default_module,$default_set,$default_step);
#    print "\t\tstep is $step\n";

    # earlier step, if this step is passed
    my $i = $STEP_LOOKUP{$step};
#    print "\t\tcan't lookup $step -> $i!\n" unless $i;
    while (defined($PASS{$i})) {
	if (scalar @{$DEPENDENCY[$i]} == 0) {
#	    print "\t\tpassing to given\n";
	    my $out = $STEP_IN{&defined_step($step)}[0];
	    my ($module,$set) = &deconstruct_name($step);
#	    print "\t\t$out -> ".&construct_name($module,$set,$out)."\n";
	    my $name = &construct_name($module,$set,$out);
	    return &check_backoff_and_get($name);
	}
#	print "\t\tpassing $step\n";
	$i = $DEPENDENCY[$i][0];
	$step = $DO_STEP[$i];
#	print "\t\tbacking off to $step\n";
        ($default_module,$default_set,$default_step) = &deconstruct_name($step);
    }

    # get file name
    my $default = $STEP_OUTNAME{&defined_step($step)};
#    print "\t\tdefined_step is ".&defined_step($step)."\n";
    die("no specified default name for $step") unless $default;

    if ($default_set) {
	$default =~ s/^(.+\/)([^\/]+)$/$1$default_set.$2/g;
    }

    # if from a step that is redone, get version number
    my $version = 0;
    $version = $RE_USE[$STEP_LOOKUP{$step}] if defined($STEP_LOOKUP{$step}) && $#RE_USE >= $STEP_LOOKUP{$step};
    $version = "*" if $version > 1e6;    # any if re-use checking
    $version = $VERSION unless $version; # current version if no re-use

    return &versionize(&long_file_name($default,$default_module,$default_set),
		       $version);
}

sub long_file_name {
    my ($file,$module,$set) = @_;
    return $file if $file =~ /^\// || $file =~ / \//;

    if ($file !~ /\//) {
	my $dir = $module;
	$dir =~ tr/A-Z/a-z/;
	$file = "$dir/$file";
    }

    my $module_working_dir_parameter = 
	$module . ($set ne "" ? ":$set" : "") . ":working-dir";

    if (defined($CONFIG{$module_working_dir_parameter})) {
	return $CONFIG{$module_working_dir_parameter}[0]."/".$file;
    }
    return &check_and_get("GENERAL:working-dir")."/".$file;
}

sub compute_version_number {
    my $dir = &check_and_get("GENERAL:working-dir");    
    $VERSION = 1;
    return unless -e $dir;
    open(LS,"find $dir/steps -maxdepth 1 -follow |");
    while(<LS>) {
	s/.+\/([^\/]+)$/$1/; # ignore path
	if ( /^(\d+)$/ ) {
	    if ($1 >= $VERSION) {
		$VERSION = $1 + 1;
	    }
	}
    }
    close(LS);
}

sub steps_file {
  my ($file,$run) = @_;
  return "steps/$run/$file";
}
