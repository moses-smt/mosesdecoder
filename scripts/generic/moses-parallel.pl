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

use FindBin qw($Bin);
use File::Basename;
my $SCRIPTS_ROOTDIR = $Bin;
$SCRIPTS_ROOTDIR =~ s/\/generic$//;
$SCRIPTS_ROOTDIR = $ENV{"SCRIPTS_ROOTDIR"} if defined($ENV{"SCRIPTS_ROOTDIR"});

#######################
#Customizable parameters 

#parameters for submiiting processes through Sun GridEngine
my $queueparameters="";

# look for the correct pwdcmd 
my $pwdcmd = getPwdCmd();

my $workingdir = `$pwdcmd`; chomp $workingdir;
my $tmpdir="$workingdir/tmp$$";
my $splitpfx="split$$";
my $linespfx="lineNum$$";
my $timespfx="times$$";

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
my $robust=5; # resubmit crashed jobs robust-times
my $logfile="";
my $logflag="";
my $existingtimesfile=undef;
my $timesfile="";
my $timesflag="";
my $searchgraphlist="";
my $searchgraphfile="";
my $searchgraphflag=0;
my $qsubname="MOSES";
my $old_sge = 0; # assume old Sun Grid Engine (<6.0) where qsub does not
                 # implement -sync and -b

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
             'decoder-parameters=s'=> \$mosesparameters,
             'feed-decoder-via-stdin'=> \$feed_moses_via_stdin,
	     'logfile=s'=> \$logfile,
	     'timesfile=s'=> \$timesfile,
	     'existingtimesfile=s'=> \$existingtimesfile,
	     'i|inputfile|input-file=s'=> \$inputlist,
             'n-best-list=s'=> \$nbestlist,
             'n-best-file=s'=> \$oldnbestfile,
             'n-best-size=i'=> \$oldnbest,
	     'output-search-graph|osg=s'=> \$searchgraphlist,
             'output-word-graph|owg=s'=> \$wordgraphlist,
	     'qsub-prefix=s'=> \$qsubname,
	     'queue-parameters=s'=> \$queueparameters,
	     'inputtype=i'=> \$inputtype,
	     'config|f=s'=>\$cfgfile,
	     'old-sge' => \$old_sge,
	    ) or exit(1);

  getNbestParameters();

  getSearchGraphParameters();

  getWordGraphParameters();
  
  getLogParameters();

  getTimesParameters();

#print_parameters();
#print STDERR "nbestflag:$nbestflag\n";
#print STDERR "searchgraphflag:$searchgraphflag\n";
print STDERR "wordgraphflag:$wordgraphflag\n";
#print STDERR "inputlist:$inputlist\n";

  chomp($inputfile=`basename $inputlist`) if defined($inputlist);

  $mosesparameters.="@ARGV -config $cfgfile -inputtype $inputtype";
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
	print STDERR "version 1.13 (29-12-2006)\n";
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
  print STDERR "   -timesfile <file> file where storing per-sentence translation times of all jobs\n";
  print STDERR "   -qsub-prefix <string> name for sumbitte jobs\n";
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
  print STDERR "   -n-best-list '<file> <N> [distinct]' where\n";
  print STDERR "                <file>:   file where storing nbest lists\n";
  print STDERR "                <N>:      size of nbest lists\n";
  print STDERR "                distinct: (optional) to activate generation of distinct nbest alternatives\n";
  print STDERR "   IMPORTANT NOTE: use single quote to group parameters of -n-best-list\n";
  print STDERR "                   This is different from standard moses\n";
  print STDERR "   IMPORTANT NOTE: The following two parameters are now OBSOLETE, and they are no more supported\n";
  print STDERR "                   -n-best-file <file> file where storing nbet lists\n";
  print STDERR "                   -n-best-size <N> size of nbest lists\n";
  print STDERR "    NOTE: -n-best-file-n-best-size    are passed to the decoder as \"-n-best-list <file> <N>\"\n";
  print STDERR "*  -config (f) <cfgfile> configuration file\n";
  print STDERR "   -decoder-parameters <string> specific parameters for the decoder\n";
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
  print STDERR "TimesFile:$timesfile\n" if ($timesflag);
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

