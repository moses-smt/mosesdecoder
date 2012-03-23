#!/usr/bin/perl -w

use strict;

my $jobs = 20;
my ($infile,$outfile,$cmd,$tmpdir);

use Getopt::Long qw(:config pass_through no_ignore_case);
GetOptions('jobs=i' => \$jobs,
	   'tmpdir=s' => \$tmpdir,
	   'in=s' => \$infile,
	   'out=s' => \$outfile,
	   'cmd=s' => \$cmd,
       'queue-flags=s' => \$qflags,
	   ) or exit(1);

die("ERROR: specify infile with -in") unless $infile;
die("ERROR: specify outfile with -out") unless $outfile;
die("ERROR: did not find infile '$infile'") unless -e $infile;
die("ERROR: you need to specify a tempdir with -tmpdir") unless $tmpdir;
$qflags = "" unless $qflags;
# set up directory
`mkdir -p $tmpdir`;

# create split input files
my $sentenceN = `cat $infile | wc -l`;
my $splitN = int(($sentenceN+$jobs-0.5) / $jobs); 
`split -a 2 -l $splitN $infile $tmpdir/in-$$-`;

# find out the names of the jobs
my @JOB=`ls $tmpdir/in-$$-*`;
chomp(@JOB);
grep(s/.+in\-\d+\-([a-z]+)$/$1/e,@JOB);

# create job scripts
foreach my $job (@JOB){
    open(BASH,">$tmpdir/job-$$-$job.bash") or die "Cannot open: $!";
    print  BASH "#bash\n\n";
#    print  BASH "export PATH=$ENV{PATH}\n\n";
    printf BASH $cmd."\n", "$tmpdir/in-$$-$job", "$tmpdir/out-$$-$job";
    close(BASH);
}

# submit jobs
foreach my $job (@JOB){
    my $cmd = "qsub $qflags -b no -j yes "
	."-o $tmpdir/job-$$-$job.stdout "
	."-e $tmpdir/job-$$-$job.stderr "
	."-N J$$-$job "
	."$tmpdir/job-$$-$job.bash "
	.">& $tmpdir/job-$$-$job.log";
    print STDERR $cmd."\n";
    print STDERR `$cmd`;
}

# get qsub ID
my @QSUB_ID;
foreach my $job (@JOB){    
    `cat $tmpdir/job-$$-$job.log` =~ /Your job (\d+) /
	or die "ERROR: Can't read log of job $tmpdir/job-$$-$job.log";
    push @QSUB_ID,$1;
}

# create hold job and wait
my $hj = "-hold_jid " . join(" -hold_jid ", @QSUB_ID);
my $qsubname = "hold-$$";
$cmd="qsub $qflags  -sync y $hj -j y -o /dev/null -e /dev/null -N $qsubname.W -b y /bin/ls >& $tmpdir/$qsubname.W.log";
print $cmd."\n";
`$cmd`;

# merge outfile
`rm -rf $outfile`;
foreach my $job (@JOB){
    `cat $tmpdir/out-$$-$job >> $outfile`;
}
