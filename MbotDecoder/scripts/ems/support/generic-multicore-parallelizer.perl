#!/usr/bin/perl -w

use strict;

my $cores = 8;
my ($infile,$outfile,$cmd,$tmpdir);
my $parent = $$; 

use Getopt::Long qw(:config pass_through no_ignore_case);
GetOptions('cores=i' => \$cores,
	   'tmpdir=s' => \$tmpdir,
	   'in=s' => \$infile,
	   'out=s' => \$outfile,
	   'cmd=s' => \$cmd,
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
my $splitN = int(($sentenceN+$cores-0.5) / $cores); 
`split -a 2 -l $splitN $infile $tmpdir/in-$parent-`;

# find out the names of the processes
my @CORE=`ls $tmpdir/in-$parent-*`;
chomp(@CORE);
grep(s/.+in\-\d+\-([a-z]+)$/$1/e,@CORE);

# create core scripts
foreach my $core (@CORE){    
    open(BASH,">$tmpdir/core-$parent-$core.bash");
    print  BASH "#bash\n\n";
#    print  BASH "export PATH=$ENV{PATH}\n\n";
    printf BASH $cmd."\n", "$tmpdir/in-$parent-$core", "$tmpdir/out-$parent-$core";
    close(BASH);
}

# fork processes
my (@CHILDREN);
foreach my $core (@CORE){
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