#get parameters for times file
sub getTimesParameters(){
  if ($timesfile){ $timesflag=1; }
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

#######################
#Script starts here

init();

version() if $version;
usage() if $help;


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

safesystem("mkdir -p $tmpdir") or die;


#splitting test file in several parts
#$decimal="-d"; #split does not accept this options (on MAC OS)
my $decimal="";

my $cmd;
my $sentenceN;
my $splitN;

my @idxlist=();

# SGE jobs can be submitted as array jobs with a range (ex: -t 1-64)
# Unfortunately, SGE can't handle multiple ranges (ex: -t 1-3,9-16,20-64)
#
# If some jobs fail, we want to be able to restart them
# using a minimal number of calls to qsub.
# 
# These two arrays will be used to keep track of the start and end ranges
# of any jobs that need to be re-run.
# 
# So, if jobs 9,10,12,18,19,20 died and need to be restarted, 
# then @idxstarts would be set to  (9,12,18)
#  and @idxends   would be set to (10,12,20)
#
# $idxranges stores the (zero-based) index of the last range,
# so it would be 2 in the example above
my @idxstarts=();
my @idxends=();
my $idxranges=-1;


if ($inputtype>=0 && $inputtype<=2) {

    my $tmpfile;

    if ($inputtype==1) { #confusion network input
	# Transform the text to use only one line per sentence CN
#	$tmpdir="/tmp/$$";
    	$tmpfile="$tmpdir/cnsplit$$";
	$cmd="cat $inputlist | perl -pe 's/\\n/ _CNendline_ /g;' | perl -pe 's/_CNendline_  _CNendline_ /_CNendline_\\n/g;' > $tmpfile";
	safesystem("$cmd") or die;
    } else {
#	$tmpdir = ".";
	$tmpfile = $inputlist;
    }
    
    
    #getting the number of input sentences (one sentence per line)
    chomp($sentenceN=`wc -l ${tmpfile} | awk '{print \$1}' `);

    #Reducing the number of jobs if less sentences to translate
    if ($jobs>$sentenceN){ $jobs=$sentenceN; }

    if ($dbg){
	print STDERR "There are $sentenceN sentences to translate, and $jobs jobs.\n";
    }

    # Split the input sentences
    $cmd="$SCRIPTS_ROOTDIR/generic/balance-corpus --num-parts=$jobs --corpus=$tmpfile --prefix=$inputfile.$splitpfx- --no-zeropad --min-words=0 --index-prefix $inputfile.$linespfx-";
    if ($searchgraphflag || $wordgraphflag) {
	$cmd.=" --balance-naive";
	print STDERR "WARNING: Use of search graph and word graph outputs currently only work with the balance-corpus --balance-naive flag. Enabling that flag in lieu of the default. This disables the use of an existing times file."
    } elsif (defined $existingtimesfile) {
	my @t=();
	open(EXISTINGTIMES,$existingtimesfile) or die "Can't read existing times file:\t$existingtimesfile";
	@t=<EXISTINGTIMES>;
	close(EXISTINGTIMES);
	if ($sentenceN == scalar(@t)) {
	    $cmd.=" --balance-time $existingtimesfile";
	} else {
	    print STDERR "WARNING: The number of time entries doesn't match the number of sentences - therefore, not using existing time file.";
	}
    }
    safesystem("$cmd") or die;

    
    if ($inputtype==1) { #confusion network input
    	my @idxlist=();
	chomp(@idxlist=`ls ${tmpdir}/$tmpfile-[0-9]*`);
	grep(s/.+\-(\d+)\D*$/$1/e,@idxlist);

	# Restore the text back to multiple lines per sentence CN
	foreach my $idx (@idxlist){
	    $cmd="perl -pe 's/ _CNendline_ /\\n/g;s/ _CNendline_/\\n/g;'";
	    safesystem("cat ${tmpdir}/$tmpfile-$idx | $cmd > ${inputfile}.$splitpfx-$idx ; \\rm -f $tmpfile-$idx;");
	}
    }

} else {
    # unknown input type
    die "INPUTTYPE:$inputtype is unknown!\n";
}



chomp(@idxlist=`ls ${inputfile}.$splitpfx-*`);
grep(s/.+\-(\d+)\D*$/$1/e,@idxlist);
@idxlist = sort {$a <=> $b} @idxlist;
 
preparing_script();


#launching process through the queue
my @sgepids =();

my @idx_todo = ();
foreach (@idxlist) { push @idx_todo,$_; }

# loop up to --robust times
while ($robust && scalar @idx_todo) {
 $robust--;

 my $failure=0;
 #foreach my $idx (@idx_todo){

  my $batch_and_join = undef;
  if ($old_sge) {
    # old SGE understands -b no as the default and does not understand 'yes'
    $batch_and_join = "-j y";
  } else {
    $batch_and_join = "-b no -j yes";
  }
 calculateIdxRanges();
 for (my $rangeindex=0; $rangeindex<=$idxranges; $rangeindex+=1) {
     
  $cmd="qsub -t $idxstarts[$rangeindex]-$idxends[$rangeindex] $queueparameters $batch_and_join ${jobscript}.bash > ${jobscript}.log 2>&1";
  print STDERR "$cmd\n" if $dbg; 

  safesystem($cmd) or die;

  my ($res,$id);

  open (IN,"${jobscript}.log")
    or die "Can't read id of job ${jobscript}.log";
  chomp($res=<IN>);
  split(/\s+|\./,$res);
  $id=$_[2];
  die "Failed to guess job id from $jobscript.log, got: $res"
    if $id !~ /^[0-9]+$/;
  close(IN);

  push @sgepids, $id;
 }

 #waiting until all jobs have finished
 my $hj = "-hold_jid " . join(" -hold_jid ", @sgepids);

 if ($old_sge) {
  # we need to implement our own waiting script
  my $syncscript = "${jobscript}.sync_workaround_script.sh";
  safesystem("echo 'date' > $syncscript") or kill_all_and_quit();

  my $pwd = `$pwdcmd`; chomp $pwd;

  my $checkpointfile = "${jobscript}.sync_workaround_checkpoint";

  # delete previous checkpoint, if left from previous runs
  safesystem("\\rm -f $checkpointfile") or kill_all_and_quit();

  # start the 'hold' job, i.e. the job that will wait
  $cmd="qsub -cwd $queueparameters $hj -o $checkpointfile -e /dev/null -N $qsubname.W $syncscript 2> $qsubname.W.log";
  safesystem($cmd) or kill_all_and_quit();
  
  # and wait for checkpoint file to appear
  my $nr=0;
  while (!-e $checkpointfile) {
    sleep(10);
    $nr++;
    print STDERR "w" if $nr % 3 == 0;
  }
  print STDERR "End of waiting.\n";
  safesystem("\\rm -f $checkpointfile $syncscript") or kill_all_and_quit();
  
  my $failure = 1;
  my $nr = 0;
  while ($nr < 60 && $failure) {
    $nr ++;
    $failure=&check_exit_status();
    if (!$failure) {
      $failure = check_translation_old_sge();
    }
    last if !$failure;
    print STDERR "Extra wait ($nr) for possibly unfinished processes.\n";
    sleep 10;
  }
 } else {
  # use the -sync option for qsub
  $cmd="qsub $queueparameters -sync y $hj -j y -o /dev/null -e /dev/null -N $qsubname.W -b y /bin/ls > $qsubname.W.log";
  safesystem($cmd) or kill_all_and_quit();

  $failure=&check_exit_status();
 }

 kill_all_and_quit() if $failure && !$robust;

 # check if some translations failed
 my @idx_still_todo = check_translation();
 if ($robust) {
     # if robust, redo crashed jobs
     if ((scalar @idx_still_todo) == (scalar @idxlist)) {
	 # ... but not if all crashed
	 print STDERR "everything crashed, not trying to resubmit jobs\n";
         $robust = 0;
	 kill_all_and_quit();
     }
     @idx_todo = @idx_still_todo;
 }
 else {
     if (scalar (@idx_still_todo)) {
	 print STDERR "some jobs crashed: ".join(" ",@idx_still_todo)."\n";
	 kill_all_and_quit();
     }
     
 }
}

#concatenating translations and removing temporary files
concatenate_1best();
concatenate_logs() if $logflag;
concatenate_runtimes() if $timesflag;
concatenate_nbest() if $nbestflag;  
safesystem("cat nbest$$ >> /dev/stdout") if $nbestlist[0] eq '-';

concatenate_searchgraph() if $searchgraphflag;  
safesystem("cat searchgraph$$ >> /dev/stdout") if $searchgraphlist eq '-';

concatenate_wordgraph() if $wordgraphflag;  
safesystem("cat wordgraph$$ >> /dev/stdout") if $wordgraphlist[0] eq '-';

#Commented out for debugging purposes only. Please uncomment!
#remove_temporary_files();


#script creation
sub preparing_script(){

    my $scriptheader="";
    $scriptheader.="\#\!/bin/bash\n\n";
      # qsub ignores the shebang line, so we'll also use the -S flag
      #
      # Be aware!! If you give qsub -S but not -V,
      #   you likely won't have your regular environment variables set up 

    $scriptheader.="\#\$ \-S /bin/bash\n";
    $scriptheader.="\#\$ \-o $qsubout-\$TASK_ID\n";
    $scriptheader.="\#\$ \-e $qsuberr-\$TASK_ID\n";
    $scriptheader.="\#\$ \-N $qsubname\n\n"; # Unfortunately, qsub doesn't respect the pseudo-variable $TASK_ID for -N

    $scriptheader.="if [ \"\" == \"\$SGE_TASK_ID\" ]; then\n";
    $scriptheader.="\tif [ \"\" == \"\$PBS_ARRAYID\" ]; then\n";
    $scriptheader.="\t\techo \"Job was not submitted as an array job\"\n";
    $scriptheader.="\t\texit 1\n";
    $scriptheader.="\telse\n";
    $scriptheader.="\t\techo \"SGE_TASK_ID is not available, but PBS_ARRAYID is; using that instead.\"\n";
    $scriptheader.="\t\tSGE_TASK_ID=\$PBS_ARRAYID\n";
    $scriptheader.="\tfi\n";
    $scriptheader.="fi\n\n";

    $scriptheader.="uname -a\n\n";
    $scriptheader.="ulimit -c 0\n\n"; # avoid coredumps
    $scriptheader.="cd $workingdir\n";

    $scriptheader.="\n";


    my $idxindex=1;
  foreach my $idx (@idxlist){
      $scriptheader.="idxarray[".$idxindex."]=".$idx."\n";
      $idxindex+=1;
  }
    $scriptheader.="\n";

    open (OUT, "> ${jobscript}.bash");
    print OUT $scriptheader;
    my $inputmethod = $feed_moses_via_stdin ? "<" : "-input-file";

    my $tmpnbestlist="";
    if ($nbestflag){
      $tmpnbestlist="$tmpdir/$nbestfile.$splitpfx-\${idxarray[\$SGE_TASK_ID]} $nbestlist[1]";

      $tmpnbestlist="$tmpdir/$nbestfile.$splitpfx-\${idxarray[\$SGE_TASK_ID]} $nbestlist[1]";
      $tmpnbestlist = "$tmpnbestlist $nbestlist[2]" if scalar(@nbestlist)==3;
      $tmpnbestlist = "-n-best-list $tmpnbestlist";
    }

    my $tmpsearchgraphlist="";
    if ($searchgraphflag){
      $tmpsearchgraphlist="-output-search-graph $tmpdir/$searchgraphfile.$splitpfx-\${idxarray[\$SGE_TASK_ID]}";
    }

    my $tmpwordgraphlist="";
    if ($wordgraphflag){
      $tmpwordgraphlist="-output-word-graph $tmpdir/$wordgraphfile.$splitpfx-\${idxarray[\$SGE_TASK_ID]} $wordgraphlist[1]";
    }

    print OUT "$mosescmd $mosesparameters $tmpwordgraphlist $tmpsearchgraphlist $tmpnbestlist $inputmethod ${inputfile}.$splitpfx-\${idxarray[\$SGE_TASK_ID]} > $tmpdir/${inputfile}.$splitpfx-\${idxarray[\$SGE_TASK_ID]}.trans\n\n";
    print OUT "echo exit status \$\?\n\n";

    if ($nbestflag){
      print OUT "\\mv -f $tmpdir/${nbestfile}.$splitpfx-\${idxarray[\$SGE_TASK_ID]} .\n\n";
      print OUT "echo exit status \$\?\n\n";
    }
    if ($searchgraphflag){
      print OUT "\\mv -f $tmpdir/${searchgraphfile}.$splitpfx-\${idxarray[\$SGE_TASK_ID]} .\n\n";
      print OUT "echo exit status \$\?\n\n";
    }

    if ($wordgraphflag){
      print OUT "\\mv -f $tmpdir/${wordgraphfile}.$splitpfx-\${idxarray[\$SGE_TASK_ID]} .\n\n";
      print OUT "echo exit status \$\?\n\n";
    }

    print OUT "\\mv -f $tmpdir/${inputfile}.$splitpfx-\${idxarray[\$SGE_TASK_ID]}.trans .\n\n";
    print OUT "echo exit status \$\?\n\n";
    close(OUT);

    #setting permissions of each script
    chmod(oct(755),"${jobscript}.bash");
  
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
    open (IN, "${inputfile}.${splitpfx}-${idx}.trans");
    @in=<IN>;
    close(IN);
    $inplength{$idx} = scalar(@in);

    open (IN, "${wordgraphfile}.${splitpfx}-${idx}");
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
    open (IN, "${inputfile}.${splitpfx}-${idx}.trans");
    @in=<IN>;
    close(IN);
    $inplength{$idx} = scalar(@in);

    open (IN, "${searchgraphfile}.${splitpfx}-${idx}");
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

    # Read original line numbers for each part	
    my @lineNum;
    my $lastLine = 0;
    foreach my $idx (@idxlist){
	open (IN,"$inputfile.$linespfx-$idx");
	for (my $i=0; <IN>; $i+=1) {
	    chomp;
	    $lineNum[$idx][$i] = $_;
	    $lastLine=$lineNum[$idx][$i] if ($lineNum[$idx][$i] > $lastLine);
	}
	close(IN);
    }

    # Initialize n-best lists to empty string
    my @nbest;
    for my $originalLine (0 .. $lastLine) {
	$nbest[$originalLine] = "";
    }

    # Read n-best lists for each part
    foreach my $idx (@idxlist){
	open (IN, "${nbestfile}.${splitpfx}-${idx}");
	while (<IN>) {
	    my ($code,@extra)=split(/\|\|\|/,$_);
	    my $originalLine = $lineNum[$idx][$code];
	    $nbest[$originalLine] .= join("\|\|\|",("$originalLine ",@extra));
	}
	close(IN);
    }
    
    # Some input lines may have returned an empty n-best lines
    # get the list of feature and set a fictitious string with zero scores
    open (IN, "${nbestfile}.${splitpfx}-$idxlist[0]");
    my $str = <IN>;
    chomp($str);
    close(IN);
    my ($code,$trans,$featurescores,$globalscore)=split(/\|\|\|/,$str);    
    my $emptytrans = "  ";
    my $emptyglobalscore = " 0.0";
    my $emptyfeaturescores = $featurescores;
    $emptyfeaturescores =~ s/[-0-9\.]+/0/g;

    # Print n-best list results
    my $outnbest=$nbestlist[0];
    if ($nbestlist[0] eq '-'){ $outnbest="nbest$$"; }
    open (OUT, "> $outnbest");
    for my $originalLine (0 .. $lastLine) {
	if ($nbest[$originalLine] eq "") {
	    print OUT join("\|\|\|",("$originalLine ",$emptytrans,$emptyfeaturescores,$emptyglobalscore)),"\n";
	} else {
	    print OUT $nbest[$originalLine];
	}
	
    }
    close(OUT);
    
}


sub concatenate_1best(){
    safesystem("rm -f ${inputfile}.$linespfx-trans");
    foreach my $idx (@idxlist){
	safesystem("paste $inputfile.$linespfx-$idx ${inputfile}.${splitpfx}-${idx}.trans >> ${inputfile}.$linespfx-trans");
    }
    safesystem("cat ${inputfile}.$linespfx-trans | sort -n | cut -f 2");
}

# Use this function to extract a list of 
#   how long it took to translate each sentence
sub concatenate_runtimes(){

    # Extract translation times for each part
    foreach my $idx (@idxlist){
	open (IN, "$qsubout-$idx");
	open (OUT, "> $inputfile.$timespfx-$idx");
	while(my $line=<IN>) {
	    print OUT $1."\n" if $line =~ /^[[:space:]]*Translation took[[:space:]]+([[:digit:]]+\.[[:digit:]]+) seconds[[:space:]]*$/;
	}
	close(IN);
	close(OUT);
    }

    # Properly order the times (by sentence number)
    safesystem("rm -f ${inputfile}.$linespfx-times");
    foreach my $idx (@idxlist){
	safesystem("paste $inputfile.$linespfx-$idx ${inputfile}.${timespfx}-${idx} >> ${inputfile}.$linespfx-times");
    }
    safesystem("cat ${inputfile}.$linespfx-times | sort -n | cut -f 2 > ${timesfile}");

}

sub concatenate_logs(){
  open (OUT, "> ${logfile}");
  foreach my $idx (@idxlist){
    my @in=();
    open (IN, "$qsubout-$idx");
    @in=<IN>;
    print OUT "@in";
    close(IN);
  }
  close(OUT);
}


sub check_exit_status(){
  print STDERR "check_exit_status\n";
  my $failure=0;
  foreach my $idx (@idxlist){
    print STDERR "check_exit_status of job $idx\n";
    open(IN,"$qsubout-$idx");
    while (<IN>){
	if (/exit status 1/) {
	    $failure=1;
	    print STDERR "\tfailure found for $idx\n";
	}
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
      chomp($inputN=`wc -l ${inputfile}.$splitpfx-$idx | cut -d' ' -f1`);
    }
    elsif ($inputtype==1){#confusion network input
      chomp($inputN=`cat ${inputfile}.$splitpfx-$idx | perl -pe 's/\\n/ _CNendline_ /g;' | perl -pe 's/_CNendline_  _CNendline_ /_CNendline_\\n/g;' | wc -l | cut -d' ' -f1 `);
    }
    elsif ($inputtype==2){#lattice input
      chomp($inputN=`wc -l ${inputfile}.$splitpfx-$idx | cut -d' ' -f1`);
    }
    else{#unknown input
      die "INPUTTYPE:$inputtype is unknown!\n";
    }
    chomp($outputN=`wc -l ${inputfile}.$splitpfx-$idx.trans | cut -d' ' -f1`);
    
    if ($inputN != $outputN){
      print STDERR "Split ($idx) were not entirely translated\n";
      print STDERR "outputN=$outputN inputN=$inputN\n";
      print STDERR "outputfile=${inputfile}.$splitpfx-$idx.trans inputfile=${inputfile}.$splitpfx-$idx\n";
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
      chomp($inputN=`wc -l ${inputfile}.$splitpfx-$idx | cut -d' ' -f1`);
    }
    elsif ($inputtype==1){#confusion network input
      chomp($inputN=`cat ${inputfile}.$splitpfx-$idx | perl -pe 's/\\n/ _CNendline_ /g;' | perl -pe 's/_CNendline_  _CNendline_ /_CNendline_\\n/g;' | wc -l | cut -d' ' -f1 `);
    }
    elsif ($inputtype==2){#lattice input
      chomp($inputN=`wc -l ${inputfile}.$splitpfx-$idx | cut -d' ' -f1`);
    }
    else{#unknown input
      die "INPUTTYPE:$inputtype is unknown!\n";
    }
    chomp($outputN=`wc -l ${inputfile}.$splitpfx-$idx.trans | cut -d' ' -f1`);

    if ($inputN != $outputN){
      print STDERR "Split ($idx) were not entirely translated\n";
      print STDERR "outputN=$outputN inputN=$inputN\n";
      print STDERR "outputfile=${inputfile}.$splitpfx-$idx.trans inputfile=${inputfile}.$splitpfx-$idx\n";
      return 1;
    }
  }
  return 0; 
}

sub remove_temporary_files(){
  #removing temporary files
  foreach my $idx (@idxlist){
    unlink("${inputfile}.${splitpfx}-${idx}.trans");
    unlink("${inputfile}.${linespfx}-${idx}");
    unlink("${inputfile}.${timespfx}-${idx}");
    unlink("${inputfile}.${splitpfx}-${idx}");
    unlink("${inputfile}.${linespfx}-trans");
    unlink("${inputfile}.${linespfx}-times");
    if ($nbestflag){ unlink("${nbestfile}.${splitpfx}-${idx}"); }
    if ($searchgraphflag){ unlink("${searchgraphfile}.${splitpfx}-${idx}"); }
    if ($wordgraphflag){ unlink("${wordgraphfile}.${splitpfx}-${idx}"); }
    unlink("$qsubout-$idx");
    unlink("$qsuberr-$idx");
  }
  unlink("$qsubname.W.log");
  unlink("${jobscript}.bash");
  unlink("${jobscript}.log");
  rmdir("$tmpdir");
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
	chomp($a=`which pawd 2>/dev/null | head -1 | awk '{print $1}'`);
	if ($a && -e $a){	$pwdcmd=$a;	}
	return $pwdcmd;
}


sub calculateIdxRanges() {
    @idxstarts=();
    @idxends=();

    $idxranges=-1;

    foreach my $idx (@idx_todo) {

	if ($idxranges < 0) {
	    $idxranges += 1;
	    push(@idxstarts,$idx);
	    push(@idxends,$idx);
	} elsif ($idxends[$idxranges]+1==$idx) {
	    $idxends[$idxranges]=$idx;
	} else {
	    $idxranges += 1;
	    push(@idxstarts,$idx);
	    push(@idxends,$idx);
	}
    }
}
