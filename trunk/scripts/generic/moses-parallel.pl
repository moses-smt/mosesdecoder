#! /usr/bin/perl

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

#######################
#Customizable parameters 

#parameters for submiiting processes through Sun GridEngine
my $queueparameters="-l mem_free=0.5G -hard";

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
my $cfgfile=""; #configuration file

my $version=undef;
my $help=0;
my $dbg=0;
my $jobs=4;
my $mosescmd="$ENV{MOSESBIN}/moses"; #decoder in use
my $orifile=undef;
my $testfile=undef;
my $nbestfile=undef;
my $orinbestfile=undef;
my $nbest=undef;
my $nbestflag=0;
my $robust=1; # undef; # resubmit crashed jobs
my $orilogfile="";
my $logflag="";
my $qsubname="MOSES";
my $inputtype=0;
my $old_sge = 0; # assume old Sun Grid Engine (<6.0) where qsub does not
                 # implement -sync and -b

#######################
# Command line options processing
sub init(){
  use Getopt::Long qw(:config pass_through no_ignore_case);
  GetOptions('version'=>\$version,
	     'help'=>\$help,
	     'debug'=>\$dbg,
	     'jobs=i'=>\$jobs,
	     'decoder=s'=> \$mosescmd,
	     'robust' => \$robust,
       'decoder-parameters=s'=> \$mosesparameters,
			 'logfile=s'=> \$orilogfile,
	     'i|inputfile|input-file=s'=> \$orifile,
	     'n-best-file=s'=> \$orinbestfile,
	     'n-best-size=i'=> \$nbest,
	     'qsub-prefix=s'=> \$qsubname,
	     'queue-parameters=s'=> \$queueparameters,
	     'inputtype=i'=> \$inputtype,
       'config=s'=>\$cfgfile,
       'old-sge' => \$old_sge,
	    ) or exit(1);

  chomp($nbestfile=`basename $orinbestfile`) if defined $orinbestfile;
  chomp($testfile=`basename $orifile`) if defined $orifile;

  $mosesparameters.="@ARGV -config $cfgfile -inputtype $inputtype";
  getNbestParameters();
  getLogParameters();
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
  print STDERR "   -qsub-prefix <string> name for sumbitte jobs\n";
	print STDERR "   -queue-parameters <string> specific requirements for queue\n";
	print STDERR "   -old-sge Assume Sun Grid Engine < 6.0\n";
  print STDERR "   -debug debug\n";
  print STDERR "   -version print version of the script\n";
  print STDERR "   -help this help\n";
  print STDERR "Moses options:\n";
  print STDERR "   -inputtype <0|1> 0 for text, 1 for confusion networks\n";
  print STDERR "*  -config <cfgfile> configuration file\n";
  print STDERR "   -decoder-parameters <string> specific parameters for the decoder\n";
  print STDERR "All other options are passed to Moses\n";
  print STDERR "  (This way to pass parameters is maintained for back compatibility\n";
	print STDERR "   but preferably use -decoder-parameters)\n";
	exit(1);
}

#printparameters
sub print_parameters(){
  print STDERR "Inputfile: $orifile\n";
  print STDERR "Logfile: $orilogfile\n";
  print STDERR "Configuration file: $cfgfile\n";
  print STDERR "Decoder in use: $mosescmd\n";
  if ($nbestflag) {
    print STDERR "Nbest file: $orinbestfile\n"; 
    print STDERR "Nbest size: $nbest\n";
  }
  print STDERR "Number of jobs:$jobs\n";
  print STDERR "Qsub name: $qsubname\n";
	print STDERR "Queue parameters: $queueparameters\n";
	print STDERR "Inputtype: text\n" if $inputtype == 0;
  print STDERR "Inputtype: confusion network\n" if $inputtype == 1;
  
  print STDERR "parameters directly passed to Moses: $mosesparameters\n";
}

#get parameters for log file
sub getLogParameters(){
  $logflag=1 if $orilogfile;
}

#get parameters for nbest computation from configuration file
sub getNbestParameters(){
  if ($orinbestfile) {     $nbestflag=1;  }
  else{
    open (CFG, "$cfgfile");
    while (chomp($_=<CFG>)){
      if (/^\[n-best-list\]/){
	chomp($orinbestfile=<CFG>);
	chomp($nbest=<CFG>);
	$nbestflag=1;
	last;
      }
    }
    close(CFG);
  }
}

