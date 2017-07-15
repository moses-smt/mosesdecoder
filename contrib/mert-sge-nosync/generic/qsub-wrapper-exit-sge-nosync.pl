#! /usr/bin/perl

# $Id$
use strict;
use Net::OpenSSH::Compat::Perl;
#######################
#Default parameters 
#parameters for submiiting processes through SGE
#NOTE: group name is ws06ossmt (with 2 's') and not ws06osmt (with 1 's')
my $queueparameters="";

# look for the correct pwdcmd 
my $pwdcmd = getPwdCmd();

my $workingdir = `$pwdcmd`; chomp $workingdir;
# my $tmpdir="$workingdir/tmp$$";
# my $jobscript="$workingdir/job$$.sh";
# my $qsubout="$workingdir/out.job$$";
# my $qsuberr="$workingdir/err.job$$";


$SIG{INT} = \&kill_all_and_quit; # catch exception for CTRL-C

my $submithost="";
my $help="";
my $dbg="";
my $version="";
my $qsubname="WR$$";
my $cmd="";
my $cmdout=undef;
my $cmderr=undef;
my $jid=0;
my $jidfile=undef;
my $pid=0;
my $pidfile=undef;
my $prevjid=undef;
my $parameters="";
my $old_sge = 0; # assume grid engine < 6.0
my $prevjidarraysize = 0;
my $force_delete = 0;
my @prevjidarray = ();

sub init(){
  use Getopt::Long qw(:config pass_through);
  GetOptions('version'=>\$version,
             'help'=>\$help,
             'debug'=>\$dbg,
             'submithost=s'=> \$submithost,
             'qsub-prefix=s'=> \$qsubname,
             'stdout=s'=> \$cmdout,
             'stderr=s'=> \$cmderr,
	     'jidfile=s'=> \$jidfile,
             'pidfile=s'=> \$pidfile,  # process id for previous job
             'prevjid=s'=> \$prevjid,
             'queue-parameter=s'=> \$queueparameters,
             'force-delete=i' => \$force_delete,
             'old-sge' => \$old_sge,
            ) or exit(1);
  $parameters="@ARGV";

  # read $pid from file
  chomp($pid=`tail -n 1 $pidfile`);
  # print "PID=+$pidfile+\n";

  if (defined $jidfile) {
    chomp($jid=`tail -n 1 $jidfile`);
  }

  # print STDERR "INPUT prevjid =+$prevjid+\n";
  $prevjid =~ s/^\s+|\s+$//g;
  # print STDERR "TRIMMED prevjid =+$prevjid+\n";
  
  @prevjidarray = split(/\s+/,$prevjid);
  $prevjidarraysize = scalar(@prevjidarray);
 
  # print STDERR "arraysize: $prevjidarraysize\n";
 
 
  version() if $version;
  usage() if $help;
  print_parameters() if $dbg;
}

#######################
##print version
sub version(){
#    print STDERR "version 1.0 (29-07-2006)\n";
    print STDERR "version 1.1 (31-07-2006)\n";
    exit(1);
}

#usage
sub usage(){
  print STDERR "qsub-wrapper.pl [options]\n";
  print STDERR "Options:\n";
  print STDERR "-stdout <file> file to find stdout from target cmd (optional)\n";
  print STDERR "-stderr <file> file to find stderr from target cmd (optional)\n";
  print STDERR "-jidfile <file> file to find the submit jobid (for submit option)\n";
  print STDERR "-pidfile <file> file to find the process id to the target job for deletion\n";
  print STDERR "-prevjid <id> wait for the previous job with jobid=id to finish before starting (optional)\n";
  print STDERR "-force-delete 1 force-delete without checking\n";
  print STDERR "-qsub-prefix <string> name for sumbitted jobs (optional)\n";
  print STDERR "-queue-parameters <string>  parameter for the queue (optional)\n";
  print STDERR "-old-sge ... assume Sun Grid Engine < 6.0\n";
  print STDERR "-debug debug\n";
  print STDERR "-version print version of the script\n";
  print STDERR "-help this help\n";
  exit(1);
}

#printparameters
sub print_parameters(){
  # print STDERR "command: $cmd\n";
  if (defined($cmdout)){ print STDERR "file for stdout: $cmdout\n"; }
  else { print STDERR "file for stdout is not defined, stdout is discarded\n"; }
  if (defined($cmderr)){ print STDERR "file for stdout: $cmderr\n"; }
  else { print STDERR "file for stderr is not defined, stderr is discarded\n"; }
  if (defined($jidfile)){ print STDERR "file for submit job id: $jidfile\n"; }
  else { print STDERR "file for submit job id is not defined, jidfile is discarded\n"; }
  print STDERR "Qsub name: $qsubname\n";
  print STDERR "Queue parameters: $queueparameters\n";
  print STDERR "parameters directly passed to cmd: $parameters\n";
  exit(1);
}


#######################
#Script starts here

init();

my $tmpdir="$workingdir/tmp$pid";
my $jobscript="$workingdir/job$pid.sh";
my $qsubout="$workingdir/out.job$pid";
my $qsuberr="$workingdir/err.job$pid";



