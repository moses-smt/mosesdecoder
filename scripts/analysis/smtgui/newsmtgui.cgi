#!/usr/bin/perl -w
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id$
use strict;

use CGI;
use Corpus; #Evan's code
use Error qw(:try);

#files with extensions other than these are interpreted as system translations; see the file 'file-descriptions', if it exists, for the comments that go with them
my %FILETYPE = ('e' => 'Reference Translation',
		'f' => 'Foreign Original',
		'ref.sgm' => 'Reference Translations',
		'e.sgm' => 'Reference Translations',
		'src.sgm' => 'Foreign Originals',
		'f.sgm' => 'Foreign Originals');
my %DONTSCORE = ('f' => 1, 'f.sgm' => 1, 'src.sgm' => 1,
		 'e' => 1, 'e.sgm' => 1, 'ref.sgm' => 1);
my @SHOW = ('f', 'e', 'comm');
my %SHOW_COLOR = ('f' => "BLUE",
		  'e' => "GREEN");
my $FOREIGN = 'f';

#FILEDESC: textual descriptions associated with specific filenames; to be displayed on the single-corpus view
my %FILEDESC = (); &load_descriptions();
my %factorData = loadFactorData('file-factors');
my %MEMORY;        &load_memory();
my (@mBLEU,@NIST);
@mBLEU=`cat mbleu-memory.dat` if -e "mbleu-memory.dat"; chop(@mBLEU);
@NIST = `cat nist-memory.dat` if -e "nist-memory.dat"; chop(@NIST);
my %in;            &ReadParse(); #parse arguments

if (scalar(@ARGV) > 0 && $ARGV[0] eq 'bleu') {
  $in{CORPUS} = $ARGV[1];
  $in{ACTION} = "VIEW_CORPUS";
}

my %MULTI_REF;
if ($in{CORPUS} && -e "$in{CORPUS}.ref.sgm") {
  my $sysid;
  open(REF,"$in{CORPUS}.ref.sgm");
  while(<REF>) {
    $sysid = $1 if /<DOC.+sysid=\"([^\"]+)\"/;
    if (/<seg[^>]*> *(\S.+\S) *<\/seg>/) {
      push @{$MULTI_REF{$sysid}}, $1;
    }
  }
  close(REF);
}

if ($in{ACTION} eq '') { &show_corpora(); }
elsif ($in{ACTION} eq 'VIEW_CORPUS') { &view_corpus(); }
elsif ($in{ACTION} eq 'SCORE_FILE') { &score_file(); }
elsif ($in{ACTION} eq 'RESCORE_FILE') { &score_file(); }
elsif ($in{ACTION} eq 'COMPARE') { &compare(); }
else { &htmlhead("Unknown Action $in{ACTION}"); }
print "</BODY></HTML>\n";

###### SHOW CORPORA IN EVALUATION DIRECTORY

sub show_corpora {
  my %CORPUS = ();

  # find corpora in evaluation directory: see the factor-index file, which was already read in
  foreach my $corpusName (keys %factorData)
  {
  	$CORPUS{$corpusName} = 1;
  }

  # list corpora
  &htmlhead("All Corpora");
  print "<UL>\n";
  foreach (sort (keys %CORPUS)) {
    print "<LI><A HREF=\"?ACTION=VIEW_CORPUS&CORPUS=".CGI::escape($_)."\">Corpus $_</A>\n";
  }
  print "</UL>\n";
}

###### SHOW INFORMATION FOR ONE CORPUS

