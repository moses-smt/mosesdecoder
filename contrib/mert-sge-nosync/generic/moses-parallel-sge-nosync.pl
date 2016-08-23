#! /usr/bin/perl

# $Id$
#######################
# Revision history
#
# 02 Aug 2006 added strict requirement
# 01 Aug 2006 fix bug about inputfile parameter
#             fix bug about suffix index generation
# 31 Jul 2006 added parameter for reading queue parameters
# 29 Jul 2006 added code to handling consfusion networks
# 28 Jul 2006 added a better policy for removing jobs from the queue in case of killing signal (CTRL-C)
#             added the parameter -qsub-prefix which sets the prefix for the name of submitted jobs
# 27 Jul 2006 added safesystem() function and other checks to handle with process failure
#             added checks for existence of decoder and configuration file
# 26 Jul 2006 fix a bug related to the use of absolute path for srcfile and nbestfile

use strict;
use Net::OpenSSH::Compat::Perl;


#######################
#Customizable parameters 

#parameters for submiiting processes through Sun GridEngine
my $submithost = undef;
my $queueparameters="";
my $batch_and_join = undef;
my $processid="$$";

# look for the correct pwdcmd 
my $pwdcmd = getPwdCmd();

my $workingdir = `$pwdcmd`; chomp $workingdir;
my $tmpdir="$workingdir/tmp$$";
my $splitpfx="split$$";


$SIG{'INT'} = \&kill_all_and_quit; # catch exception for CTRL-C

#######################
#Default parameters 
my $jobscript="$workingdir/job$$";
my $qsubout="$workingdir/out.job$$";
my $qsuberr="$workingdir/err.job$$";

my $mosesparameters="";
my $feed_moses_via_stdin = 0;
      # a workaround, for a reason, the default "-input-file X" blocks
      # my moses, while "< X" works fine.
my $cfgfile=""; #configuration file

my $version=undef;
my $help=0;
my $dbg=0;
my $jobs=4;
my $mosescmd="$ENV{MOSESBIN}/moses"; #decoder in use
my $inputlist=undef;
my $inputfile=undef;
my $inputtype=0;
my @nbestlist=();
my $nbestlist=undef;
my $nbestfile=undef;
my $oldnbestfile=undef;
my $oldnbest=undef;
my $nbestflag=0;
my @wordgraphlist=();
my $wordgraphlist=undef;
my $wordgraphfile=undef;
my $wordgraphflag=0;
my $robust=5; # resubmit crashed jobs robust-time
my $alifile=undef;
my $outnbest=undef;
my $logfile="";
my $logflag="";
my $searchgraphlist="";
my $searchgraphfile="";
my $searchgraphflag=0;
my $qsubname="MOSES";
my $qsubwrapper=undef;
my $qsubwrapper_exit=undef;
my $old_sge = 0; # assume old Sun Grid Engine (<6.0) where qsub does not
                 # implement -sync and -b
my $___LATTICE_SAMPLES = 0;
my $___DECODER_FLAGS = "";   # additional parameters to pass to the decoder
my $___N_BEST_LIST_SIZE = 100;
my $___RANGES = undef;
my $___WORKING_DIR = undef;
my $SCRIPTS_ROOTDIR = undef;
my $postdecodecmd = undef;
my $postdecodeargs = undef;


my $run = 0;
my $jobid = -1;
my $prevjid = undef;
my $need_to_normalize = 1;

#######################
# Command line options processing
sub init(){



  use Getopt::Long qw(:config pass_through no_ignore_case permute);
  GetOptions('version'=>\$version,
	     'help'=>\$help,
	     'debug'=>\$dbg,
	     'jobs=i'=>\$jobs,
	     'decoder=s'=> \$mosescmd,
	     'robust=i' => \$robust,
             'script-rootdir=s' => \$SCRIPTS_ROOTDIR,
             'lattice-samples=i' => \$___LATTICE_SAMPLES,
	     'decoder-flags=s' => \$___DECODER_FLAGS,  
             'feed-decoder-via-stdin'=> \$feed_moses_via_stdin,
	     'logfile=s'=> \$logfile,
	     'i|inputfile|input-file=s'=> \$inputlist,
             'n-best-list-size=s'=> \$___N_BEST_LIST_SIZE,
             'n-best-file=s'=> \$oldnbestfile,
             'n-best-size=i'=> \$oldnbest,
	     'output-search-graph|osg=s'=> \$searchgraphlist,
             'output-word-graph|owg=s'=> \$wordgraphlist,
             'alignment-output-file=s'=> \$alifile,
	     'submithost=s'=> \$submithost,
             'queue-parameters=s'=> \$queueparameters,
	     'inputtype=i'=> \$inputtype,
	     'config|f=s'=>\$cfgfile,
             'ranges=s@'=> \$___RANGES,
	     'old-sge' => \$old_sge,
             'run=i' => \$run,
             'need-to-normalize' => \$need_to_normalize,
             'working-dir=s' => \$___WORKING_DIR,
             'qsubwrapper=s' => \$qsubwrapper,
             'qsubwrapper-exit=s' => \$qsubwrapper_exit
	    ) or exit(1);

#            'decoder-parameters=s'=> \$mosesparameters,
#             'n-best-list=s'=> \$nbestlist,
#             'qsub-prefix=s'=> \$qsubname,
}

sub init_secondpart() {
  getNbestParameters();

  getSearchGraphParameters();

  getWordGraphParameters();
  
  getLogParameters();

  chomp(my $my_username = `whoami`);
  # $submithost = "squeal";

  print STDERR "submithost is $submithost\n";

  my $prevjid = undef;
  my $jobid = undef;

#print_parameters();
#print STDERR "nbestflag:$nbestflag\n";
#print STDERR "searchgraphflag:$searchgraphflag\n";
print STDERR "wordgraphflag:$wordgraphflag\n";
#print STDERR "inputlist:$inputlist\n";

  chomp($inputfile=`basename $inputlist`) if defined($inputlist);

  # $mosesparameters.="@ARGV -config $cfgfile -inputtype $inputtype";
  # $mosesparameters = "@ARGV -config $cfgfile -inputtype $inputtype ";
  # $mosesparameters .= "@ARGV -config $cfgfile -inputtype $inputtype ";
  $mosesparameters .= " -config $cfgfile -inputtype $inputtype ";

}


