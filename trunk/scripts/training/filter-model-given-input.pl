#!/usr/bin/perl -w

use strict;

my $MAX_LENGTH = 10;

my $dir = $ARGV[0]; &full_path(\$dir);
my $moses = $ARGV[1];
my $config = $ARGV[2];
my $input = $ARGV[3];
my $parameters = "";
for(my $i=4;$i<=$#ARGV;$i++) {
    $parameters .= " ".$ARGV[$i];
}

# special paramter: only filter, don't run Moses
my $norun = 0;
if ($parameters =~ /-norun/) {
    $parameters =~ s/-norun//;
    $norun = 1;
}

# buggy directory in place?
if (-e $dir && ! -e "$dir/info") {
    print STDERR "previous filter run crashed. delete $dir!\n";
    exit(1);
}

# already filtered? check if it can be re-used
if (-e $dir) {
    my @INFO = `cat $dir/info`;
    chop(@INFO);
    if($INFO[0] ne $config 
       || ($INFO[1] ne $input && 
	   $INFO[1].".tagged" ne $input)) {
	print STDERR "WARNING: directory does not match parameters: ($INFO[0] ne $config || $INFO[1] ne $input)\nI hope you know what you are doing...\n";
    }
    print STDERR "reusing cached filtered files\n";
}

# filtering the translation and distortion tables
else {
    print STDERR "mkdir -p $dir\n";
    `mkdir -p $dir`;

    # get tables to be filtered (and modify config file)
    my (@TABLE,@TABLE_FACTORS,@TABLE_NEW_NAME,%CONSIDER_FACTORS);
    open(INI_OUT,">$dir/moses.ini");
    open(INI,$config);
    while(<INI>) {
	print INI_OUT $_;
	if (/ttable-file\]/) {
	    while(1) {	       
		my $table_spec = <INI>;
		if ($table_spec !~ /^([\d\-]+) ([\d\-]+) (\d+) (\S+)$/) {
		    print INI_OUT $table_spec;
		    last;
		}
		my ($source_factor,$t,$w,$file) = ($1,$2,$3,$4);

		chomp($file);
		push @TABLE,$file;

		my $new_name = "$dir/phrase-table.$source_factor-$t";
		print INI_OUT "$source_factor $t $w $new_name\n";
		push @TABLE_NEW_NAME,$new_name;

		$CONSIDER_FACTORS{$source_factor} = 1;
		push @TABLE_FACTORS,$source_factor;
	    }
	}
	elsif (/distortion-file/) {
	    while(1) {
		my $table_spec = <INI>;
		if ($table_spec !~ /^([\d\-]+) ([\d\-]+) (\d+) (\S+)$/) {
		    print INI_OUT $table_spec;
		    last;
		}
		my ($source_factor,$t,$w,$file) = ($1,$2,$3,$4);

		chomp($file);
		push @TABLE,$file;

		$file =~ s/^.*\/+([^\/]+)/$1/g;
		my $new_name = "$dir/$file";
		print INI_OUT "$source_factor $t $w $new_name\n";
		push @TABLE_NEW_NAME,$new_name;

		$CONSIDER_FACTORS{$source_factor} = 1;
		push @TABLE_FACTORS,$source_factor;
	    }
	}
    }
    close(INI);
    close(INI_OUT);

