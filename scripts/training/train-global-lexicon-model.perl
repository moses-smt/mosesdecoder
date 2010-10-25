#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";

### paths

# MegaM --- available from http://www.cs.utah.edu/~hal/megam/
my $megam = "/home/pkoehn/statmt/bin/megam_i686.opt";

# temporary dir - you may need something bigger
my $tmpdir = "/tmp";

# input parameters
my ($corpus_stem,$lex_dir,$f,$e,$model);

die("syntax: train-global-lexicon-model.perl --corpus-stem FILESTEM --lex-dir DIR --f EXT --e EXT --model FILE [--tmp-dir DIR]\n")
    unless &GetOptions('corpus-stem=s' => \$corpus_stem,
		       'lex-dir=s' => \$lex_dir,
		       'f=s' => \$f,
		       'e=s' => \$e,
		       'model=s' => \$model,
                       'tmp-dir=s' => \$tmpdir)
    && defined($e) && defined($f)
    && defined($lex_dir) && -e "$lex_dir/$f.vcb" && -e "$lex_dir/$e.vcb"
    && defined($corpus_stem)
    && (    (-e "$corpus_stem.$e"    && -e "$corpus_stem.$f") 
	 || (-e "$corpus_stem.$e.gz" && -e "$corpus_stem.$f.gz"));

# read lexicon index
my (%LEX_E,%LEX_F,%DELEX_E,%DELEX_F,%COUNT_E,%COUNT_F);
&read_lex("$lex_dir/$f.vcb",\%LEX_F,\%DELEX_F,\%COUNT_F);
&read_lex("$lex_dir/$e.vcb",\%LEX_E,\%DELEX_E,\%COUNT_E);

# specified word? do just that one
if (defined($ARGV[0])) {
  my $ew = $ARGV[0];
  &process_one_word(&decode_word($ew));
  exit;
}

### cluster code --- needs to be adapted
if (`hostname` =~ /centaur/) {
  my $i=0;
  foreach my $ew (keys %LEX_E) {
    #next unless $COUNT_E{$ew}<=1;
    next if $ew eq "UNK";
    my $file = &efile($ew);
    my $jobfile = $file;
    $jobfile =~ s/(\/.\/.\/.\/)/$1job-/;
    next if -e $file || -e $file.".model";
    #next if -e $jobfile;
    open(SH,">$jobfile.bash");
    print SH "#!/bin/bash\n/home/pkoehn/experiment/disc-lex/train-discriminative-lexicon.perl ".&encode_word($ew)."\n";
    close(SH);
    print "qsub -e $jobfile.err -o $jobfile.out $jobfile.bash\n";
    `qsub -e $jobfile.err -o $jobfile.out $jobfile.bash`;
    exit if $i++ > 8000;
  }
  exit;
}

### process all words
foreach my $ew (keys %LEX_E) {
  next if $ew eq "UNK";
  my $file = &efile($ew);
  next if -e $file;
  &process_one_word($ew);
}
&consolidate();

### sub: process one word
sub process_one_word {
  my ($ew) = @_;

  print "$ew\n";
  my $file = &efile($ew);
  `touch $file`;

  # find out which foreign words co-occur with $ew
  my %COOC;
  if (-e "$corpus_stem.$f.gz") {
      open(F,"zcat $corpus_stem.$f.gz|");
      open(E,"zcat $corpus_stem.$e.gz|");
  }
  else {
      open(F,"$corpus_stem.$f");
      open(E,"$corpus_stem.$e");
  }
  while(my $f = <F>) {
    my $e = <E>;
    chop($e); chop($f);

    my $has_e = 0;
    foreach (split(/ /,$e)) {
      $has_e++ if $_ eq $ew;
    }
    next unless $has_e;

    foreach my $fw (split(/ /,$f)) {
      $COOC{$fw}++;
    }
  }
  print "\tcoocurs with ".(scalar keys %COOC)." foreign words.\n";
  close(E);
  close(F);

  # create training file
  print "\tfile $file\n";
  open(EW,">$file");
  if (-e "$corpus_stem.$f.gz") {
      open(F,"zcat $corpus_stem.$f.gz|");
      open(E,"zcat $corpus_stem.$e.gz|");
  }
  else {
      open(F,"$corpus_stem.$f");
      open(E,"$corpus_stem.$e");
  }
  while(my $f = <F>) {
    my $e = <E>;
    chop($e); chop($f);

    my $has_e = 0;
    foreach (split(/ /,$e)) {
      $has_e = 1 if $_ eq $ew;
    }
    my %HAS_E;
    print EW $has_e;

    my %ALREADY_F;
    foreach my $fw (split(/ /,$f)) {
      next unless defined($COOC{$fw});
      #$fw = "_nocooc_" unless defined($COOC{$fw});
      next if defined($ALREADY_F{$fw});
      $ALREADY_F{$fw}++;
      print EW " F$LEX_F{$fw}";
    }
    print EW "\n";
  }
  close(EW);

  # run training 
  `$megam -maxi 100 binary $file 1>$file.model 2>$file.log`;
  `rm $file`;
  close(E);
  close(F);
}