#######################
##print version
sub version(){
#    print STDERR "version 1.0 (15-07-2006)\n";
#    print STDERR "version 1.1 (17-07-2006)\n";
#    print STDERR "version 1.2 (18-07-2006)\n";
#    print STDERR "version 1.3 (21-07-2006)\n";
#    print STDERR "version 1.4 (26-07-2006)\n";
#   print STDERR "version 1.5 (27-07-2006)\n";
#    print STDERR "version 1.6 (28-07-2006)\n";
#    print STDERR "version 1.7 (29-07-2006)\n";
#    print STDERR "version 1.8 (31-07-2006)\n";
#    print STDERR "version 1.9 (01-08-2006)\n";
#    print STDERR "version 1.10 (02-08-2006)\n";
#	print STDERR "version 1.11 (10-10-2006)\n";
#	print STDERR "version 1.12 (27-12-2006)\n";
#	print STDERR "version 1.13 (29-12-2006)\n";
	print STDERR "version 1.13b (01-04-2014)\n";
    exit(1);
}

#usage
sub usage(){
  print STDERR "moses-parallel.pl [parallel-options]  [moses-options]\n";
  print STDERR "Options marked (*) are required.\n";
  print STDERR "Parallel options:\n";
  print STDERR "*  -decoder <file> Moses decoder to use\n";
  print STDERR "*  -i|inputfile|input-file <file>   the input text to translate\n";
  print STDERR "*  -jobs <N> number of required jobs\n";
  print STDERR "   -logfile <file> file where storing log files of all jobs\n";
  print STDERR "   -queue-parameters <string> specific requirements for queue\n";
  print STDERR "   -old-sge Assume Sun Grid Engine < 6.0\n";
  print STDERR "   -debug debug\n";
  print STDERR "   -version print version of the script\n";
  print STDERR "   -help this help\n";
  print STDERR "Moses options:\n";
  print STDERR "   -inputtype <0|1|2> 0 for text, 1 for confusion networks, 2 for lattices\n";
  print STDERR "   -output-search-graph (osg) <file>: Output connected hypotheses of search into specified filename\n";
  print STDERR "   -output-word-graph (osg) '<file> <0|1>': Output stack info as word graph. Takes filename, 0=only hypos in stack, 1=stack + nbest hypos\n";
  print STDERR "   IMPORTANT NOTE: use single quote to group parameters of -output-word-graph\n";
  print STDERR "                   This is different from standard moses\n";
  print STDERR "   -lattice-samples : how many lattice samples (Chatterjee & Cancedda, emnlp 2010) (added option in moses-parallel-sge-nosync)\n";
  print STDERR "   -n-best-list-size <N> : size of nbest lists (added option in moses-parallel-sge-nosync)\n";
  print STDERR "    NOTE: -n-best-file-n-best-size    are passed to the decoder as \"-n-best-list <file> <N>\"\n";
  print STDERR "   -decoder-flags : (added option is moses-parallel-sge-nosync)\n";
  print STDERR "   -ranges : (added option is moses-parallel-sge-nosync)\n";
  print STDERR "   -run : (add option in moses-parallel-sge-nosync)\n";
  print STDERR "*  -config (f) <cfgfile> configuration file\n";
  print STDERR "All other options are passed to Moses\n";
  print STDERR "  (This way to pass parameters is maintained for back compatibility\n";
  print STDERR "   but preferably use -decoder-parameters)\n";
  exit(1);
}





#printparameters
sub print_parameters(){
  print STDERR "Inputfile: $inputlist\n";
  print STDERR "Configuration file: $cfgfile\n";
  print STDERR "Decoder in use: $mosescmd\n";
  print STDERR "Number of jobs:$jobs\n";
  print STDERR "Nbest list: $nbestlist\n" if ($nbestflag);
  print STDERR "Output Search Graph: $searchgraphlist\n" if ($searchgraphflag);
  print STDERR "Output Word Graph: $wordgraphlist\n" if ($wordgraphflag);
  print STDERR "LogFile:$logfile\n" if ($logflag);
  print STDERR "Qsub name: $qsubname\n";
  print STDERR "Queue parameters: $queueparameters\n";
  print STDERR "Inputtype: text\n" if $inputtype == 0;
  print STDERR "Inputtype: confusion network\n" if $inputtype == 1;
  print STDERR "Inputtype: lattices\n" if $inputtype == 2;
  
  print STDERR "parameters directly passed to Moses: $mosesparameters\n";
}

#get parameters for log file
sub getLogParameters(){
  if ($logfile){ $logflag=1; }
}