#    # tables specified at the command line
#    if ($parameters =~ /^(.*)-distortion-file +(\S.*?) +(\-.+)$/ || 
#	$parameters =~ /^(.*)-distortion-file +(\S.*)()$/) {
#	my ($pre,$files,$post) = ($1,$2,$3);
#	@DISTORTION = ();
#	@DISTORTION_OUT = ();
#	foreach my $distortion (split(/ +/,$files)) {
#	    my $out = $distortion;
#	    $out =~ s/^.*\/+([^\/]+)/$1/g;
#	    push @DISTORTION,$distortion;
#	    push @DISTORTION_OUT,$out;
#	}
#    }
#    if ($parameters =~ /^(.*)-ttable-file +(\S.*?)( +\-.+)$/ || 
#	$parameters =~ /^(.*)-ttable-file +(\S+)()$/) {
#	$table = $2;
#    }

    # get the phrase pairs appearing in the input text
    my %PHRASE_USED;
    die("could not find input file $input") unless -e $input;
    open(INPUT,$input);
    while(my $line = <INPUT>) {
	chop($line);
	my @WORD = split(/ +/,$line);
	for(my $i=0;$i<=$#WORD;$i++) {
	    for(my $j=0;$j<$MAX_LENGTH && $j+$i<=$#WORD;$j++) {
		foreach (keys %CONSIDER_FACTORS) {
		    my @FACTOR = split(/,/);
		    my $phrase = "";
		    for(my $k=$i;$k<=$i+$j;$k++) {
			my @WORD_FACTOR = split(/\|/,$WORD[$k]);
			for(my $f=0;$f<=$#FACTOR;$f++) {
			    $phrase .= $WORD_FACTOR[$FACTOR[$f]]."|";
			}
			chop($phrase);
			$phrase .= " ";
		    }
		    chop($phrase);
		    $PHRASE_USED{$_}{$phrase}++;
		}
	    }
	}
    }
    close(INPUT);

    # filter files
    for(my $i=0;$i<=$#TABLE;$i++) {
	my ($used,$total) = (0,0);
	my $file = $TABLE[$i];
	my $factors = $TABLE_FACTORS[$i];
	my $new_file = $TABLE_NEW_NAME[$i];
	print STDERR "filtering $file -> $new_file...\n";

        if (-e $file && $file =~ /\.gz$/) { open(FILE,"zcat $file |"); }
        elsif (! -e $file && -e "$file.gz") { open(FILE,"zcat $file.gz|"); }
        elsif (-e $file) { open(FILE,$file); }
	else { die("could not find model file $file");  }

	open(FILE_OUT,">$new_file");

	while(my $entry = <FILE>) {
	    my ($foreign,$rest) = split(/ \|\|\| /,$entry,2);
	    $foreign =~ s/ $//;
	    if (defined($PHRASE_USED{$factors}{$foreign})) {
		print FILE_OUT $entry;
		$used++;
	    }
	    $total++;
	}
	close(FILE);
	close(FILE_OUT);
	printf STDERR "$used of $total phrases pairs used (%.2f%s) - note: max length $MAX_LENGTH\n",(100*$used/$total),'%';
    }

    open(INFO,">$dir/info");
    print INFO "$config\n$input\n";
    close(INFO);
}

# quit, if only filtering
exit if $norun;

# parameters specified at the command line
#if ($parameters =~ /^(.*)-distortion-file +(\S.*?)( +-.+)$/ || 
#    $parameters =~ /^(.*)-distortion-file +(\S.*)()$/) {
#    my ($pre,$files,$post) = ($1,$2,$3);
#    $parameters = "$pre -distortion-file ";
#    foreach my $distortion (split(/ +/,$files)) {
#	my $out = $distortion;
#	$out =~ s/^.*\/+([^\/]+)/$1/g;
#	$parameters .= "$dir/$out";
#    }
#    $parameters .= $post;
#}
#if ($parameters =~ /^(.*)-ttable-file +(\S+)( +-.+)$/ || 
#    $parameters =~ /^(.*)-ttable-file +(\S+)()$/) {
#    $parameters = $1.$3;
#}

print STDERR "$moses -f $dir/moses.ini $parameters < $input\n";
print `$moses -f $dir/moses.ini $parameters < $input`;

sub full_path {
    my ($PATH) = @_;
    return if $$PATH =~ /^\//;
    $$PATH = `pwd`."/".$$PATH;
    $$PATH =~ s/[\r\n]//g;
    $$PATH =~ s/\/\.\//\//g;
    $$PATH =~ s/\/+/\//g;
    my $sanity = 0;
    while($$PATH =~ /\/\.\.\// && $sanity++<10) {
        $$PATH =~ s/\/+/\//g;
        $$PATH =~ s/\/[^\/]+\/\.\.\//\//g;
    }
    $$PATH =~ s/\/[^\/]+\/\.\.$//;
    $$PATH =~ s/\/+$//;
}