#######################
#Script starts here

init();

version() if $version;
usage() if $help;


if (!defined $orifile || !defined $mosescmd || ! defined $cfgfile) {
  print STDERR "Please specify -input-file, -decoder and -config\n";
  usage();
}

#checking if inputfile exists
if (! -e ${orifile} ){
  print STDERR "Inputfile ($orifile) does not exists\n";
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

if ($inputtype==0){ #text input
#getting the number of input sentences
  chomp($sentenceN=`wc -l ${orifile} | awk '{print \$1}' `);

#Reducing the number of jobs if less sentences to translate
  if ($jobs>$sentenceN){ $jobs=$sentenceN; }

#Computing the number of sentences for each files
  if ($sentenceN % $jobs == 0){ $splitN=int($sentenceN / $jobs); }
  else{ $splitN=int($sentenceN /$jobs) + 1; }

  if ($dbg){
    print STDERR "There are $sentenceN sentences to translate\n";
    print STDERR "There are at most $splitN sentences per job\n";
  }

  $cmd="split $decimal -a 2 -l $splitN $orifile ${testfile}.$splitpfx-";
  safesystem("$cmd") or die;
}
else{ #confusion network input
  my $tmpfile="/tmp/cnsplit$$";
  $cmd="cat $orifile | perl -pe 's/\\n/ _CNendline_ /g;' | perl -pe 's/_CNendline_  _CNendline_ /_CNendline_\\n/g;' > $tmpfile";
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
    safesystem("cat $tmpfile$idx | $cmd > ${testfile}.$splitpfx$idx ; rm $tmpfile$idx;");
  }
}

chomp(@idxlist=`ls ${testfile}.$splitpfx-*`);
grep(s/.+(\-\S+)$/$1/e,@idxlist);

safesystem("mkdir -p $tmpdir") or die;

preparing_script();

#launching process through the queue
my @sgepids =();

# if robust switch is used, redo jobs that crashed
my @idx_todo = ();
foreach (@idxlist) { push @idx_todo,$_; }