#get parameters for nbest computation (possibly from configuration file)
sub getNbestParameters(){
  if (!$nbestlist){
    open (CFG, "$cfgfile");
    while (chomp($_=<CFG>)){
      if (/^\[n-best-list\]/){
	my $tmp;
	while (chomp($tmp=<CFG>)){
	  last if $tmp eq "" || $tmp=~/^\[/;
	  $nbestlist .= "$tmp ";
	}
	last;
      }
    }
    close(CFG);
  }

  if ($nbestlist){
     if ($oldnbestfile){
        print STDERR "There is a conflict between NEW parameter -n-best-list and OBSOLETE parameter -n-best-file\n";
        print STDERR "Please use only -nbest-list '<file> <N> [distinct]\n";
        exit;
     }
  }
  else{
    if ($oldnbestfile){
       print STDERR "You are using the OBSOLETE parameter -n-best-file\n";
       print STDERR "Next time please use only -n-best-list '<file> <N> [distinct]\n";
       $nbestlist="$oldnbestfile";
       if ($oldnbest){ $nbestlist.=" $oldnbest"; }
       else { $nbestlist.=" 1"; }
    }
  }

  if ($nbestlist){
    my @tmp=split(/[ \t]+/,$nbestlist);
    @nbestlist = @tmp;

    if ($nbestlist[0] eq '-'){ $nbestfile="nbest"; }
    else{ chomp($nbestfile=`basename $nbestlist[0]`);     }
    $nbestflag=1;
  }
  print STDERR "getNbest\n";
  print STDERR "nbestflag = $nbestflag\n";
  print STDERR "nbestfile = $nbestfile\n";


}

#get parameters for search graph computation (possibly from configuration file)
sub getSearchGraphParameters(){
  if (!$searchgraphlist){
    open (CFG, "$cfgfile");
    while (chomp($_=<CFG>)){
      if (/^\[output-search-graph\]/ || /^\[osg\]/){
	my $tmp;
	while (chomp($tmp=<CFG>)){
	  last if $tmp eq "" || $tmp=~/^\[/;
	  $searchgraphlist = "$tmp";
	}
	last;
      }
    }
    close(CFG);
  }
  if ($searchgraphlist){
    if ($searchgraphlist eq '-'){ $searchgraphfile="searchgraph"; }
    else{ chomp($searchgraphfile=`basename $searchgraphlist`); }
    $searchgraphflag=1;
  }
}

#get parameters for word graph computation (possibly from configuration file)
sub getWordGraphParameters(){
  if (!$wordgraphlist){
    open (CFG, "$cfgfile");
    while (chomp($_=<CFG>)){
      if (/^\[output-word-graph\]/ || /^\[owg\]/){
        my $tmp;
        while (chomp($tmp=<CFG>)){
          last if $tmp eq "" || $tmp=~/^\[/;
          $wordgraphlist .= "$tmp ";
        }
        last;
      }
    }
    close(CFG);
  }
  if ($wordgraphlist){
    my @tmp=split(/[ \t]+/,$wordgraphlist);
    @wordgraphlist = @tmp;

    if ($wordgraphlist[0] eq '-'){ $wordgraphfile="wordgraph"; }
    else{ chomp($wordgraphfile=`basename $wordgraphlist[0]`);     }
    $wordgraphflag=1;
  }
}

sub sanity_check_order_of_lambdas {
  my $featlist = shift;
  my $filename_or_stream = shift;

  my @expected_lambdas = @{$featlist->{"names"}};
  my @got = get_order_of_scores_from_nbestlist($filename_or_stream);
  die "Mismatched lambdas. Decoder returned @got, we expected @expected_lambdas"
    if "@got" ne "@expected_lambdas";
}




#######################
#Script starts here

init();

print "I have started parallel moses!!";

# moses.ini file uses FULL names for lambdas, while this training script
# internally (and on the command line) uses ABBR names.
my @ABBR_FULL_MAP = qw(d=weight-d lm=weight-l tm=weight-t w=weight-w
 g=weight-generation lex=weight-lex I=weight-i);
my %ABBR2FULL = map {split/=/,$_,2} @ABBR_FULL_MAP;
my %FULL2ABBR = map {my ($a, $b) = split/=/,$_,2; ($b, $a);} @ABBR_FULL_MAP;




version() if $version;
usage() if $help;

#######################################
# incorporate run_decoder() here

### moved to below
# my $qsubname = mert$run;
# $mosesparameters = "$___DECODER_FLAGS $decoder_config";
# $nbestlist = "$filename $___N_BEST_LIST_SIZE";


my $featlist = get_featlist_from_moses("./run$run.moses.ini");
$featlist = insert_ranges_to_featlist($featlist, $___RANGES);


## sub run_decoder {
# my ($featlist, $run, $need_to_normalize) = @_;
my $filename_template = "run%d.best$___N_BEST_LIST_SIZE.out";
my $filename = sprintf($filename_template, $run);
my $lsamp_filename = undef;
if ($___LATTICE_SAMPLES) {
  my $lsamp_filename_template = "run%d.lsamp$___LATTICE_SAMPLES.out";
  $lsamp_filename = sprintf($lsamp_filename_template, $run);
}

# user-supplied parameters
print STDERR "params = $___DECODER_FLAGS\n";


# parameters to set all model weights (to override moses.ini)
my @vals = @{$featlist->{"values"}};
if ($need_to_normalize) {
  print STDERR "Normalizing lambdas: @vals\n";
  my $totlambda=0;
  grep($totlambda+=abs($_),@vals);
  grep($_/=$totlambda,@vals);
}


########################################

    # parameters to set all model weights (to override moses.ini)
    my @vals = @{$featlist->{"values"}};
    if ($need_to_normalize) {
      print STDERR "Normalizing lambdas: @vals\n";
      my $totlambda=0;
      grep($totlambda+=abs($_),@vals);
      grep($_/=$totlambda,@vals);
    }
    # moses now does not seem accept "-tm X -tm Y" but needs "-tm X Y"
    my %model_weights;
    for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
      my $name = $featlist->{"names"}->[$i];
      $model_weights{$name} = "-$name" if !defined $model_weights{$name};
      $model_weights{$name} .= sprintf " %.6f", $vals[$i];
    }
    my $decoder_config = join(" ", values %model_weights);
    $decoder_config .= " -weight-file run$run.sparse-weights" if -e "run$run.sparse-weights";
    print STDERR "DECODER_CFG = $decoder_config\n";
    print "decoder_config = $decoder_config\n";
    



#########################################

# moses now does not seem accept "-tm X -tm Y" but needs "-tm X Y"
my %model_weights;
for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
  my $name = $featlist->{"names"}->[$i];
  $model_weights{$name} = "-$name" if !defined $model_weights{$name};
  $model_weights{$name} .= sprintf " %.6f", $vals[$i];
}
my $decoder_config = join(" ", values %model_weights);
$decoder_config .= " -weight-file run$run.sparse-weights" if -e "run$run.sparse-weights";
print STDERR "DECODER_CFG = $decoder_config\n";
print STDERR "decoder_config = $decoder_config\n";

####### moved here??? ###########
$qsubname = "dec$run";
# $mosesparameters = "$___DECODER_FLAGS $decoder_config";
$mosesparameters = "$___DECODER_FLAGS $decoder_config ";

print STDERR "moses parameter with ___DECODER_FLAGS $___DECODER_FLAGS decoder_config $decoder_config and $qsubname\n";
print STDERR "$mosesparameters\n";

$nbestlist = "$filename $___N_BEST_LIST_SIZE";

print STDERR "to output -n-best-list $nbestlist\n";

init_secondpart();
#################################


# run the decoder
my $decoder_cmd;
my $lsamp_cmd = "";
if ($___LATTICE_SAMPLES) {
  $lsamp_cmd = " -lattice-samples $lsamp_filename $___LATTICE_SAMPLES ";
}


if (!defined $inputlist || !defined $mosescmd || ! defined $cfgfile) {
  print STDERR "Please specify -input-file, -decoder and -config\n";
  usage();
}

#checking if inputfile exists
if (! -e ${inputlist} ){
  print STDERR "Inputfile ($inputlist) does not exists\n";
  usage();
}

#checking if decoder exists
if (! -e $mosescmd) {
  print STDERR "Decoder ($mosescmd) does not exists\n";
  usage();
}

#checking if configfile exists
if (! -e $cfgfile) {
  print STDERR "Configuration file ($cfgfile) does not exists\n";
  usage();
}


print_parameters(); # so that people know
exit(1) if $dbg; # debug mode: just print and do not run


#splitting test file in several parts
#$decimal="-d"; #split does not accept this options (on MAC OS)
my $decimal="";

my $cmd;
my $sentenceN;
my $splitN;

my @idxlist=();
my $idxliststr="";

if ($inputtype==0){ #text input
#getting the number of input sentences (one sentence per line)
  chomp($sentenceN=`wc -l ${inputlist} | awk '{print \$1}' `);

#Reducing the number of jobs if less sentences to translate
  if ($jobs>$sentenceN){ $jobs=$sentenceN; }

#Computing the number of sentences for each files
  if ($sentenceN % $jobs == 0){ $splitN=int($sentenceN / $jobs); }
  else{ $splitN=int($sentenceN /$jobs) + 1; }

  if ($dbg){
    print STDERR "There are $sentenceN sentences to translate\n";
    print STDERR "There are at most $splitN sentences per job\n";
  }

  $cmd="split $decimal -a 2 -l $splitN $inputlist ${inputfile}.$splitpfx-";
  safesystem("$cmd") or die;
}
elsif ($inputtype==1){ #confusion network input
  my $tmpfile="/tmp/cnsplit$$";
  $cmd="cat $inputlist | perl -pe 's/\\n/ _CNendline_ /g;' | perl -pe 's/_CNendline_  _CNendline_ /_CNendline_\\n/g;' > $tmpfile";
  safesystem("$cmd") or die;

#getting the number of input CNs
  chomp($sentenceN=`wc -l $tmpfile | awk '{print \$1}' `);

#Reducing the number of jobs if less CNs to translate
  if ($jobs>$sentenceN){ $jobs=$sentenceN; }

#Computing the number of CNs for each files
  if ($sentenceN % $jobs == 0){ $splitN=int($sentenceN / $jobs); }
  else{ $splitN=int($sentenceN /$jobs) + 1; }

  if ($dbg){
    print STDERR "There are $sentenceN confusion networks to translate\n";
    print STDERR "There are at most $splitN sentences per job\n";
  }

  $cmd="split $decimal -a 2 -l $splitN $tmpfile $tmpfile-";
  safesystem("$cmd") or die;
 
  my @idxlist=();
  chomp(@idxlist=`ls $tmpfile-*`);
  grep(s/.+(\-\S+)$/$1/e,@idxlist);

  foreach my $idx (@idxlist){
    $cmd="perl -pe 's/ _CNendline_ /\\n/g;s/ _CNendline_/\\n/g;'";
    safesystem("cat $tmpfile$idx | $cmd > ${inputfile}.$splitpfx$idx ; \\rm -f $tmpfile$idx;");
  }
}
elsif ($inputtype==2){ #confusion network input
#getting the number of input lattices (one lattice per line)
  chomp($sentenceN=`wc -l ${inputlist} | awk '{print \$1}' `);

#Reducing the number of jobs if less lattices to translate
  if ($jobs>$sentenceN){ $jobs=$sentenceN; }

#Computing the number of sentences for each files
  if ($sentenceN % $jobs == 0){ $splitN=int($sentenceN / $jobs); }
  else{ $splitN=int($sentenceN /$jobs) + 1; }

  if ($dbg){
    print STDERR "There are $sentenceN lattices to translate\n";
    print STDERR "There are at most $splitN lattices per job\n";
  }

  $cmd="split $decimal -a 2 -l $splitN $inputlist ${inputfile}.$splitpfx-";
  safesystem("$cmd") or die;
}
else{ #unknown input type
  die "INPUTTYPE:$inputtype is unknown!\n";
}

chomp(@idxlist=`ls ${inputfile}.$splitpfx-*`);
grep(s/.+(\-\S+)$/$1/e,@idxlist);

safesystem("mkdir -p $tmpdir") or die;

preparing_script();



#launching process through the queue
my @sgepids =();
my $splitdecodejid="";

my @idx_todo = ();
foreach (@idxlist) { push @idx_todo,$_; }

# loop up to --robust times
my $max_robust = $robust;
my $robust_idx;
while ($robust && scalar @idx_todo) {
 $robust--;

 my $failure=0;


 foreach my $idx (@idx_todo){

  if ($old_sge) {
    # old SGE understands -b no as the default and does not understand 'yes'
    $batch_and_join = "-j y";
  } else {
    $batch_and_join = "-b yes -j yes";  # -b yes to submit bash script
  }


  ##### Replace the direct qsub command with submit_or_exec_thu_host() #############

  my $split_decoder_cmd = "${jobscript}${idx}.bash";
  &submit_or_exec_thu_host($submithost,$run,$idx,$split_decoder_cmd,$batch_and_join,"decode$run$idx.out","decode$run$idx.err","decode$run$idx.id");
  chomp($jobid=`tail -n 1 decode$run$idx.id`);
  print STDERR "JOBID for decoding sub-task decode$run$idx is $jobid\n";

  ###################################################################################

  ##### # get jobid #################################################################
  push @sgepids, $jobid; 
  $splitdecodejid .= " $jobid";
  ###################################################################################
  }

  ## clear temp file for this robust trial
  foreach my $idx (@idx_todo){
    &exit_submit_thu_host($submithost,$run,$idx,"","decode$run$idx.out","decode$run$idx.err","decode$run$idx.id","decode$run$idx.id.pid",$splitdecodejid);
  } 

  ## WAIT JOB for robust iteration
  $cmd = "date";
  $robust_idx = $max_robust - $robust;
  &submit_or_exec_thu_host($submithost,$run,"R${robust_idx}.W",$cmd,"-sync y","decode${run}R${robust_idx}.W.out","decode${run}R${robust_idx}.W.err","decode${run}R${robust_idx}.W.id",$splitdecodejid);
  # no need to harvest jobid as this is in sync mode
  
  # clear up tmp files
  &exit_submit_thu_host($submithost,$run,"R${robust_idx}.W","","decode${run}R${robust_idx}.W.out","decode${run}R${robust_idx}.W.err","decode${run}R${robust_idx}.W.id","decode${run}R${robust_idx}.W.id.pid",$splitdecodejid);



  # check if some translations failed
  my @idx_still_todo = check_translation();
  if ($robust) {
      # if robust, redo crashed jobs
      ##RESUBMIT_ANYWAY## if ((scalar @idx_still_todo) == (scalar @idxlist)) {
      ##RESUBMIT_ANYWAY##	 # ... but not if all crashed	
      ##RESUBMIT_ANYWAY##	 print STDERR "everything crashed, not trying to resubmit jobs\n";
      ##RESUBMIT_ANYWAY##    $robust = 0;
      ##RESUBMIT_ANYWAY##	 kill_all_and_quit();
      ##RESUBMIT_ANYWAY## }
      @idx_todo = @idx_still_todo;
  }
  else {
      if (scalar (@idx_still_todo)) {
 	 print STDERR "some jobs crashed: ".join(" ",@idx_still_todo)."\n";
 	 # kill_all_and_quit();
      }
      
  }

}


 
$idxliststr = join(" ",@idxlist);


$postdecodecmd = "$SCRIPTS_ROOTDIR/training/sge-nosync/moses-parallel-postdecode-sge-nosync.pl" if !defined $postdecodecmd;

$postdecodeargs = "" if !defined $postdecodeargs;
$postdecodeargs = "$postdecodeargs --process-id $$ --idxliststr \"$idxliststr\"";
$postdecodeargs = "$postdecodeargs --nbestfile $nbestfile" if (defined $nbestfile);
$postdecodeargs = "$postdecodeargs --outnbest $outnbest" if (defined $outnbest);
$postdecodeargs = "$postdecodeargs --input-file $inputfile" if ($inputfile);

my $cmd = "$postdecodecmd $postdecodeargs";
&submit_or_exec_thu_host($submithost,$run,".CONCAT",$cmd,"","run$run.out","run$run.err","postdecode$run.id","$splitdecodejid");


chomp($jobid=`tail -n 1 postdecode$run.id`);
$prevjid = $jobid;

## clear up tmp
&exit_submit_thu_host($submithost,$run,".CONCAT","","run$run.out","run$run.err","postdecode$run.id","postdecode$run.id.pid",$prevjid);



##### OVERALL WAIT JOB ##########
  my $syncscript = "${jobscript}.waitall.sh";

  open (OUT, ">$syncscript");
  my $scriptheader="\#\!/bin/bash\n\#\$ -S /bin/sh\n# Both lines are needed to invoke base\n#the above line is ignored by qsub, unless parameter \"-b yes\" is set!\n\n";
  $scriptheader .="uname -a\n\n";
  $scriptheader .="cd $___WORKING_DIR\n\n";
  print OUT $scriptheader;
  print OUT "'date'";
  close(OUT);

 # safesystem("echo 'date' > $syncscript") or kill_all_and_quit();
 chmod(oct(755),"$syncscript");

 # $cmd="qsub $queueparameters -o /dev/null -e /dev/null -N $qsubname.W -b y /bin/ls > $qsubname.W.log";
 $batch_and_join = "-j y";

 &submit_or_exec_thu_host($submithost,$run,".W",$syncscript,$batch_and_join,"decode$run.W.out","decode$run.W.err","decode$run.W.id",$prevjid);
 chomp($jobid=`tail -n 1 decode$run.W.id`);
 print STDERR "JOBID for wait-job for all decoding sub-task is $jobid\n";
 $prevjid = $jobid;
 
 ## clear-up tmp
 &exit_submit_thu_host($submithost,$run,".W",$batch_and_join,"decode$run.W.out","decode$run.W.err","decode$run.W.id","decode$run.W.id.pid",$prevjid);

exit();





### ending scripts in run_decoder() ##############
sanity_check_order_of_lambdas($featlist, $filename);
return ($filename, $lsamp_filename);
##################################################




#script creation
sub preparing_script(){
  my $currStartTranslationId = 0;
  
  foreach my $idx (@idxlist){
    my $scriptheader="";
    $scriptheader.="\#\! /bin/bash\n\n";
      # !!! this is useless. qsub ignores the first line of the script.
      # Pass '-S /bin/bash' to qsub instead.
    $scriptheader.="uname -a\n\n";
    $scriptheader.="ulimit -c 0\n\n"; # avoid coredumps
    $scriptheader.="cd $workingdir\n\n";

    open (OUT, "> ${jobscript}${idx}.bash");
    print OUT $scriptheader;
    my $inputmethod = $feed_moses_via_stdin ? "<" : "-input-file";

    my $tmpnbestlist="";
    if ($nbestflag){
      $tmpnbestlist="$tmpdir/$nbestfile.$splitpfx$idx $nbestlist[1]";
      $tmpnbestlist = "$tmpnbestlist $nbestlist[2]" if scalar(@nbestlist)==3;
      $tmpnbestlist = "-n-best-list $tmpnbestlist";
    }

    $outnbest=$nbestlist[0];
    if ($nbestlist[0] eq '-'){ $outnbest="nbest$$"; }

    print STDERR "n-best-list $tmpnbestlist\n";
    

    my $tmpalioutfile = "";
    if (defined $alifile){
      $tmpalioutfile="-alignment-output-file $tmpdir/$alifile.$splitpfx$idx";
    }

    my $tmpsearchgraphlist="";
    if ($searchgraphflag){
      $tmpsearchgraphlist="-output-search-graph $tmpdir/$searchgraphfile.$splitpfx$idx";
    }

    my $tmpwordgraphlist="";
    if ($wordgraphflag){
      $tmpwordgraphlist="-output-word-graph $tmpdir/$wordgraphfile.$splitpfx$idx $wordgraphlist[1]";
    }

	my $tmpStartTranslationId = ""; # "-start-translation-id $currStartTranslationId";

    print OUT "$mosescmd $mosesparameters $tmpStartTranslationId $tmpalioutfile $tmpwordgraphlist $tmpsearchgraphlist $tmpnbestlist $inputmethod ${inputfile}.$splitpfx$idx > $tmpdir/${inputfile}.$splitpfx$idx.trans\n\n";
    print OUT "echo exit status \$\?\n\n";

    if (defined $alifile){
      print OUT "\\mv -f $tmpdir/${alifile}.$splitpfx$idx .\n\n";
      print OUT "echo exit status \$\?\n\n";
    }
    if ($nbestflag){
      print OUT "\\mv -f $tmpdir/${nbestfile}.$splitpfx$idx .\n\n";
      print OUT "echo exit status \$\?\n\n";
    }
    if ($searchgraphflag){
      print OUT "\\mv -f $tmpdir/${searchgraphfile}.$splitpfx$idx .\n\n";
      print OUT "echo exit status \$\?\n\n";
    }

    if ($wordgraphflag){
      print OUT "\\mv -f $tmpdir/${wordgraphfile}.$splitpfx$idx .\n\n";
      print OUT "echo exit status \$\?\n\n";
    }

    print OUT "\\mv -f $tmpdir/${inputfile}.$splitpfx$idx.trans .\n\n";
    print OUT "echo exit status \$\?\n\n";
    close(OUT);

    #setting permissions of each script
    chmod(oct(755),"${jobscript}${idx}.bash");
    
    $currStartTranslationId += $splitN;
  }
}

sub concatenate_wordgraph(){
  my $oldcode="";
  my $newcode=-1;
  my %inplength = ();
  my $offset = 0;

  my $outwordgraph=$wordgraphlist[0];
  if ($wordgraphlist[0] eq '-'){ $outwordgraph="wordgraph$$"; }

  open (OUT, "> $outwordgraph");
  foreach my $idx (@idxlist){

#computing the length of each input file
    my @in=();
    open (IN, "${inputfile}.${splitpfx}${idx}.trans");
    @in=<IN>;
    close(IN);
    $inplength{$idx} = scalar(@in);

    open (IN, "${wordgraphfile}.${splitpfx}${idx}");
    while (<IN>){

      my $code="";
      if (/^UTTERANCE=/){
        ($code)=($_=~/^UTTERANCE=(\d+)/);
     
	print STDERR "code:$code offset:$offset\n"; 
        $code += $offset;
        if ($code ne $oldcode){

# if there is a jump between two consecutive codes
# it means that an input sentence is not translated
# fill this hole with a "fictitious" list of wordgraphs
# comprising just one "_EMPTYSEARCHGRAPH_
          while ($code - $oldcode > 1){
             $oldcode++;
             print OUT "UTTERANCE=$oldcode\n";
	print STDERR " to OUT -> code:$oldcode\n"; 
             print OUT "_EMPTYWORDGRAPH_\n";
          }
        }
      
        $oldcode=$code;
        print OUT "UTTERANCE=$oldcode\n";
        next;
      }
      print OUT "$_";
    }
    close(IN);
    $offset += $inplength{$idx};

    while ($offset - $oldcode > 1){
      $oldcode++;
      print OUT "UTTERANCE=$oldcode\n";
      print OUT "_EMPTYWORDGRAPH_\n";
    }
  }
  close(OUT);
}


sub concatenate_searchgraph(){
  my $oldcode="";
  my $newcode=-1;
  my %inplength = ();
  my $offset = 0;

  my $outsearchgraph=$searchgraphlist;
  if ($searchgraphlist eq '-'){ $outsearchgraph="searchgraph$$"; }

  open (OUT, "> $outsearchgraph");
  foreach my $idx (@idxlist){

#computing the length of each input file
    my @in=();
    open (IN, "${inputfile}.${splitpfx}${idx}.trans");
    @in=<IN>;
    close(IN);
    $inplength{$idx} = scalar(@in);

    open (IN, "${searchgraphfile}.${splitpfx}${idx}");
    while (<IN>){
      my ($code,@extra)=split(/[ \t]+/,$_);
      $code += $offset;
      if ($code ne $oldcode){

# if there is a jump between two consecutive codes
# it means that an input sentence is not translated
# fill this hole with a "fictitious" list of searchgraphs
# comprising just one "_EMPTYSEARCHGRAPH_
        while ($code - $oldcode > 1){
           $oldcode++;
           print OUT "$oldcode _EMPTYSEARCHGRAPH_\n";
        }
      }
      $oldcode=$code;
      print OUT join(" ",($oldcode,@extra));
    }
    close(IN);
    $offset += $inplength{$idx};

    while ($offset - $oldcode > 1){
      $oldcode++;
      print OUT "$oldcode _EMPTYSEARCHGRAPH_\n";
    }
  }
  close(OUT);
}

sub concatenate_nbest(){
  my $oldcode="";
  my $newcode=-1;
  my %inplength = ();
  my $offset = 0;
 
# get the list of feature and set a fictitious string with zero scores
  open (IN, "${nbestfile}.${splitpfx}$idxlist[0]");
  my $str = <IN>;
  chomp($str);
  close(IN);
  my ($code,$trans,$featurescores,$globalscore)=split(/\|\|\|/,$str);
  
  my $emptytrans = "  ";
  my $emptyglobalscore = " 0.0";
  my $emptyfeaturescores = $featurescores;
  $emptyfeaturescores =~ s/[-0-9\.]+/0/g;

  my $outnbest=$nbestlist[0];
  if ($nbestlist[0] eq '-'){ $outnbest="nbest$$"; }

  open (OUT, "> $outnbest");
  foreach my $idx (@idxlist){

#computing the length of each input file
    my @in=();
    open (IN, "${inputfile}.${splitpfx}${idx}.trans");
    @in=<IN>;
    close(IN);
    $inplength{$idx} = scalar(@in);

    open (IN, "${nbestfile}.${splitpfx}${idx}");
    while (<IN>){
      my ($code,@extra)=split(/\|\|\|/,$_);
      $code += $offset;
      if ($code ne $oldcode){

# if there is a jump between two consecutive codes
# it means that an input sentence is not translated
# fill this hole with a "fictitious" list of translation
# comprising just one "emtpy translation" with zero scores
        while ($code - $oldcode > 1){
           $oldcode++;
           print OUT join("\|\|\|",($oldcode,$emptytrans,$emptyfeaturescores,$emptyglobalscore)),"\n";
        }
      }
      $oldcode=$code;
      print OUT join("\|\|\|",($oldcode,@extra));
    }
    close(IN);
    $offset += $inplength{$idx};

    while ($offset - $oldcode > 1){
      $oldcode++;
      print OUT join("\|\|\|",($oldcode,$emptytrans,$emptyfeaturescores,$emptyglobalscore)),"\n";
    }
  }
  close(OUT);
}



sub check_exit_status(){
  print STDERR "check_exit_status\n";
  my $failure=0;
  foreach my $idx (@idxlist){
    print STDERR "check_exit_status of job $idx\n";
    open(IN,"$qsubout$idx");
    while (<IN>){
      $failure=1 if (/exit status 1/);
    }
    close(IN);
  }
  return $failure;
}

sub kill_all_and_quit(){
  print STDERR "Got interrupt or something failed.\n";
  print STDERR "kill_all_and_quit\n";
  foreach my $id (@sgepids){
    print STDERR "qdel $id\n";
    safesystem("qdel $id");
  }

  print STDERR "Translation was not performed correctly\n";
  print STDERR "or some of the submitted jobs died.\n";
  print STDERR "qdel function was called for all submitted jobs\n";

  exit(1);
}


sub check_translation(){
  #checking if all sentences were translated
  my $inputN;
  my $outputN;
  my @failed = ();
  foreach my $idx (@idx_todo){
    if ($inputtype==0){#text input
      chomp($inputN=`wc -l ${inputfile}.$splitpfx$idx | cut -d' ' -f1`);
    }
    elsif ($inputtype==1){#confusion network input
      chomp($inputN=`cat ${inputfile}.$splitpfx$idx | perl -pe 's/\\n/ _CNendline_ /g;' | perl -pe 's/_CNendline_  _CNendline_ /_CNendline_\\n/g;' | wc -l | cut -d' ' -f1 `);
    }
    elsif ($inputtype==2){#lattice input
      chomp($inputN=`wc -l ${inputfile}.$splitpfx$idx | cut -d' ' -f1`);
    }
    else{#unknown input
      die "INPUTTYPE:$inputtype is unknown!\n";
    }
    chomp($outputN=`wc -l ${inputfile}.$splitpfx$idx.trans | cut -d' ' -f1`);
    
    if ($inputN != $outputN){
      print STDERR "Split ($idx) were not entirely translated\n";
      print STDERR "outputN=$outputN inputN=$inputN\n";
      print STDERR "outputfile=${inputfile}.$splitpfx$idx.trans inputfile=${inputfile}.$splitpfx$idx\n";
      push @failed,$idx;
    }
  }
  return @failed;
}

sub check_translation_old_sge(){
  #checking if all sentences were translated
  my $inputN;
  my $outputN;
  foreach my $idx (@idx_todo){
    if ($inputtype==0){#text input
      chomp($inputN=`wc -l ${inputfile}.$splitpfx$idx | cut -d' ' -f1`);
    }
    elsif ($inputtype==1){#confusion network input
      chomp($inputN=`cat ${inputfile}.$splitpfx$idx | perl -pe 's/\\n/ _CNendline_ /g;' | perl -pe 's/_CNendline_  _CNendline_ /_CNendline_\\n/g;' | wc -l |
 cut -d' ' -f1 `);
    }
    elsif ($inputtype==2){#lattice input
      chomp($inputN=`wc -l ${inputfile}.$splitpfx$idx | cut -d' ' -f1`);
    }
    else{#unknown input
      die "INPUTTYPE:$inputtype is unknown!\n";
    }
    chomp($outputN=`wc -l ${inputfile}.$splitpfx$idx.trans | cut -d' ' -f1`);

    if ($inputN != $outputN){
      print STDERR "Split ($idx) were not entirely translated\n";
      print STDERR "outputN=$outputN inputN=$inputN\n";
      print STDERR "outputfile=${inputfile}.$splitpfx$idx.trans inputfile=${inputfile}.$splitpfx$idx\n";
      return 1;
    }
		
  }
  return 0; 
}

sub remove_temporary_files(){
  #removing temporary files
  foreach my $idx (@idxlist){
    unlink("${inputfile}.${splitpfx}${idx}.trans");
    unlink("${inputfile}.${splitpfx}${idx}");
    if (defined $alifile){ unlink("${alifile}.${splitpfx}${idx}"); }
    if ($nbestflag){ unlink("${nbestfile}.${splitpfx}${idx}"); }
    if ($searchgraphflag){ unlink("${searchgraphfile}.${splitpfx}${idx}"); }
    if ($wordgraphflag){ unlink("${wordgraphfile}.${splitpfx}${idx}"); }
    unlink("${jobscript}${idx}.bash");
    unlink("${jobscript}${idx}.log");
    unlink("$qsubname.W.log");
    unlink("$qsubout$idx");
    unlink("$qsuberr$idx");
    rmdir("$tmpdir");
  }
  if ($nbestflag && $nbestlist[0] eq '-'){ unlink("${nbestfile}$$"); };
  if ($searchgraphflag  && $searchgraphlist eq '-'){ unlink("${searchgraphfile}$$"); };
  if ($wordgraphflag  && $wordgraphlist eq '-'){ unlink("${wordgraphfile}$$"); };
}

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
    exit 1;
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}


