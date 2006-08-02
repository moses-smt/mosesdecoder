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

#parameters for submiiting processes through SGE
#NOTE: group name is ws06ossmt (with 2 's') and not ws06osmt (with 1 's')
my $queueparameters="-l ws06ossmt=true -l mem_free=0.5G -hard";

my $workingdir=$ENV{PWD};
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

my $version="";
my $help="";
my $dbg="";
my $jobs=4;
my $mosescmd="$ENV{MOSESBIN}/moses"; #decoder in use
my $orifile="";
my $testfile="";
my $nbestfile="";
my $orinbestfile="";
my $nbest="";
my $nbestflag="";
my $qsubname="MOSES";
my $inputtype=0;

#######################
# Command line options processing
sub init(){
  use Getopt::Long qw(:config pass_through);
  GetOptions('version'=>\$version,
	     'help'=>\$help,
	     'debug'=>\$dbg,
	     'jobs=i'=>\$jobs,
	     'decoder=s'=> \$mosescmd,
	     'i|inputfile|input-file=s'=> \$orifile,
	     'n-best-file=s'=> \$orinbestfile,
	     'n-best-size=i'=> \$nbest,
	     'qsub-prefix=s'=> \$qsubname,
	     'queue-parameters=s'=> \$queueparameters,
	     'inputtype=i'=> \$inputtype,
             'config=s'=>\$cfgfile
	    ) or exit(1);

  chomp($nbestfile=`basename $orinbestfile`);
  chomp($testfile=`basename $orifile`);
  
  $mosesparameters="@ARGV -config $cfgfile -inputtype $inputtype";
  getNbestParameters();
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
    print STDERR "version 1.10 (02-08-2006)\n";
    exit(1);
}

#usage
sub usage(){
  print STDERR "moses-parallel.pl [parallel-options]  [moses-options]\n";
  print STDERR "Options marked (*) are required.\n";
  print STDERR "Parallel options:\n";
  print STDERR "*  -decoder <file> Moses decoder to use\n";
  print STDERR "   -i|inputfile|input-file <string> input file\n";
  print STDERR "   -inputfile <file>   the input text to translate\n";
  print STDERR "*  -jobs <N> number of required jobs\n";
  print STDERR "   -qsub-prefix <string> name for sumbitte jobs\n";
  print STDERR "   -queue-parameters <string> specific requirements for queue\n";
  print STDERR "   -debug debug\n";
  print STDERR "   -version print version of the script\n";
  print STDERR "   -help this help\n";
  print STDERR "Moses options:\n";
  print STDERR "   -inputtype <0|1> 0 for text, 1 for confusion networks\n";
  print STDERR "*  -config <cfgfile> configuration file\n";
  print STDERR "All other options are passed to Moses\n";
  exit(1);
}

#printparameters
sub print_parameters(){
  print STDERR "Inputfile: $orifile\n";
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
  print STDERR "Please specify -inputfile, -decoder and -config\n";
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

my $failure=0;
foreach my $idx (@idxlist){
  print STDERR "qsub $queueparameters -b no -j yes -o $qsubout$idx -e $qsuberr$idx -N $qsubname$idx ${jobscript}${idx}.bash\n" if $dbg; 

  $cmd="qsub $queueparameters -b no -j yes -o $qsubout$idx -e $qsuberr$idx -N $qsubname$idx ${jobscript}${idx}.bash >& ${jobscript}${idx}.log";

  safesystem($cmd) or die;

  my ($res,$id);

  open (IN,"${jobscript}${idx}.log");
  chomp($res=<IN>);
  split(/\s+/,$res);
  $id=$_[2];
  close(IN);

  push @sgepids, $id;
}

#waiting until all jobs have finished
my $hj = "-hold_jid " . join(" -hold_jid ", @sgepids);

$cmd="qsub $queueparameters -sync yes $hj -j yes -o /dev/null -e /dev/null -N $qsubname.W -b yes /bin/ls >& $qsubname.W.log";
safesystem($cmd) or kill_all_and_quit();

$failure=&check_exit_status();

kill_all_and_quit() if $failure;

check_translation();

#concatenating translations and removing temporary files
concatenate_1best();
if ($nbestflag){  concatenate_nbest();  }

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
      print OUT "$mosescmd $mosesparameters -n-best-list $tmpdir/${nbestfile}.$splitpfx$idx $nbest -i ${testfile}.$splitpfx$idx > $tmpdir/${testfile}.$splitpfx$idx.trans\n\n";
      print OUT "echo exit status \$\?\n\n";
      print OUT "mv $tmpdir/${nbestfile}.$splitpfx$idx .\n\n";
      print OUT "echo exit status \$\?\n\n";
    }else{
      print OUT "$mosescmd $mosesparameters -i ${testfile}.$splitpfx$idx > $tmpdir/${testfile}.$splitpfx$idx.trans\n\n";
    }
    print OUT "mv $tmpdir/${testfile}.$splitpfx$idx.trans .\n\n";
    print OUT "echo exit status \$\?\n\n";
    close(OUT);
  }
}



sub concatenate_nbest(){
  my $oldcode="";
  my $newcode=-1;
  open (OUT, "> ${orinbestfile}");
  foreach my $idx (@idxlist){
    open (IN, "${nbestfile}.${splitpfx}${idx}");
    while (<IN>){
      my ($code,@extra)=split(/\|\|\|/,$_);
      $newcode++ if $code ne $oldcode;
      $oldcode=$code;
      print OUT join("\|\|\|",($newcode,@extra));
    }
    close(IN);
    $oldcode="";
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
  print STDERR "Any of the submitted jobs died not correctly\n";
  print STDERR "qdel function was called for all submitted jobs\n";

  exit(1);
}


sub check_translation(){
  #checking if all sentences were translated
  my $inputN;
  my $outputN;
  foreach my $idx (@idxlist){
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
      exit(1);
    }
  }
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
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}

