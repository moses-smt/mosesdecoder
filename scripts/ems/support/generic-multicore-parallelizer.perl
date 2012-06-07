#!/usr/bin/perl -w

use strict;

my $cores = 8;
my $serial = 1;
my ($infile,$outfile,$cmd,$tmpdir);
my $parent = $$; 

use Getopt::Long qw(:config pass_through no_ignore_case);
GetOptions('cores=i' => \$cores,
	   'tmpdir=s' => \$tmpdir,
	   'in=s' => \$infile,
	   'out=s' => \$outfile,
	   'cmd=s' => \$cmd,
     'serial=i' => \$serial
    ) or exit(1);

die("ERROR: specify command with -cmd") unless $cmd;
die("ERROR: specify infile with -in") unless $infile;
die("ERROR: specify outfile with -out") unless $outfile;
die("ERROR: did not find infile '$infile'") unless -e $infile;
die("ERROR: you need to specify a tempdir with -tmpdir") unless $tmpdir;
# set up directory
`mkdir -p $tmpdir`;

# create split input files
my $sentenceN = `cat $infile | wc -l`;
my $splitN = int(($sentenceN+($cores*$serial)-0.5) / ($cores*$serial)); 
print STDERR "split -a 3 -l $splitN $infile $tmpdir/in-$parent-\n";
`split -a 4 -l $splitN $infile $tmpdir/in-$parent-`;

# find out the names of the processes
my @CORE=`ls $tmpdir/in-$parent-*`;
chomp(@CORE);
grep(s/.+in\-\d+\-([a-z]+)$/$1/e,@CORE);

# create core scripts
for(my $i=0;$i<scalar(@CORE);$i++) {
    my $core = $CORE[$i];
    open(BASH,">$tmpdir/core-$parent-$core.bash") or die "Cannot open: $!";
    print  BASH "#bash\n\n";
#    print  BASH "export PATH=$ENV{PATH}\n\n";
    printf BASH $cmd."\n", "$tmpdir/in-$parent-$core", "$tmpdir/out-$parent-$core";
    for(my $j=2;$j<=$serial;$j++) {
      $core = $CORE[++$i];
      printf BASH $cmd."\n", "$tmpdir/in-$parent-$core", "$tmpdir/out-$parent-$core";
    }
    close(BASH);
}

# fork processes
my (@CHILDREN);
foreach my $core (@CORE){
    next unless -e "$tmpdir/core-$parent-$core.bash";
    my $child = fork();
    if (! $child) { # I am child
	print STDERR "running child $core\n";
	`bash $tmpdir/core-$parent-$core.bash 1> $tmpdir/core-$parent-$core.stdout 2> $tmpdir/core-$parent-$core.stderr`;
	exit 0;
    }
    push @CHILDREN,$child;
    print "adding child $core to children\n";
    sleep(1);
}

print "waiting on children\n";
foreach my $child (@CHILDREN) {
    waitpid( $child, 0 );
}
sleep(1);

# merge outfile
`rm -rf $outfile`;
foreach my $core (@CORE){
    `cat $tmpdir/out-$parent-$core >> $outfile`;
}