# look for the correct pwdcmd (pwd by default, pawd if it exists)
# I assume that pwd always exists
sub getPwdCmd(){
	my $pwdcmd="pwd";
	my $a;
	chomp($a=`which pawd | head -1 | awk '{print $1}'`);
	if ($a && -e $a){	$pwdcmd=$a;	}
	return $pwdcmd;
}



sub get_featlist_from_moses {
  # run moses with the given config file and return the list of features and
  # their initial values
  
  ### variable conversion for moses-parallel-sge-nosync.pl ###
  my $___INPUTTYPE = $inputtype;
  my $___DECODER = $mosescmd;
  ######################################################
  my $configfn = shift;


  # forceful load features list every time
  my $featlistfn = "./features.list.run${run}_start";
  if (-e $featlistfn) {
    print STDERR "Removing old features list: $featlistfn\n";
    print STDERR "Generating a new one with $configfn\n";
  } 
  print STDERR "Asking moses for feature names and values from $configfn\n";
  my $cmd = "$___DECODER $___DECODER_FLAGS -config $configfn  -inputtype $___INPUTTYPE -show-weights > $featlistfn";
  print STDERR "$cmd\n";   #DEBUG
  safesystem($cmd) or die "Failed to run moses with the config $configfn";
  

  # read feature list
  my @names = ();
  my @startvalues = ();
  open(INI,$featlistfn) or die "Can't read $featlistfn";
  my $nr = 0;
  my @errs = ();
  while (<INI>) {
    $nr++;
    chomp;
    /^(.+) (\S+) (\S+)$/ || die("invalid feature: $_");
    my ($longname, $feature, $value) = ($1,$2,$3);
    next if $value eq "sparse";
    push @errs, "$featlistfn:$nr:Bad initial value of $feature: $value\n"
      if $value !~ /^[+-]?[0-9.e]+$/;
    push @errs, "$featlistfn:$nr:Unknown feature '$feature', please add it to \@ABBR_FULL_MAP\n"
      if !defined $ABBR2FULL{$feature};
    push @names, $feature;
    push @startvalues, $value;
  }
  close INI;
  if (scalar @errs) {
    print STDERR join("", @errs);
    exit 1;
  }
  return {"names"=>\@names, "values"=>\@startvalues};
}


