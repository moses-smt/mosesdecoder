#!/usr/bin/perl -w

# read arguments
my $countFile = $ARGV[0];

my $ZCAT = "gzip -cd";
my $BZCAT = "bzcat";

&process_count_file($countFile);

sub process_count_file {
    $file = $_[0];
    open(COUNT_READER, &open_compressed($file)) or die "ERROR: Can't read $file";

    print STDERR "reading file to calculate normalizer";
    $normalizer=0;
    while(<COUNT_READER>) {
        my $line = $_;
        chomp($line);
        my @line_array = split(/\s+/, $line);
        my $count = $line_array[0];
        $normalizer+=$count;
    }

    close(COUNT_READER);

    print STDERR "reading file again to print the counts";
    open(COUNT_READER, &open_compressed($file)) or die "ERROR: Can't read $file";

    while(<COUNT_READER>) {
        my $line = $_;
        chomp($line);
        my @line_array = split(/\s+/, $line);
        my $score = $line_array[0]/$normalizer;
        print $score."\n";
    }

    close(COUNT_READER);
}

sub open_compressed {
    my ($file) = @_;
    print STDERR "FILE: $file\n";

    # add extensions, if necessary
    $file = $file.".bz2" if ! -e $file && -e $file.".bz2";
    $file = $file.".gz"  if ! -e $file && -e $file.".gz";

    # pipe zipped, if necessary
    return "$BZCAT $file|" if $file =~ /\.bz2$/;
    return "$ZCAT $file|"  if $file =~ /\.gz$/;
    return $file;
}
