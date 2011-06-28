#!/usr/bin/perl
# Uses Google AJAX API to collect many translations, i.e. create a parallel
# corpus of Google translations.
# Expects one sentence per line, not tokenized!
#
# Ondrej Bojar, bojar@ufal.mff.cuni.cz

use strict;
use Getopt::Long;
use CGI;
use JSON;
use HTTP::Request;
use HTTP::Headers;
use LWP::UserAgent;
use Data::Dumper;

my $shucks = 0;
sub catch_zap {
    my $signame = shift;
    $shucks++;
    print STDERR "Somebody sent me a SIG$signame, will exit.\n";
}
$SIG{INT} = \&catch_zap;  # best strategy


my $srclang = "en";
my $tgtlang = "cs";
my $batchlimit = 560; # how many characters can be translated at once
my $skip_long_sentences = 0;
    # provide empty string for sentences beyond batchlimit
my $sleep = 5;
my $inpad = " My_SpE. "; # sentence delimiter
my $outpad = " ?My_SpE. ?"; # sentence delimiter as returned by Google
my $verbose = 0;
my $require_fullstop_to_join = 1; # avoid joining sentences if not ended by a
                                  # full stop

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");
binmode(STDERR, ":utf8");

GetOptions(
  "srclang=s" => \$srclang,
  "tgtlang=s" => \$tgtlang,
  "sleep=s" => \$sleep, # fractional number of seconds to sleep
  "inpad=s" => \$inpad,
  "outpad=s" => \$outpad,
  "skip-long-sentences" => \$skip_long_sentences,
  "require-fullstop-to-join" => \$require_fullstop_to_join,
) or exit 1;

my @fnames = @ARGV;

sub microsleep {
  select(undef,undef,undef,$_[0]);
}

# debugging translation
 # my $outlines = translate_batch(\@fnames);
 # print $_."\n" while ($_ = shift @$outlines);
 # exit 1;

if (scalar @fnames == 0) {
  print STDERR "Suggested usage:\n";
  print STDERR "  nohup ./get_many_translations.pl infile1 outfile1\n";
  print STDERR "      [infile2.gz outfile2.gz ...] > log &\n";
  print STDERR "Use ctrl-C to interrupt at any time.\n";
  print STDERR "Restart with the same input and output files to continue.\n";
  exit 1;
}

my $skipped = 0;
while (0 < scalar @fnames) {
  last if $shucks;
  my $infile = shift @fnames;
  my $outfile = shift @fnames;
  collect_translations($infile, $outfile);
  # finalize the output file, if compressed
  if ($outfile =~ /\.(gz|bz2)$/) {
    print STDERR "Recompressing $outfile\n";
    *INF = my_open($outfile);
    my @lines = <INF>;
    close INF;
    rename $outfile, $outfile."~tmpbkup"
      or die "Failed to backup $outfile before finalizing.";
    *OUTF = my_append($outfile);
    print OUTF $_ while ($_ = shift @lines);
    close OUTF;
    unlink $outfile."~tmpbkup";
  }
}
print STDERR "Done. Skipped $skipped sentences.\n";

sub collect_translations {
  my $infile = shift;
  my $outfile = shift;

  while (1) {
    last if $shucks;
    # infinite loop, until everything translated
    my $gotlines = wcl($outfile);
    print STDERR "$outfile contains $gotlines lines already, extending.\n";
  
    my $nr = 0;
    my @inlines = ();
    my $droplast = 0;
    *INF = my_open($infile);
    while (<INF>) {
      $nr++;
      if (length(join($inpad, @inlines)) > $batchlimit) {
        # don't read any further
        $droplast = 1;
        last;
      }
      # extend the current batch of sentences
      if ($nr > $gotlines) {
        chomp;
        push @inlines, $_;
      }
      # don't read any further if not a full stop
      last if 0 < scalar(@inlines)
        && $inlines[-1] !~ /\.\s*$/ && $require_fullstop_to_join;
    }
    if (length(join($inpad, @inlines)) > $batchlimit) {
      # an additional test, necessary at the very end of the file
      $droplast = 1;
    }
    close INF;
    if (0 == scalar @inlines) {
      print STDERR "No more input lines in $infile.\n";
      return;
    }
    if ($droplast) {
      my $skippedtext = pop @inlines;
        # don't translate the line exceeding batch limit

      if (0==scalar @inlines) {
        $nr--;
        if ($skip_long_sentences) {
          print STDERR "$infile:$nr:SKIPPING too long sentence: $skippedtext\n";
          $skipped++;
        } else {
          die "$infile:$nr:Line exceeds Google batch limit!";
        }
      }
    }
  
    my $outlines;
    
    if (0 == scalar @inlines) {
      # special case: skipping too long sentences
      $outlines = [""];
    } else {
      $outlines = translate_batch(\@inlines);
      last if !defined $outlines;
    }
  
    *OUTF = my_append($outfile);
    foreach my $outline (@$outlines) {
      print OUTF $outline."\n";
    }
    close OUTF;
  }
}