sub insert_ranges_to_featlist {
  ### variable conversion for moses-parallel-sge-nosync.pl ###
  my $___INPUTTYPE = $inputtype;
  my $___DECODER = $mosescmd;
  ######################################################
  
  my $featlist = shift;
  my $ranges = shift;

  $ranges = [] if !defined $ranges;

  # first collect the ranges from options
  my $niceranges;
  foreach my $range (@$ranges) {
    my $name = undef;
    foreach my $namedpair (split /,/, $range) {
      if ($namedpair =~ /^(.*?):/) {
        $name = $1;
        $namedpair =~ s/^.*?://;
        die "Unrecognized name '$name' in --range=$range"
          if !defined $ABBR2FULL{$name};
      }
      my ($min, $max) = split /\.\./, $namedpair;
      die "Bad min '$min' in --range=$range" if $min !~ /^-?[0-9.]+$/;
      die "Bad max '$max' in --range=$range" if $min !~ /^-?[0-9.]+$/;
      die "No name given in --range=$range" if !defined $name;
      push @{$niceranges->{$name}}, [$min, $max];
    }
  }

  # now populate featlist
  my $seen = undef;
  for(my $i=0; $i<scalar(@{$featlist->{"names"}}); $i++) {
    my $name = $featlist->{"names"}->[$i];
    $seen->{$name} ++;
    my $min = 0.0;
    my $max = 1.0;
    if (defined $niceranges->{$name}) {
      my $minmax = shift @{$niceranges->{$name}};
      ($min, $max) = @$minmax if defined $minmax;
    }
    $featlist->{"mins"}->[$i] = $min;
    $featlist->{"maxs"}->[$i] = $max;
  }
  return $featlist;
}