sub consolidate {
  open(MODEL,">$model");
  open(LEVEL1,"ls $tmpdir|");
  while (my $dir1 = <LEVEL1>) {
    chop($dir1);
    open(LEVEL2,"ls $tmpdir/$dir1|");
    while (my $dir2 = <LEVEL2>) {
      chop($dir2);
      open(LEVEL3,"ls $tmpdir/$dir1/$dir2|");
      while (my $dir3 = <LEVEL3>) {
        chop($dir3);
        open(DIR,"ls $tmpdir/$dir1/$dir2/$dir3|");
        while(my $file = <DIR>) {
          chop($file);
          next unless $file =~ /^(.+).model$/;
          my $word = $1;
          &decode_word(\$word);
          &consolidate_file("$tmpdir/$dir1/$dir2/$dir3/$file",$word);
        }
        close(DIR);
      }
      close(LEVEL3);
    }
    close(LEVEL2);
  }
  close(LEVEL1);
  close(MODEL);
}

sub consolidate_file {
  my($file,$word) = @_;
  print $file."\n";
  open(FILE,$file);
  while(<FILE>) {
    chomp;
    my($feature,$weight) = split;
    die("$file: can't resolve feature $feature") if $feature =~ /F(\d+)/ && !defined($DELEX_F{$1});
    $feature = $DELEX_F{$1} if $feature =~ /F(\d+)/ && defined($DELEX_F{$1});
    print MODEL "$word $feature $weight\n";
  }
  close(FILE);
}

sub efile {
  my ($word) = @_;
  my $first  = &code_letter(substr($word."___",0,1));
  my $second = &code_letter(substr($word."___",1,1));
  my $third  = &code_letter(substr($word."___",2,1));
  `mkdir -p $tmpdir/$first/$second/$third`;
  &encode_word(\$word);
  return "$tmpdir/$first/$second/$third/".&encode_word($word);
}

sub encode_word {
  my ($word) = @_;
  $word =~ s/_/_0/g;
  $word =~ s/\\/_1/g;
  $word =~ s/\//_2/g;
  $word =~ s/\./_3/g;
  $word =~ s/\'/_4/g;
  $word =~ s/\"/_5/g;
  $word =~ s/\?/_6/g;
  $word =~ s/\*/_7/g;
  $word =~ s/\!/_8/g;
  $word =~ s/\&/_9/g;
  $word =~ s/\</_a/g;
  $word =~ s/\>/_b/g;
  $word =~ s/\$/_c/g;
  $word =~ s/\[/_d/g;
  $word =~ s/\]/_e/g;
  $word =~ s/\(/_f/g;
  $word =~ s/\)/_g/g;
  $word =~ s/\{/_h/g;
  $word =~ s/\}/_i/g;
  $word =~ s/\|/_j/g;
  $word =~ s/\;/_k/g;
  $word =~ s/\:/_l/g;
  $word =~ s/\`/_m/g;
  $word =~ s/\~/_n/g;
  $word =~ s/\@/_o/g;
  $word =~ s/\,/_p/g;
  $word =~ s/\#/_q/g;
  return $word;
}

sub decode_word {
  my ($word) = @_;
  $word =~ s/_1/\\/g;
  $word =~ s/_2/\//g;
  $word =~ s/_3/\./g;
  $word =~ s/_4/\'/g;
  $word =~ s/_5/\"/g;
  $word =~ s/_6/\?/g;
  $word =~ s/_7/\*/g;
  $word =~ s/_8/\!/g;
  $word =~ s/_9/\&/g;
  $word =~ s/_a/\</g;
  $word =~ s/_b/\>/g;
  $word =~ s/_c/\$/g;
  $word =~ s/_d/\[/g;
  $word =~ s/_e/\]/g;
  $word =~ s/_f/\(/g;
  $word =~ s/_g/\)/g;
  $word =~ s/_h/\{/g;
  $word =~ s/_i/\}/g;
  $word =~ s/_j/\|/g;
  $word =~ s/_k/\;/g;
  $word =~ s/_l/\:/g;
  $word =~ s/_m/\`/g;
  $word =~ s/_n/\~/g;
  $word =~ s/_o/\@/g;
  $word =~ s/_p/\,/g;
  $word =~ s/_q/\#/g;
  $word =~ s/_0/_/g;
  return $word;
}

sub code_letter {
  my ($letter) = @_;
  $letter = "_" unless $letter =~ /[a-z0-1]/i;
  $letter =~ tr/A-Z/a-z/;
  return $letter;
}

sub read_lex {
  my ($file,$LEX,$DELEX,$COUNT) = @_;
  open(LEX,$file);
  while(<LEX>) {
    chop;
    my ($id,$word,$count) = split;
    $$LEX{$word} = $id;
    $$DELEX{$id} = $word;
    $$COUNT{$word} = $count;
  }
  close(LEX);
}