### usage() if $cmd eq "";
###
### safesystem("mkdir -p $tmpdir") or die;
###
### preparing_script();
###
#### my $maysync = $old_sge ? "" : "-sync y";
#### never run in syn mode
###my $maysync = "";
###
###my $qsubcmd = "";
#### create the qsubcmd to submit to the queue with the parameter "-b yes"
####my $qsubcmd="qsub $queueparameters $maysync -V -o $qsubout -e $qsuberr -N $qsubname -b yes $jobscript > $jobscript.log 2>&1";
###
#### add -b yes if not yet defined, otherwise leave empty
###$queueparameters .= " -b yes " if (index($queueparameters," -b ")==-1);
###
###
###if (defined $prevjid && $prevjid!=-1 && $prevjidarraysize == 1) {
###    $qsubcmd="qsub $queueparameters $maysync -V -hold_jid $prevjid -o $qsubout -e $qsuberr -N $qsubname $jobscript > $jobscript.log 2>&1";
###} elsif (defined $prevjid && $prevjidarraysize > 1) {
###    my $hj = "-hold_jid " . join(" -hold_jid ", @prevjidarray);
###    # print STDERR "hj is $hj\n";
###    $qsubcmd="qsub $queueparameters $maysync -V $hj -o $qsubout -e $qsuberr -N $qsubname $jobscript > $jobscript.log 2>&1";    
###} else {
###  $qsubcmd="qsub $queueparameters $maysync -V -o $qsubout -e $qsuberr -N $qsubname $jobscript > $jobscript.log 2>&1";
###}
###
###print "submitting $qsubcmd\n";
###
####run the qsubcmd 
###
###safesystem($qsubcmd) or die;
###
####getting id of submitted job#############
###my $res;
###open (IN,"$jobscript.log") or die "Can't read main job id: $jobscript.log";
###chomp($res=<IN>);
###my @arrayStr = split(/\s+/,$res);
###my $id=$arrayStr[2];
###die "Failed to get job id from $jobscript.log, got: $res"
###  if $id !~ /^[0-9]+$/;
###close(IN);
############################################
###print STDERR " res:$res\n";
###print STDERR " id:$id\n";
###
###open (JIDOUT,">$jidfile") or die "Can't open jid file to write";
###print JIDOUT "$id\n";
###close(JIDOUT);
###
###open (JOBNUMOUT,">$jidfile.job") or die "Can't open id.job file to write";
###print JOBNUMOUT "$$\n";
###close(JOBNUMOUT);
###
###if ($old_sge) {
###  # need to workaround -sync, add another job that will wait for the main one
###  # prepare a fake waiting script
###  my $syncscript = "$jobscript.sync_workaround_script.sh";
###  safesystem("echo 'date' > $syncscript") or die;
###
###  my $checkpointfile = "$jobscript.sync_workaround_checkpoint";
###
###  # ensure checkpoint does not exist
###  safesystem("\\rm -f $checkpointfile") or die;
###
###  # start the 'hold' job, i.e. the job that will wait
####  $cmd="qsub -cwd $queueparameters -hold_jid $id -o $checkpointfile f -e /dev/null -N $qsubname.W $syncscript >& $qsubname.W.log";
###  $cmd="qsub -cwd $queueparameters -hold_jid $id -o $checkpointfile -e /dev/null -N $qsubname.W $syncscript >& $qsubname.W.log";
###  safesystem($cmd) or die;
###  
###  # and wait for checkpoint file to appear
###  my $nr=0;
###  while (!-e $checkpointfile) {
###    sleep(10);
###    $nr++;
###    print STDERR "w" if $nr % 3 == 0;
###  }
###  safesystem("\\rm -f $checkpointfile $syncscript") or die();
###  print STDERR "End of waiting workaround.\n";
###}


my $failure=0;


if (!$force_delete) {
  $failure=&check_exit_status();
  print STDERR "check_exit_status returned $failure\n";
}


&kill_all_and_quit() if $failure;

&remove_temporary_files() if !$dbg;




sub check_exit_status(){
  my $failure=0;

  print STDERR "check_exit_status of submitted job $jid from file $qsubout\n";
  open(IN,"$qsubout") or die "Can't read $qsubout";
  while (<IN>){
    $failure=1 if (/failed with exit status/);
  }
  close(IN);
  return $failure;
}

sub kill_all_and_quit(){
  my $my_username = undef;

  # chomp($my_username = `whoami`);  
  # 
  # my $ssh = Net::OpenSSH::Compat::Perl->new($submithost, debug=>0);
  # 
  # $ssh->login("$my_username",`cat /home/$my_username/accpw`);

  my $ssh = Net::OpenSSH::Compat::Perl->new($submithost, debug=>0);
  
  $ssh->login();

  print STDERR "kill_all_and_quit\n";
  print STDERR "qdel $jid\n";
  # safesystem("qdel $jid");
  $ssh->cmd("qdel $jid");

  print STDERR "The submitted jobs died not correctly\n";
  print STDERR "Send qdel signal to the submitted jobs\n";

  exit(1);
}

sub remove_temporary_files(){
  #removing temporary files

  unlink("${jobscript}");
  unlink("${jobscript}.log");
  unlink("$qsubout");
  unlink("$qsuberr");
  rmdir("$tmpdir");
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
    exit(1);
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