sub submit_or_exec_thu_host {
  # use Net::OpenSSH::Compat::Perl;
  
  my $argvlen = @_;
  my $submithost = undef;
  my $run = -1;
  my $idx = "";
  my $batch_and_join = "";
  my $my_username = undef;
  my $cmd = undef;
  my $qsubwrapcmd = undef;
  my $stdout = undef;
  my $stderr = undef;
  my $jidfile = undef;
  my $prevjid = undef;
  

  # if supply 7 arguments, exec without submit
  # if supply 8 arguments, then submit new job
  # if supply 9 arguments, wait for the previous job to finish
  if ($argvlen == 7){
    ($submithost,$run,$idx,$cmd,$batch_and_join,$stdout,$stderr) = @_;
  } elsif ($argvlen == 8){
    ($submithost,$run,$idx,$cmd,$batch_and_join,$stdout,$stderr,$jidfile) = @_;
  } elsif ($argvlen == 9){
    ($submithost,$run,$idx,$cmd,$batch_and_join,$stdout,$stderr,$jidfile,$prevjid) = @_;
  }



  chomp(my $my_username = `whoami`);
  my $ssh = Net::OpenSSH::Compat::Perl->new($submithost, debug=>0);

  print STDERR "submithost = $submithost\n";
  print STDERR "qusbwrapper at = $qsubwrapper\n";

  $ssh->login();

  if ($argvlen == 9) {
    $qsubwrapcmd = "$qsubwrapper -command='$cmd' -queue-parameter=\"$queueparameters $batch_and_join\" -qsub-prefix='$qsubname$idx' -stdout=$stdout -stderr=$stderr -jidfile=$jidfile -prevjid='$prevjid'";
  } elsif ($argvlen == 8) {
    # $qsubwrapcmd = "$qsubwrapper -command='${jobscript}${idx}.bash' -queue-parameter=\"$queueparameters $batch_and_join\" -qsub-prefix='$qsubname$idx' -stdout=$stdout -stderr=$stderr -jidfile=$jidfile"; 
    $qsubwrapcmd = "$qsubwrapper -command='$cmd' -queue-parameter=\"$queueparameters $batch_and_join\" -qsub-prefix='$qsubname$idx' -stdout=$stdout -stderr=$stderr -jidfile=$jidfile"; 
  }
  print STDERR "Executing $qsubwrapcmd in $___WORKING_DIR\n";
  $ssh->cmd("cd $___WORKING_DIR && $qsubwrapcmd");

}

