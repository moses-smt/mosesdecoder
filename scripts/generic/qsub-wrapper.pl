#! /usr/bin/perl

use strict;

#######################
#Default parameters 
#parameters for submiiting processes through SGE
#NOTE: group name is ws06ossmt (with 2 's') and not ws06osmt (with 1 's')
my $queueparameters="-l ws06ossmt=true -l mem_free=0.5G";

my $workingdir=$ENV{PWD};
my $tmpdir="$workingdir/tmp$$";
my $jobscript="$workingdir/job$$";
my $qsubout="$workingdir/out.job$$";
my $qsuberr="$workingdir/err.job$$";


$SIG{INT} = \&kill_all_and_quit; # catch exception for CTRL-C

my $help="";
my $dbg="";
my $version="";
my $qsubname="WR$$";
my $cmd="";
my $cmdout="";
my $cmderr="";
my $parameters="";
my $old_sge = 0; # assume grid engine < 6.0

sub init(){
  use Getopt::Long qw(:config pass_through);
  GetOptions('version'=>\$version,
             'help'=>\$help,
             'debug'=>\$dbg,
             'qsub-prefix=s'=> \$qsubname,
             'command=s'=> \$cmd,
             'stdout=s'=> \$cmdout,
             'stderr=s'=> \$cmderr,
             'queue-parameter=s'=> \$queueparameters,
             'old-sge' => \$old_sge,
            ) or exit(1);
  $parameters="@ARGV";
  
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
  print STDERR "-command <file> command to run\n";
  print STDERR "-stdout <file> file to save stdout of cmd (optional)\n";
  print STDERR "-stderr <file> file to save stderr of cmd (optional)\n";
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
  print STDERR "command: $cmd\n";
  print STDERR "file for stdout: $cmdout\n";
  print STDERR "file for stderr: $cmderr\n";
  print STDERR "Qsub name: $qsubname\n";
  print STDERR "Queue parameters: $queueparameters\n";
  print STDERR "parameters directly passed to cmd: $parameters\n";
  exit(1);
}

#script creation
sub preparing_script(){
  my $scriptheader="\#\!/bin/csh\n\n";
  $scriptheader.="uname -a\n\n";

  $scriptheader.="cd $workingdir\n\n";
    
  open (OUT, "> ${jobscript}.csh");
  print OUT $scriptheader;

  print OUT "($cmd $parameters > $tmpdir/cmdout$$ ) >& $tmpdir/cmderr$$\n\n";
  print OUT "echo exit status \$\?\n\n";

  if ($cmdout){
    print OUT "mv $tmpdir/cmdout$$ $cmdout\n\n";
    print OUT "echo exit status \$\?\n\n";
  }
  else{
    print OUT "rm $tmpdir/cmdout$$\n\n";
    print OUT "echo exit status \$\?\n\n";
  }

  if ($cmderr){
    print OUT "mv $tmpdir/cmderr$$ $cmderr\n\n";
    print OUT "echo exit status \$\?\n\n";
  }
  else{
    print OUT "rm $tmpdir/cmderr$$\n\n";
    print OUT "echo exit status \$\?\n\n";
  }
  close(OUT);

  #setting permissions of each script
  chmod(oct(755),"${jobscript}.csh");
}

#######################
#Script starts here

init();

usage() if $cmd eq "";

safesystem("mkdir -p $tmpdir") or die;

preparing_script();

my $maysync = $old_sge ? "" : "-sync y";

# submit the main job
my $qsubcmd="qsub $queueparameters $maysync -o $qsubout -e $qsuberr -N $qsubname ${jobscript}.csh >& ${jobscript}.log";
safesystem($qsubcmd) or die;

#getting id of submitted job
my $res;
open (IN,"${jobscript}.log") or die "Can't read main job id: ${jobscript}.log";
chomp($res=<IN>);
split(/\s+/,$res);
my $id=$_[2];
close(IN);

print SDTERR " res:$res\n";
print SDTERR " id:$id\n";

if ($old_sge) {
  # need to workaround -sync, add another job that will wait for the main one
  # prepare a fake waiting script
  my $syncscript = "${jobscript}.sync_workaround_script.sh";
  safesystem("echo 'date' > $syncscript") or die;

  my $checkpointfile = "${jobscript}.sync_workaround_checkpoint";

  # ensure checkpoint does not exist
  safesystem("rm -f $checkpointfile") or die;

  # start the 'hold' job, i.e. the job that will wait
  $cmd="qsub -cwd $queueparameters -hold_jid $id -o $checkpointfile -e /dev/null -N $qsubname.W $syncscript >& $qsubname.W.log";
  safesystem($cmd) or die;
  
  # and wait for checkpoint file to appear
  my $nr=0;
  while (!-e $checkpointfile) {
    sleep(10);
    $nr++;
    print STDERR "w" if $nr % 3 == 0;
  }
  safesystem("rm -f $checkpointfile $syncscript") or die();
  print STDERR "End of waiting workaround.\n";
}


my $failure=&check_exit_status();
print STDERR "check_exit_status returned $failure\n";

&kill_all_and_quit() if $failure;

&remove_temporary_files() if !$dbg;

sub check_exit_status(){
  my $failure=0;

  print STDERR "check_exit_status of submitted job $id\n";
  open(IN,"$qsubout") or die "Can't read $qsubout";
  while (<IN>){
    $failure=1 if (/exit status 1/);
  }
  close(IN);
  return $failure;
}

sub kill_all_and_quit(){
  print STDERR "kill_all_and_quit\n";
  print STDERR "qdel $id\n";
  safesystem("qdel $id");

  print STDERR "The submitted jobs died not correctly\n";
  print STDERR "Send qdel signal to the submitted jobs\n";

  exit(1);
}

sub remove_temporary_files(){
  #removing temporary files

  unlink("${jobscript}.csh");
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