my $looped_once = 0;
while((!$robust && !$looped_once) || ($robust && scalar @idx_todo)) {
 $looped_once = 1;

 my $failure=0;
 foreach my $idx (@idx_todo){
  print STDERR "qsub $queueparameters -b no -j yes -o $qsubout$idx -e $qsuberr$idx -N $qsubname$idx ${jobscript}${idx}.bash\n" if $dbg; 

  $cmd="qsub $queueparameters -b no -j yes -o $qsubout$idx -e $qsuberr$idx -N $qsubname$idx ${jobscript}${idx}.bash >& ${jobscript}${idx}.log";

  safesystem($cmd) or die;

  my ($res,$id);

  open (IN,"${jobscript}${idx}.log")
    or die "Can't read id of job ${jobscript}${idx}.log";
  chomp($res=<IN>);
  split(/\s+/,$res);
  $id=$_[2];
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
  safesystem("rm -f $checkpointfile") or kill_all_and_quit();

  # start the 'hold' job, i.e. the job that will wait
  $cmd="qsub -cwd $queueparameters $hj -o $checkpointfile -e /dev/null -N $qsubname.W $syncscript >& $qsubname.W.log";
  safesystem($cmd) or kill_all_and_quit();
  
  # and wait for checkpoint file to appear
  my $nr=0;
  while (!-e $checkpointfile) {
    sleep(10);
    $nr++;
    print STDERR "w" if $nr % 3 == 0;
  }
  print STDERR "End of waiting.\n";
  safesystem("rm -f $checkpointfile $syncscript") or kill_all_and_quit();
  
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
  $cmd="qsub $queueparameters -sync y $hj -j y -o /dev/null -e /dev/null -N $qsubname.W -b y /bin/ls >& $qsubname.W.log";
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
concatenate_nbest() if $nbestflag;  


remove_temporary_files();


#script creation
sub preparing_script(){
  foreach my $idx (@idxlist){
    my $scriptheader="\#\! /bin/bash\n\n";
    $scriptheader.="uname -a\n\n";
    $scriptheader.="cd $workingdir\n\n";

    open (OUT, "> ${jobscript}${idx}.bash");
    print OUT $scriptheader;
    if ($nbestflag){
      print OUT "$mosescmd $mosesparameters -n-best-list $tmpdir/${nbestfile}.$splitpfx$idx $nbest -input-file ${testfile}.$splitpfx$idx > $tmpdir/${testfile}.$splitpfx$idx.trans\n\n";
      print OUT "echo exit status \$\?\n\n";

      print OUT "mv $tmpdir/${nbestfile}.$splitpfx$idx .\n\n";
      print OUT "echo exit status \$\?\n\n";
    }else{
      print OUT "$mosescmd $mosesparameters -input-file ${testfile}.$splitpfx$idx > $tmpdir/${testfile}.$splitpfx$idx.trans\n\n";
    }
    print OUT "mv $tmpdir/${testfile}.$splitpfx$idx.trans .\n\n";
    print OUT "echo exit status \$\?\n\n";
    close(OUT);

    #setting permissions of each script
    chmod(oct(755),"${jobscript}${idx}.bash");
  }
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

  open (OUT, "> ${orinbestfile}");
  foreach my $idx (@idxlist){

#computing the length of each input file
    my @in=();
    open (IN, "${testfile}.${splitpfx}${idx}.trans");
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

sub concatenate_1best(){
  foreach my $idx (@idxlist){
    my @in=();
    open (IN, "${testfile}.${splitpfx}${idx}.trans");
    @in=<IN>;
    print STDOUT "@in";
    close(IN);
  }
}

sub concatenate_logs(){
  open (OUT, "> ${orilogfile}");
  foreach my $idx (@idxlist){
    my @in=();
    open (IN, "$qsubout$idx");
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
      chomp($inputN=`wc -l ${testfile}.$splitpfx$idx | cut -d' ' -f1`);
    }
    else{
      chomp($inputN=`cat ${testfile}.$splitpfx$idx | perl -pe 's/\\n/ _CNendline_ /g;' | perl -pe 's/_CNendline_  _CNendline_ /_CNendline_\\n/g;' | wc -l | cut -d' ' -f1 `);
    }
    chomp($outputN=`wc -l ${testfile}.$splitpfx$idx.trans | cut -d' ' -f1`);
    
    if ($inputN != $outputN){
      print STDERR "Split ($idx) were not entirely translated\n";
      print STDERR "outputN=$outputN inputN=$inputN\n";
      print STDERR "outputfile=${testfile}.$splitpfx$idx.trans inputfile=${testfile}.$splitpfx$idx\n";
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
      chomp($inputN=`wc -l ${testfile}.$splitpfx$idx | cut -d' ' -f1`);
    }
    else{
      chomp($inputN=`cat ${testfile}.$splitpfx$idx | perl -pe 's/\\n/ _CNendline_ /g;' | perl -pe 's/_CNendline_  _CNendline_ /_CNendline_\\n/g;' | wc -l |
 cut -d' ' -f1 `);
    }
    chomp($outputN=`wc -l ${testfile}.$splitpfx$idx.trans | cut -d' ' -f1`);

    if ($inputN != $outputN){
      print STDERR "Split ($idx) were not entirely translated\n";
      print STDERR "outputN=$outputN inputN=$inputN\n";
      print STDERR "outputfile=${testfile}.$splitpfx$idx.trans inputfile=${testfile}.$splitpfx$idx\n";
      return 1;
    }
  }
  return 0; 
}

sub remove_temporary_files(){
  #removing temporary files
  foreach my $idx (@idxlist){
    unlink("${testfile}.${splitpfx}${idx}.trans");
    unlink("${testfile}.${splitpfx}${idx}");
    if ($nbestflag){      unlink("${nbestfile}.${splitpfx}${idx}");     }
    unlink("${jobscript}${idx}.bash");
    unlink("${jobscript}${idx}.log");
    unlink("$qsubname.W.log");
    unlink("$qsubout$idx");
    unlink("$qsuberr$idx");
    rmdir("$tmpdir");
  }
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