sub exit_submit_thu_host {

  my $argvlen = @_;
  my $submithost = undef;
  my $run = -1;
  my $idx = "";
  my $batch_and_join = "";
  my $my_username = undef;
  my $cmd = undef;
  my $stdout = undef;
  my $stderr = undef;
  my $jidfile = undef;
  my $pidfile = undef;
  my $prevjid = undef;
  my $prevjidarraysize = 0;
  my @prevjidarray = ();
  my $pid = undef;
  my $qsubcmd="";
  my $hj="";

  # if supply 8 arguments, then submit new job
  # if supply 9 arguments, wait for the previous job to finish
  if ($argvlen == 6){
    ($submithost,$run,$idx,$batch_and_join,$stdout,$stderr) = @_;
  } elsif ($argvlen == 8){
    ($submithost,$run,$idx,$batch_and_join,$stdout,$stderr,$jidfile,$pidfile) = @_;
  } elsif ($argvlen == 9){
    ($submithost,$run,$idx,$batch_and_join,$stdout,$stderr,$jidfile,$pidfile,$prevjid) = @_;
  }

  # parse prevjid ########################
  $prevjid =~ s/^\s+|\s+$//g;
  @prevjidarray = split(/\s+/,$prevjid);
  $prevjidarraysize = scalar(@prevjidarray);
  ########################################


  # print STDERR "exec: $stdout\n";

  # read pid from file, and draft exit script ##################
  chomp ($pid=`tail -n 1 $pidfile`);
  open (OUT, ">exitjob$pid.sh");

  my $scriptheader="\#\!/bin/bash\n\#\$ -S /bin/sh\n# Both lines are needed to invoke base\n#the above line is ignored by qsub, unless parameter \"-b yes\" is set!\n\n";
  $scriptheader .="uname -a\n\n";
  $scriptheader .="cd $___WORKING_DIR\n\n";

  print OUT $scriptheader;

  print OUT "if $qsubwrapper_exit -stdout=$stdout -stderr=$stderr -jidfile=$jidfile -pidfile=$pidfile > exitjob$pid.out 2> exitjob$pid.err ; then
    echo 'succeeded'
else
    echo failed with exit status \$\?
    die=1
fi
";
  print OUT "\n\n";

  close (OUT);
  # setting permissions of the script
  chmod(oct(755),"exitjob$pid.sh");
  ##############################################################
 
  # log in submit host #########################################
  chomp(my $my_username = `whoami`);
  my $ssh = Net::OpenSSH::Compat::Perl->new($submithost, debug=>0);

  print STDERR "submithost = $submithost\n";
  print STDERR "my username = $my_username\n";
  print STDERR "qusbwrapper at = $qsubwrapper\n";

  $ssh->login();
  ##############################################################

      
  if ($argvlen==9) {
    if (defined $prevjid && $prevjid!=-1 && $prevjidarraysize == 1){
      $hj = "-hold_jid $prevjid";
    } elsif (defined $prevjid && $prevjidarraysize > 1){
      $hj = "-hold_jid " . join(" -hold_jid ", @prevjidarray);
    }
    $qsubcmd="qsub $queueparameters -o /dev/null -e /dev/null $hj exitjob$pid.sh > exitjob$pid.log 2>&1";
    $ssh->cmd("cd $___WORKING_DIR && $qsubcmd");
  } elsif ($argvlen==8) {
    $qsubcmd="qsub $queueparameters -o /dev/null -e /dev/null exitjob$pid.sh > exitjob$pid.log 2>&1";
    $ssh->cmd("cd $___WORKING_DIR && $qsubcmd");   
  }
  print STDERR "Executing $qsubcmd in $___WORKING_DIR\n";
 
}
 