sub wcl {
  my $f = shift;
  my $gotlines = 0;
  if (-e $f) {
    *PEEKF = my_open($f);
    $gotlines ++ while (<PEEKF>);
    close PEEKF;
  }
  return $gotlines;
}

sub translate_batch {
  my $inlines = shift;
  my @outlines = ();

  my $responsestr = single_query(join($inpad, @$inlines));
  my $response = from_json($responsestr, {utf8=>1});

  my $translated_text = $response->{"responseData"}->{"translatedText"};

  # special treatment of final empty sentences in the batch
  my $finblanks = 0;
  while ($translated_text =~ /$outpad$/) {
    $finblanks ++;
    $translated_text =~ s/$outpad$//;
  }
  # main split:
  my @outlines = split /$outpad/, $translated_text;
  push @outlines, ( map {""} (1..$finblanks) );   # add final blank lines

  if (scalar @$inlines != scalar @outlines) {
    print STDERR "Input lines:\n";
    map {print STDERR $_."\n"} @$inlines;
    print STDERR "\nOutput text:\n$translated_text\n\n";
    print STDERR "Details:\n".Dumper($response)."\n\n";
    print STDERR "Mismatched number of sentences! Expected ".(scalar @$inlines)
      ." got ".(scalar @outlines)."\n";
    return undef;
  }

  # unescape what google escapes
  @outlines = map { s/&quot;/"/g; s/&#39;/'/g;
                    s/&lt;/</g; s/&gt;/>/g;
                    s/&amp;/&/g;
                    $_ } @outlines;

  return \@outlines;
}

sub single_query {
  my $intext = shift;
  my $querytext = CGI::escape($intext);
  print STDERR "Req: $querytext\n" if $verbose;
  # debugging offline:
  # return '{"responseData": {"translatedText":"Ciao mondo. Ahoj. Nazdar."}, "responseDetails": null, "responseStatus": 200}';
  my $headers = HTTP::Headers->new;
  $headers->referer('http://ufal.mff.cuni.cz/~bojar/translate-czeng-by-google.html');
  my $request = HTTP::Request->new("GET",
    "http://ajax.googleapis.com/ajax/services/language/translate?v=1.0&q=$querytext&langpair=$srclang%7C$tgtlang",
    $headers);
  my $ua = LWP::UserAgent->new;
  print STDERR "Requesting translation...\n" if $verbose;
  microsleep($sleep);
  my $response = $ua->request($request);
  if ($response->is_success) {
    my $text = $response->content();
    return $text;
  } else {
    print STDERR "Req: $querytext\n";
    die "Failed to get translations: ".$response->status_line;
  }
}

sub my_open {
  my $f = shift;
  die "Not found: $f" if ! -e $f;

  my $opn;
  my $hdl;
  my $ft = `file $f`;
  # file might not recognize some files!
  if ($f =~ /\.gz$/ || $ft =~ /gzip compressed data/) {
    $opn = "zcat $f |";
  } elsif ($f =~ /\.bz2$/ || $ft =~ /bzip2 compressed data/) {
    $opn = "bzcat $f |";
  } else {
    $opn = "$f";
  }
  open $hdl, $opn or die "Can't open '$opn': $!";
  binmode $hdl, ":utf8";
  return $hdl;
}

sub my_append {
  my $f = shift;

  my $opn;
  my $hdl;
  # file might not recognize some files!
  if ($f =~ /\.gz$/) {
    $opn = "| gzip -c >> $f";
  } elsif ($f =~ /\.bz2$/) {
    $opn = "| bzip2 >> $f";
  } else {
    $opn = ">> $f";
  }
  open $hdl, $opn or die "Can't append '$opn': $!";
  binmode $hdl, ":utf8";
  return $hdl;
}
