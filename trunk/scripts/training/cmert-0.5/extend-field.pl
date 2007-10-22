#! /usr/bin/perl

sub PrintArgsAndDie () {
    print stderr "USAGE: enhanced-mert.pl [-h] \n";
    print stderr "This scripts extend the number of active fields for the mert procedure. (See the dual script reduce-field.pl)\n";
    exit(1);
}

my $weightfile="";
my $size=-1;
my $activefields="";

while (@ARGV){
    if ($ARGV[0] eq "-h"){
        &PrintArgsAndDie();
    }
    if ($ARGV[0] eq "-d"){
        $debug=1; 
	shift(@ARGV);
    }
    if ($ARGV[0] eq "-weight"){
        $weightfile=$ARGV[1];
        shift(@ARGV); shift(@ARGV);
    }
    if ($ARGV[0] eq "-d"){
        $size=$ARGV[1];
        shift(@ARGV); shift(@ARGV);
    }
    if ($ARGV[0] eq "-activate"){
        $activefields=$ARGV[1];
        shift(@ARGV);   shift(@ARGV);
    }

}

die "Cannot open/find weight file ($weightfile)\n" if ! -e $weightfile;

my @weight=();
open(IN,$weightfile);
chomp($weight=<IN>);
close(IN);
push @weight,(0,split(/[ \t]+/,$weight));
my @active=();
my @invertedactive=();

if ($activefields eq ""){
	for (my $i=1; $i<=$size; $i++){	$active[$i]=1; };
}else{
	@active=split(/,/,$activefields);
}

for (my $i=0; $i<=$size; $i++){	$invertedactive[$i]=0; };
for (my $i=0; $i<scalar(@active); $i++){	$invertedactive[$active[$i]]=1; };
my $j=0;
for (my $i=1; $i<=$size; $i++){	if (!$invertedactive[$i]){$notactive[$j]=$i; $j++}};

if ($debug>0){
	print STDERR "ORIGINAL SIZE: $size\n";
	print STDERR "ORIGINAL WEIGHTS: @weight\n";
	print STDERR "ORIGINAL ACTIVE: @active\n";
	print STDERR "ORIGINAL NOTACTIVE: @notactive\n";
	print STDERR "ORIGINAL INVERTEDACTIVE: @invertedactive\n";
}

while(chomp($_=<STDIN>)){
	@field=split(/[ \t]+/,$_);

	my $j=1;
	for (my $i=1; $i<=$size; $i++){	
		if ($invertedactive[$i]){
			print STDOUT "$field[$j] ";
			print STDERR "j:$j i:$i -> $field[$j]\n" if $debug>0;
			$j++;
		}else{
			printf STDOUT "%.6f ",$field[0]*$weight[$i];
			print STDERR "i:$i -> $field[0] $weight[$i]\n" if $debug>0;
		}
	};
	print STDOUT "\n";
}