sub view_corpus {
  my @TABLE;
  &htmlhead("View Corpus $in{CORPUS}");

  # find corpora in evaluation directory
  my $corpus = new Corpus('-name' => "$in{CORPUS}", '-descriptions' => \%FILEDESC, '-info_line' => $factorData{$in{CORPUS}});
#  $corpus->printDetails(); #debugging info

  my ($sentence_count, $lineInfo);
  if(-e "$in{CORPUS}.f")
  {
  	$lineInfo = `wc -l $in{CORPUS}.f`;
  	$lineInfo =~ /^\s*(\d+)\s+/;
  	$sentence_count = 0 + $1;
	}
	else
	{
	  $lineInfo = `wc -l $in{CORPUS}.e`;
	  $lineInfo =~ /^\s*(\d+)\s+/;
	  $sentence_count = 0 + $1;
	}

  print "Corpus '$in{CORPUS}' consists of $sentence_count sentences\n";
  print "(<A HREF=?ACTION=VIEW_CORPUS&CORPUS=" . CGI::escape($in{CORPUS})."&mBLEU=1>with mBLEU</A>)" if ((!defined($in{mBLEU})) && (scalar keys %MEMORY) && -e "$in{CORPUS}.e" && -e "$in{CORPUS}.f");
  print "<P>\n";
  print "<FORM ACTION=''>\n";
  print "<INPUT TYPE=HIDDEN NAME=ACTION VALUE=COMPARE>\n";
  print "<INPUT TYPE=HIDDEN NAME=CORPUS VALUE=\"$in{CORPUS}\">\n";
  print "<TABLE BORDER=1 CELLSPACING=0><TR>
<TD>File (<A HREF=?ACTION=VIEW_CORPUS&CORPUS=" . CGI::escape($in{CORPUS}).">sort</A>)</TD>
<TD>Date (<A HREF=?ACTION=VIEW_CORPUS&CORPUS=" . CGI::escape($in{CORPUS})."&SORT=TIME>sort</A>)</TD>";
  if (-e "$in{CORPUS}.e") {
    print "<TD>IBM BLEU (<A HREF=?ACTION=VIEW_CORPUS&CORPUS=" . CGI::escape($in{CORPUS})."&SORT=IBM>sort</A>)</TD>";
  }
  if (-e "$in{CORPUS}.ref.sgm" && -e "$in{CORPUS}.src.sgm") {
    print "<TD>NIST (<A HREF=?ACTION=VIEW_CORPUS&CORPUS=" . CGI::escape($in{CORPUS})."&SORT=NIST>sort</A>)</TD>";
    if (! -e "$in{CORPUS}.e") {
      print "<TD>BLEU (<A HREF=?ACTION=VIEW_CORPUS&CORPUS=" . CGI::escape($in{CORPUS})."&SORT=BLEU>sort</A>)</TD>";
    }
  }
  if ($in{mBLEU} && (scalar keys %MEMORY) && -e "$in{CORPUS}.e" && -e "$in{CORPUS}.f") {
    print "<TD>mBLEU (<A HREF=?ACTION=VIEW_CORPUS&CORPUS=" . CGI::escape($in{CORPUS})."&SORT=mBLEU>sort</A>)</TD>";
  }
  print "<TD>Unknown Words</TD>"; #can't sort on; only applies to the input
  print "<TD>Perplexity</TD>"; #applies to truth and system outputs
  print "<TD>WER (<A HREF=?ACTION=VIEW_CORPUS&CORPUS=" . CGI::escape($in{CORPUS})."&SORT=WER>sort</A>)</TD>";
  print "<TD>Noun & adj WER-PWER</TD>"; #can't sort on; only applies to sysoutputs
  print "<TD>Surface vs. lemma PWER</TD>"; #can't sort on; only applies to sysoutputs
	print "<TD>Statistical Measures</TD>";

  opendir(DIR, ".") or die "couldn't open '.' for read";
  my @filenames = readdir(DIR); #includes . and ..
  closedir(DIR);
  foreach $_ (@filenames)
  {
  	next if -d $_; #if is a directory
    my $sgm = 0;
    if (/.sgm$/)
	 {
	 	`grep '<seg' $_ | wc -l` =~ /^\s*(\d+)\s+/;
		next unless $1 == $sentence_count;
		$sgm = 1;
    }
    else
	 {
	 	`wc -l $_` =~ /^\s*(\d+)\s+/;
		next unless $1 == $sentence_count;
    }
	 next unless /^$in{CORPUS}\.([^\/]+)$/;
    my $file = $1;
	 my $sort = "";
    # checkbox for compare
    my $row = "<TR><TD style=\"font-size: small\"><INPUT TYPE=CHECKBOX NAME=FILE_$file VALUE=1>";
    # README
    if (-e "$in{CORPUS}.$file.README") {
      my $readme = `cat $in{CORPUS}.$file.README`;
      $readme =~ s/([\"\'])/\\\"/g;
      $readme =~ s/[\n\r]/\\n/g;
      $readme =~ s/\t/\\t/g;
      $row .= "<A HREF='javascript:FieldInfo(\"$in{CORPUS}.$file\",\"$readme\")'>";
    }
    # filename
    $row .= "$file</A>";
    # description (hard-coded)
    my @TRANSLATION_SENTENCE = `cat $in{CORPUS}.$file`;
    chop(@TRANSLATION_SENTENCE);

	 #count sentences that contain null words
	 my $null_count = 0;
    foreach (@TRANSLATION_SENTENCE)
	 {
      $null_count++ if /^NULL$/ || /^NONE$/;
    }
    if ($null_count > 0) {
      $row .= "$null_count NULL ";
    }

    $row .= " (".$FILETYPE{$file}.")" if defined($FILETYPE{$file});
    $row .= " (".$FILEDESC{$in{CORPUS}.".".$file}.")" if defined($FILEDESC{$in{CORPUS}.".".$file});
    $row .= " (".$FILEDESC{$file}.")" if defined($FILEDESC{$file});
    # filedate
    my @STAT = stat("$in{CORPUS}.$file");
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($STAT[8]); #STAT[8] should be last modify time
    my $time = sprintf("%04d-%02d-%02d %02d:%02d:%02d",$year+1900,$mon+1,$mday,$hour,$min,$sec);
    $row .= "</TD>\n<TD>".$time."</TD>\n";
    if (defined($in{SORT}) && $in{SORT} eq 'TIME') { $sort = $time; }
    # IBM BLEU score
    my $no_bleu =0;
    if (!$sgm && -e "$in{CORPUS}.e") {
      $row .= "<TD>";
      if (!defined($DONTSCORE{$file}) && $file !~ /^f$/ && $file ne "e" && $file !~ /^pt/) {
	my ($score,$p1,$p2,$p3,$p4,$bp) = $corpus->calcBLEU($file, 'surf');
	print STDERR "193: `$score `$p1 `$p2 `$p3 `$p4 `$bp\n";
	$row .= sprintf("<B>%.04f</B> %.01f/%.01f/%.01f/%.01f *%.03f", $score, $p1, $p2, $p3, $p4, $bp);
	if (defined($in{SORT}) && $in{SORT} eq 'IBM') { $sort = $score; }
      }
      $row .= "</TD>\n";
    }
    else {
      $no_bleu=1;
    }
    # NIST score
    if (-e "$in{CORPUS}.ref.sgm" && -e "$in{CORPUS}.src.sgm"
	&& !$DONTSCORE{$file}) {
      $row .= "<TD>";
      print "$DONTSCORE{$file}+";
      my ($nist,$nist_bleu);
      if ($file =~ /sgm$/) {
	($nist,$nist_bleu) = get_nist_score("$in{CORPUS}.ref.sgm","$in{CORPUS}.src.sgm","$in{CORPUS}.$file");
	$row .= sprintf("<B>%.04f</B>",$nist);
	if ($in{SORT} eq 'NIST') { $sort = $nist; }
      }
      $row .= "</TD>\n";
      if ($no_bleu) {
	$row .= "<TD>";
	if ($file =~ /sgm$/) {
	  $row .= sprintf("<B>%.04f</B>",$nist_bleu);
	  if ($in{SORT} eq 'BLEU') { $sort = $nist_bleu; }
	}
	$row .= "</TD>\n";
      }
    }
    # multi-bleu
    if ($in{mBLEU} && (scalar keys %MEMORY) && -e "$in{CORPUS}.e") {
      $row .= "<TD>";
      if (!defined($DONTSCORE{$file}) && $file !~ /^f$/ && $file ne "e") {
	my ($score,$p1,$p2,$p3,$p4,$bp) = get_multi_bleu_score("$in{CORPUS}.f","$in{CORPUS}.e","$in{CORPUS}.$file");
	$row .= sprintf("<B>%.04f</B> %.01f/%.01f/%.01f/%.01f *%.03f",$score,$p1,$p2,$p3,$p4,$bp);
	if ($in{SORT} eq 'mBLEU') { $sort = $score; }
      }
      $row .= "</TD>\n";
    }

	 my $isSystemOutput = ($file ne 'e' && $file ne 'f' && $file !~ /^pt/);
	 # misc stats (note the unknown words should come first so the total word count is available for WER)
	 $row .= "<TD align=\"center\">";
	 if($file eq 'f') #input
	 {
	 	try
		{
			my ($unknownCount, $totalCount) = calc_unknown_words($corpus, 'surf');
	 		$row .= sprintf("%.4lf (%d / %d)", $unknownCount / $totalCount, $unknownCount, $totalCount);
		}
		catch Error::Simple with {$row .= "[system error]";};
	 }
	 $row .= "</TD>\n<TD align=\"center\">";
	 if($file eq 'e' || $file eq 'f' || $isSystemOutput)
	 {
	 	try
		{
			my $perplexity = $corpus->calcPerplexity(($file eq 'e') ? 'truth' : (($file eq 'f') ? 'input' : $file), 'surf');
			$row .= sprintf("%.2lf", $perplexity);
		}
		catch Error::Simple with {$row .= "[system error]";}
	 }
	 $row .= "</TD>\n<TD align=\"center\">";
	 if($isSystemOutput)
	 {
	 	try
		{
			my $surfaceWER = $corpus->calcOverallWER($file);
			$row .= sprintf("%.4lf", $surfaceWER);
		}
		catch Error::Simple with {$row .= "[system error]";};
	 }
	 $row .= "</TD>\n<TD align=\"center\">";
	 my ($nnAdjWER, $nnAdjPWER, $surfPWER, $lemmaPWER);
	 if($isSystemOutput)
	 {
		try
		{
			($nnAdjWER, $nnAdjPWER, $surfPWER, $lemmaPWER) = calc_misc_stats($corpus, $file);
			$row .= sprintf("WER = %.4lg<br>PWER = %.4lg<br><b>ratio = %.3lf</b>", $nnAdjWER, $nnAdjPWER, $nnAdjPWER / $nnAdjWER);
		}
		catch Error::Simple with {$row .= "[system error]";};
	}
	$row .= "</TD>\n<TD align=\"center\">";
	if($isSystemOutput)
	{
		if($surfPWER == -1)
		{
			$row .= "[system error]";
		}
		else
		{
			my ($lemmaBLEU, $p1, $p2, $p3, $p4, $brevity) = $corpus->calcBLEU($file, 'lemma');
			$row .= sprintf("surface = %.3lf<br>lemma = %.3lf<br><b>lemma BLEU = %.04f</b> %.01f/%.01f/%.01f/%.01f *%.03f",
									$surfPWER, $lemmaPWER, $lemmaBLEU, $p1, $p2, $p3, $p4, $brevity);
		}
	}
	$row .= "</TD>\n<TD align=\"center\">";
	if($isSystemOutput)
	{
		try
		{
			my $testInfo = $corpus->statisticallyTestBLEUResults($file, 'surf');
			my @tTestPValues = @{$testInfo->[0]};
			my @confidenceIntervals = @{$testInfo->[1]};
			$row .= "n-gram precision p-values (high p <=> consistent score):<br>t test " . join("/", map {sprintf("%.4lf", $_)} @tTestPValues);
			$row .= "<p>n-gram precision 95% intervals:<br>" . join(",<br>", map {sprintf("[%.4lf - %.4lf]", $_->[0], $_->[1])} @confidenceIntervals);
			my @bleuInterval = (approxBLEUFromNgramScores(map {$_->[0]} @confidenceIntervals), approxBLEUFromNgramScores(map {$_->[1]} @confidenceIntervals));
			$row .= sprintf("<br><b>(BLEU: ~[%.4lf - %.4lf])</b>", $bleuInterval[0], $bleuInterval[1]);
		}
		catch Error::Simple with {$row .= "[system error]";}
	}
	$row .= "</TD>\n";

    # correct sentence score
    my($correct,$wrong,$unknown);
    $row .= "<TD>";
    if (!defined($DONTSCORE{$file}) && (scalar keys %MEMORY)) {
      my ($correct,$just_syn,$just_sem,$wrong,$unknown) = get_score_from_memory("$in{CORPUS}.$FOREIGN",
			       "$in{CORPUS}.$file");
      $row .= "<B><FONT COLOR=GREEN>$correct</FONT></B>";
      $row .= "/<FONT COLOR=ORANGE>$just_syn</FONT>";
      $row .= "/<FONT COLOR=ORANGE>$just_sem</FONT>";
      $row .= "/<FONT COLOR=RED>$wrong</FONT> ($unknown)</TD>\n";
      if ($in{SORT} eq 'SCORE') {
	$sort = sprintf("%03d %04d",$correct,$just_syn+$just_sem);
      }
    }
	 else
	 {
	 	$row .= "</TD>\n";
	}

    $row .= "</TR>\n";
    push @TABLE, "<!-- $sort -->\n$row";
  }
  close(DIR);
  foreach (reverse sort @TABLE) { print $_; }
  print "</TABLE>\n";
  print "<INPUT TYPE=SUBMIT VALUE=\"Compare\">\n";
  print "<INPUT TYPE=CHECKBOX NAME=SURFACE VALUE=1 CHECKED> Compare all different sentences (instead of just differently <I>evaluated</I> sentences) <INPUT TYPE=CHECKBOX NAME=WITH_EVAL VALUE=1 CHECKED> with evaluation</FORM><P>\n";
  print "<P>The score is to be read as: <FONT COLOR=GREEN>correct</FONT>/<FONT COLOR=ORANGE>just-syn-correct</FONT>/<FONT COLOR=ORANGE>just-sem-correct</FONT>/<FONT COLOR=RED>wrong</FONT> (unscored)\n";
  print "<BR>IBM BLEU is to be read as: <B>metric</B> unigram/bigram/trigram/quadgram *brevity-penalty<P>";
  print "<DIV STYLE=\"border: 1px solid #006600\">";
  print "<H2>Comparison of System Translations (p-values)</H2>";
  my @sysnames = $corpus->getSystemNames();
  for(my $i = 0; $i < scalar(@sysnames); $i++)
  {
  	for(my $j = $i + 1; $j < scalar(@sysnames); $j++)
	{
		my $comparison = $corpus->statisticallyCompareSystemResults($sysnames[$i], $sysnames[$j], 'surf');
		print "<P><FONT COLOR=#00aa22>" . $sysnames[$i] . " vs. " . $sysnames[$j] . "</FONT>: [<I>t</I> test] ";
		for(my $k = 0; $k < scalar(@{$comparison->[0]}); $k++)
		{
			print sprintf(($k == 0) ? "%.4lg" : "; %.4lg ", $comparison->[0]->[$k]);
			if($comparison->[1]->[$k] == 0) {print "(&larr;)";} else {print "(&rarr;)";}
		}
		print "&nbsp;&nbsp;---&nbsp;&nbsp;[sign test] ";
		for(my $k = 0; $k < scalar(@{$comparison->[2]}); $k++)
		{
			print sprintf(($k == 0) ? "%.4lg " : "; %.4lg ", $comparison->[2]->[$k]);
			if($comparison->[3]->[$k] == 0) {print "(&larr;)";} else {print "(&rarr;)";}
		}
		print "\n";
	}
  }
  print "</DIV\n";
  print "<P><A HREF=\"newsmtgui.cgi?action=\">All corpora</A>\n";
}

###### SCORE TRANSLATIONS

sub score_file {
  if ($in{VIEW}) {
    &htmlhead("View Translations");
  }
  else {
    &htmlhead("Score Translations");
  }
  print "<A HREF=\"?ACTION=VIEW_CORPUS&CORPUS=".CGI::escape($in{CORPUS})."\">View Corpus $in{CORPUS}</A><P>\n";
  print "<FORM ACTION=\"\" METHOD=POST>\n";
  print "<INPUT TYPE=HIDDEN NAME=ACTION VALUE=$in{ACTION}>\n";
  print "<INPUT TYPE=HIDDEN NAME=CORPUS VALUE=\"$in{CORPUS}\">\n";
  print "<INPUT TYPE=HIDDEN NAME=FILE VALUE=\"$in{FILE}\">\n";

  # get sentences
  my @SENTENCES;
  if ($in{FILE} =~ /.sgm$/) {
      @SENTENCES = `grep '<seg' $in{CORPUS}.$in{FILE}`;
      for(my $i=0;$i<$#SENTENCES;$i++) {
	  $SENTENCES[$i] =~ s/^<seg[^>]+> *(\S.+\S) *<\/seg> *$/$1/;
      }
  }
  else {
      @SENTENCES = `cat $in{CORPUS}.$in{FILE}`; chop(@SENTENCES);
  }

  my %REFERENCE;
  foreach (@SHOW) {
    if (-e "$in{CORPUS}.$_") {
      @{$REFERENCE{$_}} = `cat $in{CORPUS}.$_`; chop(@{$REFERENCE{$_}});
    }
  }

  # update memory
  foreach (keys %in) {
    next unless /^SYN_SCORE_(\d+)$/;
    next unless $in{"SEM_SCORE_$1"};
    &store_in_memory($REFERENCE{$FOREIGN}[$1],
		     $SENTENCES[$1],
		     "syn_".$in{"SYN_SCORE_$1"}." sem_".$in{"SEM_SCORE_$1"});
  }

  # display sentences
  for(my $i=0;$i<=$#SENTENCES;$i++) {
    my $evaluation = &get_from_memory($REFERENCE{$FOREIGN}[$i],$SENTENCES[$i]);
    next if ($in{ACTION} eq 'SCORE_FILE' &&
	     ! $in{VIEW} &&
	     $evaluation ne '' && $evaluation ne 'wrong');
    print "<P>Sentence ".($i+1).":<BR>\n";
    # color coding
    &color_highlight_ngrams($i,&nist_normalize_text($SENTENCES[$i]),$REFERENCE{"e"}[$i]);
    if (%MULTI_REF) {
	foreach my $sysid (keys %MULTI_REF) {
	    print "<FONT COLOR=GREEN>".$MULTI_REF{$sysid}[$i]."</FONT> (Reference $sysid)<BR>\n";
	}
    }

    # all sentences
    print "$SENTENCES[$i] (System output)<BR>\n";
    foreach my $ref (@SHOW) {
      if (-e "$in{CORPUS}.$ref") {
	print "<FONT COLOR=$SHOW_COLOR{$ref}>".$REFERENCE{$ref}[$i]."</FONT> (".$FILETYPE{$ref}.")<BR>\n" if $REFERENCE{$ref}[$i];
      }
    }
    if (! $in{VIEW}) {
      print "<INPUT TYPE=RADIO NAME=SYN_SCORE_$i VALUE=correct";
      print " CHECKED" if ($evaluation =~ /syn_correct/);
      print "> perfect English\n";
      print "<INPUT TYPE=RADIO NAME=SYN_SCORE_$i VALUE=wrong";
      print " CHECKED" if ($evaluation =~ /syn_wrong/);
      print "> imperfect English<BR>\n";
      print "<INPUT TYPE=RADIO NAME=SEM_SCORE_$i VALUE=correct";
      print " CHECKED" if ($evaluation =~ /sem_correct/);
      print "> correct meaning\n";
      print "<INPUT TYPE=RADIO NAME=SEM_SCORE_$i VALUE=wrong";
      print " CHECKED" if ($evaluation =~ /sem_wrong/);
      print "> incorrect meaning\n";
    }
  }
  if (! $in{VIEW}) {
    print "<P><INPUT TYPE=SUBMIT VALUE=\"Add evaluation\">\n";
    print "</FORM>\n";
  }
}

sub color_highlight_ngrams {
  my($i,$sentence,$single_reference) = @_;
  my @REF = ();
  my %NGRAM = ();
  if (%MULTI_REF) {
    foreach my $sysid (keys %MULTI_REF) {
      push @REF,&nist_normalize_text($MULTI_REF{$sysid}[$i]);
    }
  }
  elsif ($single_reference) {
    @REF = ($single_reference);
  }
  if (@REF) {
    foreach my $ref (@REF) {
      my @WORD = split(/\s+/,$ref);
      for(my $n=1;$n<=4;$n++) {
	for(my $w=0;$w<=$#WORD-($n-1);$w++) {
	  my $ngram = "$n: ";
	  for(my $j=0;$j<$n;$j++) {
	    $ngram .= $WORD[$w+$j]." ";
	  }
	  $NGRAM{$ngram}++;
	}
      }
    }
    $sentence =~ s/^\s+//;
    $sentence =~ s/\s+/ /;
    $sentence =~ s/\s+$//;
    my @WORD = split(/\s+/,$sentence);
    my @CORRECT;
    for(my $w=0;$w<=$#WORD;$w++) {
      $CORRECT[$w] = 0;
    }
    for(my $n=1;$n<=4;$n++) {
      for(my $w=0;$w<=$#WORD-($n-1);$w++) {
	my $ngram = "$n: ";
	for(my $j=0;$j<$n;$j++) {
	  $ngram .= $WORD[$w+$j]." ";
	}
	next unless defined($NGRAM{$ngram}) && $NGRAM{$ngram}>0;
	$NGRAM{$ngram}--;
	for(my $j=0;$j<$n;$j++) {
	  $CORRECT[$w+$j] = $n;
	}
      }
    }
    my @COLOR;
    $COLOR[0] = "#FF0000";
    $COLOR[1] = "#C000C0";
    $COLOR[2] = "#0000FF";
    $COLOR[3] = "#00C0C0";
    $COLOR[4] = "#00C000";
    for(my $w=0;$w<=$#WORD;$w++) {
      print "<B><FONT COLOR=".$COLOR[$CORRECT[$w]].">$WORD[$w]<SUB>".$CORRECT[$w]."</SUB></FONT></B> ";
    }
    print "\n<BR>";
  }
}

###### OTHER STATS

#print (in some unspecified way) the offending exception of type Error::Simple
#arguments: the error object, a context string
#return: none
sub printError
{
	my ($err, $context) = @_;
	warn "$context: " . $err->{'-text'} . " @ " . $err->{'-file'} . " (" .$err->{'-line'} . ")\n";
}

#compute number and percentage of unknown tokens for a given factor in foreign corpus
#arguments: corpus object ref, factor name
#return (unkwordCount, totalWordCount), or (-1, -1) if an error occurs
sub calc_unknown_words
{
	my ($corpus, $factorName) = @_;
	try
	{
		my ($unknownCount, $totalCount) = $corpus->calcUnknownTokens($factorName);
		return ($unknownCount, $totalCount);
	}
	catch Error::Simple with
	{
		my $err = shift;
		printError($err, 'calc_unknown_words()');
		return (-1, -1);
	};
}

#compute (if we have the necessary factors) info for:
#- diff btwn wer and pwer for NNs & ADJs -- if large, many reordering errors
#- diff btwn pwer for surface forms and pwer for lemmas -- if large, morphology errors
#arguments: corpus object, system name
#return (NN/ADJ (wer, pwer), surf pwer, lemma pwer), or (-1, -1, -1, -1) if an error occurs
sub calc_misc_stats
{
	my ($corpus, $sysname) = @_;
	try
	{
		my ($nnAdjWER, $nnAdjPWER) = $corpus->calcNounAdjWER_PWERDiff($sysname);
		my ($surfPWER, $lemmaPWER) = ($corpus->calcOverallPWER($sysname, 'surf'), $corpus->calcOverallPWER($sysname, 'lemma'));
		return ($nnAdjWER, $nnAdjPWER, $surfPWER, $lemmaPWER);
	}
	catch Error::Simple with
	{
		my $err = shift;
		printError($err, 'calc_misc_stats()');
		return (-1, -1, -1, -1);
	};
}

#approximate BLEU score from n-gram precisions (currently assume no length penalty)
#arguments: n-gram precisions as an array
#return: BLEU score
sub approxBLEUFromNgramScores
{
	my $logsum = 0;
	foreach my $p (@_) {$logsum += log($p);}
	return exp($logsum / scalar(@_));
}

###### NIST SCORE

sub get_nist_score {
  my($reference_file,$source_file,$translation_file) = @_;
  my @STAT = stat($translation_file);
  my $current_timestamp = $STAT[9];
  foreach (@NIST) {
    my ($file,$time,$nist,$bleu) = split;
    return ($nist,$bleu)
      if ($file eq $translation_file && $current_timestamp == $time);
  }

  my $nist_eval = `/home/pkoehn/statmt/bin/mteval-v10.pl -c -r $reference_file -s $source_file -t $translation_file`;
  return (0,0) unless ($nist_eval =~ /NIST score = (\d+\.\d+)  BLEU score = (\d+\.\d+)/i);

  open(NIST,">>nist-memory.dat");
  printf NIST "$translation_file $current_timestamp %f %f\n",$1,$2;
  close(NIST);
  return ($1,$2);
}

sub nist_normalize_text {
    my ($norm_text) = @_;

# language-independent part:
    $norm_text =~ s/<skipped>//g; # strip "skipped" tags
    $norm_text =~ s/-\n//g; # strip end-of-line hyphenation and join lines
    $norm_text =~ s/\n/ /g; # join lines
    $norm_text =~ s/(\d)\s+(\d)/$1$2/g; #join digits
    $norm_text =~ s/&quot;/"/g;  # convert SGML tag for quote to "
    $norm_text =~ s/&amp;/&/g;   # convert SGML tag for ampersand to &
    $norm_text =~ s/&lt;/</g;    # convert SGML tag for less-than to >
    $norm_text =~ s/&gt;/>/g;    # convert SGML tag for greater-than to <

# language-dependent part (assuming Western languages):
    $norm_text = " $norm_text ";
#    $norm_text =~ tr/[A-Z]/[a-z]/ unless $preserve_case;
    $norm_text =~ s/([\{-\~\[-\` -\&\(-\+\:-\@\/])/ $1 /g;   # tokenize punctuation
    $norm_text =~ s/([^0-9])([\.,])/$1 $2 /g; # tokenize period and comma unless preceded by a digit
    $norm_text =~ s/([\.,])([^0-9])/ $1 $2/g; # tokenize period and comma unless followed by a digit
    $norm_text =~ s/([0-9])(-)/$1 $2 /g; # tokenize dash when preceded by a digit
    $norm_text =~ s/\s+/ /g; # one space only between words
    $norm_text =~ s/^\s+//;  # no leading space
    $norm_text =~ s/\s+$//;  # no trailing space

    return $norm_text;
}

###### BLEU SCORE

sub get_multi_bleu_score {
  my($foreign_file,$reference_file,$translation_file) = @_;
  my @STAT = stat($translation_file);
  my $current_timestamp = $STAT[9];
  foreach (@mBLEU) {
    my ($file,$time,$score,$g1,$g2,$g3,$g4,$bp) = split;
    if ($file eq $translation_file && $current_timestamp == $time) {
      return ($score,$g1*100,$g2*100,$g3*100,$g4*100,$bp);
    }
  }

  # load reference translation from reference file
  my @REFERENCE_SENTENCE = `cat $reference_file`; chop(@REFERENCE_SENTENCE);
  my @TRANSLATION_SENTENCE = `cat $translation_file`; chop(@TRANSLATION_SENTENCE);
  my %REF;
  my @FOREIGN_SENTENCE = `cat $foreign_file`; chop(@FOREIGN_SENTENCE);
  for(my $i=0;$i<=$#TRANSLATION_SENTENCE;$i++) {
    push @{$REF{$FOREIGN_SENTENCE[$i]}},$REFERENCE_SENTENCE[$i];
  }
  # load reference translation from translation memory
  foreach my $memory (keys %MEMORY) {
    next if $MEMORY{$memory} ne 'syn_correct sem_correct';
    my ($foreign,$english) = split(/ .o0O0o. /,$memory);
    next unless defined($REF{$foreign});
    push @{$REF{$foreign}},$english;
  }
  my(@CORRECT,@TOTAL,$length_translation,$length_reference);
  # compute bleu
  for(my $i=0;$i<=$#TRANSLATION_SENTENCE;$i++) {
    my %REF_NGRAM = ();
    my @WORD = split(/ /,$TRANSLATION_SENTENCE[$i]);
    my $length_translation_this_sentence = scalar(@WORD);
    my ($closest_diff,$closest_length) = (9999,9999);
    foreach my $reference (@{$REF{$FOREIGN_SENTENCE[$i]}}) {
      my @WORD = split(/ /,$reference);
      my $length = scalar(@WORD);
      if (abs($length_translation_this_sentence-$length) < $closest_diff) {
	$closest_diff = abs($length_translation_this_sentence-$length);
	$closest_length = $length;
      }
      for(my $n=1;$n<=4;$n++) {
	my %REF_NGRAM_N = ();
	for(my $start=0;$start<=$#WORD-($n-1);$start++) {
	  my $ngram = "$n";
	  for(my $w=0;$w<$n;$w++) {
	    $ngram .= " ".$WORD[$start+$w];
	  }
	  $REF_NGRAM_N{$ngram}++;
	}
	foreach my $ngram (keys %REF_NGRAM_N) {
	  if (!defined($REF_NGRAM{$ngram}) ||
	      $REF_NGRAM{$ngram} < $REF_NGRAM_N{$ngram}) {
	    $REF_NGRAM{$ngram} = $REF_NGRAM_N{$ngram};
	  }
	}
      }
    }
    $length_translation += $length_translation_this_sentence;
    $length_reference += $closest_length;
    for(my $n=1;$n<=4;$n++) {
      my %T_NGRAM = ();
      for(my $start=0;$start<=$#WORD-($n-1);$start++) {
	my $ngram = "$n";
	for(my $w=0;$w<$n;$w++) {
	  $ngram .= " ".$WORD[$start+$w];
	}
	$T_NGRAM{$ngram}++;
      }
      foreach my $ngram (keys %T_NGRAM) {
	my $n = 0+$ngram;
#	print "$i e $ngram $T_NGRAM{$ngram}<BR>\n";
	$TOTAL[$n] += $T_NGRAM{$ngram};
	if (defined($REF_NGRAM{$ngram})) {
	  if ($REF_NGRAM{$ngram} >= $T_NGRAM{$ngram}) {
	    $CORRECT[$n] += $T_NGRAM{$ngram};
#	    print "$i e correct1 $T_NGRAM{$ngram}<BR>\n";
	  }
	  else {
	    $CORRECT[$n] += $REF_NGRAM{$ngram};
#	    print "$i e correct2 $REF_NGRAM{$ngram}<BR>\n";
	  }
	}
      }
    }
  }
  my $brevity_penalty = 1;
  if ($length_translation<$length_reference) {
    $brevity_penalty = exp(1-$length_reference/$length_translation);
  }
  my $bleu = $brevity_penalty * exp((my_log( $CORRECT[1]/$TOTAL[1] ) +
				     my_log( $CORRECT[2]/$TOTAL[2] ) +
				     my_log( $CORRECT[3]/$TOTAL[3] ) +
				     my_log( $CORRECT[4]/$TOTAL[4] ) ) / 4);

  open(BLEU,">>mbleu-memory.dat");
  @STAT = stat($translation_file);
  printf BLEU "$translation_file $STAT[9] %f %f %f %f %f %f\n",$bleu,$CORRECT[1]/$TOTAL[1],$CORRECT[2]/$TOTAL[2],$CORRECT[3]/$TOTAL[3],$CORRECT[4]/$TOTAL[4],$brevity_penalty;
  close(BLEU);

  return ($bleu,
	  100*$CORRECT[1]/$TOTAL[1],
	  100*$CORRECT[2]/$TOTAL[2],
	  100*$CORRECT[3]/$TOTAL[3],
	  100*$CORRECT[4]/$TOTAL[4],
	  $brevity_penalty);
}

sub my_log {
  return -9999999999 unless $_[0];
  return log($_[0]);
}


###### SCORE TRANSLATIONS

################################ IN PROGRESS ###############################
sub compare2
{
	&htmlhead("Compare Translations");
	print "<A HREF=\"?ACTION=VIEW_CORPUS&CORPUS=".CGI::escape($in{CORPUS})."\">View Corpus $in{CORPUS}</A><P>\n";
	print "<FORM ACTION=\"\" METHOD=POST>\n";
	print "<INPUT TYPE=HIDDEN NAME=ACTION VALUE=$in{ACTION}>\n";
	print "<INPUT TYPE=HIDDEN NAME=CORPUS VALUE=\"$in{CORPUS}\">\n";
	my $corpus = new Corpus('-name' => "$in{CORPUS}", '-descriptions' => \%FILEDESC, '-info_line' => $factorData{$in{CORPUS}});
	$corpus->writeComparisonPage(\*STDOUT, /^.*$/);
	print "</FORM>\n";
}

sub compare {
  &htmlhead("Compare Translations");
  print "<A HREF=\"?ACTION=VIEW_CORPUS&CORPUS=".CGI::escape($in{CORPUS})."\">View Corpus $in{CORPUS}</A><P>\n";
  print "<FORM ACTION=\"\" METHOD=POST>\n";
  print "<INPUT TYPE=HIDDEN NAME=ACTION VALUE=$in{ACTION}>\n";
  print "<INPUT TYPE=HIDDEN NAME=CORPUS VALUE=\"$in{CORPUS}\">\n";

  # get sentences
  my %SENTENCES;
  my $sentence_count;
  foreach (keys %in) {
    if (/^FILE_(.+)$/) {
      my $file = $1;
      print "<INPUT TYPE=HIDDEN NAME=\"$file\" VALUE=1>\n";
      my @SENTENCES;
      if ($file =~ /.sgm$/) {
	  @{$SENTENCES{$file}} = `grep '<seg' $in{CORPUS}.$file`;
	  for(my $i=0;$i<$#{$SENTENCES{$file}};$i++) {
	      $SENTENCES{$file}[$i] =~ s/^<seg[^>]+> *(\S.+\S) *<\/seg> *$/$1/;
	  }
      }
      else {
	  @{$SENTENCES{$file}} = `cat $in{CORPUS}.$1`;
	  chop(@{$SENTENCES{$file}});
      }

      $sentence_count = scalar @{$SENTENCES{$file}};
    }
  }
  my %REFERENCE;
  foreach (@SHOW) {
    if (-e "$in{CORPUS}.$_") {
      @{$REFERENCE{$_}} = `cat $in{CORPUS}.$_`; chop(@{$REFERENCE{$_}});
    }
  }

  # update memory
  foreach (keys %in) {
    next unless /^SYN_SCORE_(.+)_(\d+)$/;
    next unless $in{"SEM_SCORE_$1_$2"};
    &store_in_memory($REFERENCE{$FOREIGN}[$2],
		     $SENTENCES{$1}[$2],
                     "syn_".$in{"SYN_SCORE_$1_$2"}." sem_".$in{"SEM_SCORE_$1_$2"});
  }

  # display sentences
  for(my $i=0;$i<$sentence_count;$i++)
  {
    my $evaluation = "";
    my $show = 0;
    my $surface = "";
    foreach my $file (keys %SENTENCES)
	 {
      if ($in{SURFACE}) {
	$SENTENCES{$file}[$i] =~ s/ *$//;
	$surface = $SENTENCES{$file}[$i] if ($surface eq '');
	$show = 1 if ($SENTENCES{$file}[$i] ne $surface);
      }
      else {
	my $this_ev = &get_from_memory($REFERENCE{$FOREIGN}[$i],$SENTENCES{$file}[$i]);
	$this_ev = "syn_wrong sem_wrong" unless $this_ev;
	$evaluation = $this_ev if ($evaluation eq '');
	$show = 1 if ($evaluation ne $this_ev);
      }
    }
    next unless $show;
    print "<HR>Sentence ".($i+1).":<BR>\n";
    foreach my $ref (@SHOW) {
      if (-e "$in{CORPUS}.$ref") {
	print "<FONT COLOR=$SHOW_COLOR{$ref}>".$REFERENCE{$ref}[$i]."</FONT> (".$FILETYPE{$ref}.")<BR>\n";
      }
    }
    foreach my $file (keys %SENTENCES) {
      print "<B>$SENTENCES{$file}[$i]</B> ($file)<BR>\n";
      &color_highlight_ngrams($i,&nist_normalize_text($SENTENCES{$file}[$i]),$REFERENCE{"e"}[$i]);
      if (0 && $in{WITH_EVAL}) {
	$evaluation = &get_from_memory($REFERENCE{$FOREIGN}[$i],$SENTENCES{$file}[$i]);
	print "<INPUT TYPE=RADIO NAME=SYN_SCORE_$file"."_$i VALUE=correct";
	print " CHECKED" if ($evaluation =~ /syn_correct/);
	print "> perfect English\n";
	print "<INPUT TYPE=RADIO NAME=SYN_SCORE_$file"."_$i VALUE=wrong";
	print " CHECKED" if ($evaluation =~ /syn_wrong/);
	print "> imperfect English<BR>\n";
	print "<INPUT TYPE=RADIO NAME=SEM_SCORE_$file"."_$i VALUE=correct";
	print " CHECKED" if ($evaluation =~ /sem_correct/);
	print "> correct meaning\n";
	print "<INPUT TYPE=RADIO NAME=SEM_SCORE_$file"."_$i VALUE=wrong";
	print " CHECKED" if ($evaluation =~ /sem_wrong/);
	print "> incorrect meaning<BR>\n";
      }
    }
  }
  print "<P><INPUT TYPE=SUBMIT VALUE=\"Add evaluation\">\n";
  print "</FORM>\n";
}

###### MEMORY SUBS

sub load_memory {
  open(MEMORY,"evaluation-memory.dat") or return;
  while(<MEMORY>) {
    chop;
    my($foreign,$translation,$evaluation) = split(/ \.o0O0o\. /);
    $evaluation = 'syn_correct sem_correct' if ($evaluation eq 'correct');
    $MEMORY{"$foreign .o0O0o. $translation"} = $evaluation;
  }
  close(MEMORY);
}

sub get_score_from_memory {
  my($foreign_file,$translation_file) = @_;
  my $unknown=0;
  my $correct=0;
  my $just_syn=0;
  my $just_sem=0;
  my $wrong=0;
  my @FOREIGN = `cat $foreign_file`; chop(@FOREIGN);
  my @TRANSLATION = `cat $translation_file`; chop(@TRANSLATION);
  for(my $i=0;$i<=$#FOREIGN;$i++) {
    if (my $evaluation = &get_from_memory($FOREIGN[$i],$TRANSLATION[$i])) {
      if ($evaluation eq 'syn_correct sem_correct') { $correct++ }
      elsif ($evaluation eq 'syn_correct sem_wrong') { $just_syn++ }
      elsif ($evaluation eq 'syn_wrong sem_correct') { $just_sem++ }
      elsif ($evaluation eq 'syn_wrong sem_wrong') { $wrong++ }
      else { $unknown++; }
    }
    else { $unknown++; }
  }
  return($correct,$just_syn,$just_sem,$wrong,$unknown);
}

sub store_in_memory {
  my($foreign,$translation,$evaluation) = @_;
  &trim(\$translation);
  return if $MEMORY{"$foreign .o0O0o. $translation"} eq $evaluation;
  $MEMORY{"$foreign .o0O0o. $translation"} = $evaluation;
  open(MEMORY,">>evaluation-memory.dat") or die "store_in_memory(): couldn't open 'evaluation-memory.dat' for append\n";
  print MEMORY "$foreign .o0O0o. $translation .o0O0o. $evaluation\n";
  close(MEMORY);
}

sub get_from_memory {
  my($foreign,$translation) = @_;
  &trim(\$translation);
  return $MEMORY{"$foreign .o0O0o. $translation"};
}

sub trim {
  my($translation) = @_;
  $$translation =~ s/ +/ /g;
  $$translation =~ s/^ +//;
  $$translation =~ s/ +$//;
}

sub load_descriptions {
  open(FD,"file-descriptions") or die "load_descriptions(): couldn't open 'file-descriptions' for read\n";
  while(<FD>) {
  	chomp;
    my($file,$description) = split(/\s+/,$_,2);
    $FILEDESC{$file} = $description;
  }
  close(FD);
}

#read config file giving various corpus config info
#arguments: filename to read
#return: hash of corpus names to strings containing formatted info
sub loadFactorData
{
	my $filename = shift;
	my %data = ();
	open(INFILE, "<$filename") or die "loadFactorData(): couldn't open '$filename' for read\n";
	while(my $line = <INFILE>)
	{
		if($line =~ /^\#/) {next;} #skip comment lines
		$line =~ /^\s*(\S+)\s*:\s*(\S.*\S)\s*$/;
		my $corpusName = $1;
		$data{$corpusName} = $2;
	}
	close(INFILE);
	return %data;
}

###### SUBS

sub htmlhead {
  print <<"___ENDHTML";
Content-type: text/html

<HTML><HEAD>
<TITLE>MTEval: $_[0]</TITLE>
<SCRIPT LANGUAGE="JavaScript">

<!-- hide from old browsers

function FieldInfo(field,description) {
  popup = window.open("","popDialog","height=500,width=600,scrollbars=yes,resizable=yes");
  popup.document.write("<HTML><HEAD><TITLE>"+field+"</TITLE></HEAD><BODY BGCOLOR=#FFFFCC><CENTER><B>"+field+"</B><HR SIZE=2 NOSHADE></CENTER><PRE>"+description+"</PRE><CENTER><FORM><INPUT TYPE='BUTTON' VALUE='Okay' onClick='self.close()'></FORM><CENTER></BODY></HTML>");
  popup.focus();
  popup.document.close();
}

<!-- done hiding -->

</SCRIPT>
</HEAD>
<BODY BGCOLOR=white>
<H2>Evaluation Tool for Machine Translation<BR>$_[0]</H2>
___ENDHTML
}


############################# parts of cgi-lib.pl


sub ReadParse {
  my ($i, $key, $val);

  # Read in text
  my $in;
  if (&MethGet) {
    $in = $ENV{'QUERY_STRING'};
  } elsif (&MethPost) {
    read(STDIN,$in,$ENV{'CONTENT_LENGTH'});
  }

  my @in = split(/[&;]/,$in);

  foreach $i (0 .. $#in) {
    # Convert plus's to spaces
    $in[$i] =~ s/\+/ /g;

    # Split into key and value.
    ($key, $val) = split(/=/,$in[$i],2); # splits on the first =.

    # Convert %XX from hex numbers to alphanumeric
    $key =~ s/%(..)/pack("c",hex($1))/ge;
    $val =~ s/%(..)/pack("c",hex($1))/ge;

    # Associate key and value
    $in{$key} .= "\0" if (defined($in{$key})); # \0 is the multiple separator
    $in{$key} .= $val;

  }

  return scalar(@in);
}

sub MethGet {
  return ($ENV{'REQUEST_METHOD'} eq "GET");
}

sub MethPost {
  return ($ENV{'REQUEST_METHOD'} eq "POST");
}
