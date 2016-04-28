#!/usr/bin/perl -w
 
use strict;
 
#################################
# History:
#
# version 11b -- text normalization modified:
#    * take out the join digit line because it joins digits
#      when it shouldn't have
#      $norm_text =~ s/(\d)\s+(?=\d)/$1/g; #join digits
#
# version 11a -- corrected output of individual n-gram precision values
#
# version 11 -- bug fixes:
#    * make filehandle operate in binary mode to prevent Perl from operating
#      (by default in Red Hat 9) in UTF-8
#    * fix failure on joining digits
# version 10 -- updated output to include more details of n-gram scoring.
#    Defaults to generate both NIST and BLEU scores.  Use -b for BLEU
#    only, use -n for NIST only
#
# version 09d -- bug fix (for BLEU scoring, ngrams were fixed at 4
#    being the max, regardless what was entered on the command line.)
#
# version 09c -- bug fix (During the calculation of ngram information,
#    each ngram was being counted only once for each segment.  This has
#    been fixed so that each ngram is counted correctly in each segment.)
#
# version 09b -- text normalization modified:
#    * option flag added to preserve upper case
#    * non-ASCII characters left in place.
#
# version 09a -- text normalization modified:
#    * &quot; and &amp; converted to "" and &, respectively
#    * non-ASCII characters kept together (bug fix)
#
# version 09 -- modified to accommodate sgml tag and attribute
#    names revised to conform to default SGML conventions.
#
# version 08 -- modifies the NIST metric in accordance with the
#    findings on the 2001 Chinese-English dry run corpus.  Also
#    incorporates the BLEU metric as an option and supports the
#    output of ngram detail.
#
# version 07 -- in response to the MT meeting on 28 Jan 2002 at ISI
#    Keep strings of non-ASCII characters together as one word
#    (rather than splitting them into one-character words).
#    Change length penalty so that translations that are longer than
#    the average reference translation are not penalized.
#
# version 06
#    Prevent divide-by-zero when a segment has no evaluation N-grams.
#    Correct segment index for level 3 debug output.
#
# version 05
#    improve diagnostic error messages
#
# version 04
#    tag segments
#
# version 03
#    add detailed output option (intermediate document and segment scores)
#
# version 02
#    accommodation of modified sgml tags and attributes
#
# version 01
#    same as bleu version 15, but modified to provide formal score output.
#
# original IBM version
#    Author: Kishore Papineni
#    Date: 06/10/2001
#################################
 
######
# Intro
my ($date, $time) = date_time_stamp();
print "MT evaluation scorer began on $date at $time\n";
print "command line:  ", $0, " ", join(" ", @ARGV), "\n";
my $usage = "\n\nUsage: $0 [-h] -r <ref_file> -s src_file -t <tst_file>\n\n".
    "Description:  This Perl script evaluates MT system performance.\n".
    "\n".
    "Required arguments:\n".
    "  -r <ref_file> is a file containing the reference translations for\n".
    "      the documents to be evaluated.\n".
    "  -s <src_file> is a file containing the source documents for which\n".
    "      translations are to be evaluated\n".
    "  -t <tst_file> is a file containing the translations to be evaluated\n".
    "\n".
    "Optional arguments:\n".
    "  -c preserves upper-case alphabetic characters\n".
    "  -b generate BLEU scores only\n".
    "  -n generate NIST scores only\n".
    "  -d detailed output flag used in conjunction with \"-b\" or \"-n\" flags:\n".
    "         0 (default) for system-level score only\n".
    "         1 to include document-level scores\n".
    "         2 to include segment-level scores\n".
    "         3 to include ngram-level scores\n".
    "  -h prints this help message to STDOUT\n".
    "\n";
 
use vars qw ($opt_r $opt_s $opt_t $opt_d $opt_h $opt_b $opt_n $opt_c $opt_x);
use Getopt::Std;
getopts ('r:s:t:d:hbncx:');
die $usage if defined($opt_h);
die "Error in command line:  ref_file not defined$usage" unless defined $opt_r;
die "Error in command line:  src_file not defined$usage" unless defined $opt_s;
die "Error in command line:  tst_file not defined$usage" unless defined $opt_t;
my $max_Ngram = 9;
my $detail = defined $opt_d ? $opt_d : 0;
my $preserve_case = defined $opt_c ? 1 : 0;
 
my $METHOD = "BOTH";
if (defined $opt_b) { $METHOD = "BLEU"; }
if (defined $opt_n) { $METHOD = "NIST"; }
my $method;
 
my ($ref_file) = $opt_r;
my ($src_file) = $opt_s;
my ($tst_file) = $opt_t;
 
######
# Global variables
my ($src_lang, $tgt_lang, @tst_sys, @ref_sys); # evaluation parameters
my (%tst_data, %ref_data); # the data -- with structure:  {system}{document}[segments]
my ($src_id, $ref_id, $tst_id); # unique identifiers for ref and tst translation sets
my %eval_docs;     # document information for the evaluation data set
my %ngram_info;    # the information obtained from (the last word in) the ngram
 
######
# Get source document ID's
($src_id) = get_source_info ($src_file);
 
######
# Get reference translations
($ref_id) = get_MT_data (\%ref_data, "RefSet", $ref_file);
 
compute_ngram_info ();
 
######
# Get translations to evaluate
($tst_id) = get_MT_data (\%tst_data, "TstSet", $tst_file);
 
######
# Check data for completeness and correctness
check_MT_data ();
 
######
#
my %NISTmt = ();
my %BLEUmt = ();
 
######
# Evaluate
print "  Evaluation of $src_lang-to-$tgt_lang translation using:\n";
my $cum_seg = 0;
foreach my $doc (sort keys %eval_docs) {
    $cum_seg += @{$eval_docs{$doc}{SEGS}};
}
print "    src set \"$src_id\" (", scalar keys %eval_docs, " docs, $cum_seg segs)\n";
print "    ref set \"$ref_id\" (", scalar keys %ref_data, " refs)\n";
print "    tst set \"$tst_id\" (", scalar keys %tst_data, " systems)\n\n";
 
foreach my $sys (sort @tst_sys) {
    for (my $n=1; $n<=$max_Ngram; $n++) {
        $NISTmt{$n}{$sys}{cum} = 0;
        $NISTmt{$n}{$sys}{ind} = 0;
        $BLEUmt{$n}{$sys}{cum} = 0;
        $BLEUmt{$n}{$sys}{ind} = 0;
    }
 
    if (($METHOD eq "BOTH") || ($METHOD eq "NIST")) {
        $method="NIST";
        score_system ($sys, %NISTmt);
    }
    if (($METHOD eq "BOTH") || ($METHOD eq "BLEU")) {
        $method="BLEU";
        score_system ($sys, %BLEUmt);
    }
}
 
######
printout_report ();
 
($date, $time) = date_time_stamp();
print "MT evaluation scorer ended on $date at $time\n";
 
exit 0;
 
#################################
 
sub get_source_info {
 
    my ($file) = @_;
    my ($name, $id, $src, $doc);
    my ($data, $tag, $span);
     
 
#read data from file
    open (FILE, $file) or die "\nUnable to open translation data file '$file'", $usage;
    binmode FILE;
    $data .= $_ while <FILE>;
    close (FILE);
 
#get source set info
    die "\n\nFATAL INPUT ERROR:  no 'src_set' tag in src_file '$file'\n\n"
        unless ($tag, $span, $data) = extract_sgml_tag_and_span ("SrcSet", $data);
 
    die "\n\nFATAL INPUT ERROR:  no tag attribute '$name' in file '$file'\n\n"
        unless ($id) = extract_sgml_tag_attribute ($name="SetID", $tag);
 
    die "\n\nFATAL INPUT ERROR:  no tag attribute '$name' in file '$file'\n\n"
        unless ($src) = extract_sgml_tag_attribute ($name="SrcLang", $tag);
    die "\n\nFATAL INPUT ERROR:  $name ('$src') in file '$file' inconsistent\n"
        ."                    with $name in previous input data ('$src_lang')\n\n"
            unless (not defined $src_lang or $src eq $src_lang);
    $src_lang = $src;
 
#get doc info -- ID and # of segs
    $data = $span;
    while (($tag, $span, $data) = extract_sgml_tag_and_span ("Doc", $data)) {
        die "\n\nFATAL INPUT ERROR:  no tag attribute '$name' in file '$file'\n\n"
            unless ($doc) = extract_sgml_tag_attribute ($name="DocID", $tag);
        die "\n\nFATAL INPUT ERROR:  duplicate '$name' in file '$file'\n\n"
            if defined $eval_docs{$doc};
        $span =~ s/[\s\n\r]+/ /g;  # concatenate records
        my $jseg=0, my $seg_data = $span;
        while (($tag, $span, $seg_data) = extract_sgml_tag_and_span ("Seg", $seg_data)) {
            ($eval_docs{$doc}{SEGS}[$jseg++]) = NormalizeText ($span);
        }
        die "\n\nFATAL INPUT ERROR:  no segments in document '$doc' in file '$file'\n\n"
            if $jseg == 0;
    }
    die "\n\nFATAL INPUT ERROR:  no documents in file '$file'\n\n"
        unless keys %eval_docs > 0;
    return $id;
}
 
#################################
 
sub get_MT_data {
 
    my ($docs, $set_tag, $file) = @_;
    my ($name, $id, $src, $tgt, $sys, $doc);
    my ($tag, $span, $data);
 
#read data from file
    open (FILE, $file) or die "\nUnable to open translation data file '$file'", $usage;
    binmode FILE;
    $data .= $_ while <FILE>;
    close (FILE);
 
#get tag info
    while (($tag, $span, $data) = extract_sgml_tag_and_span ($set_tag, $data)) {
        die "\n\nFATAL INPUT ERROR:  no tag attribute '$name' in file '$file'\n\n" unless
            ($id) = extract_sgml_tag_attribute ($name="SetID", $tag);
 
        die "\n\nFATAL INPUT ERROR:  no tag attribute '$name' in file '$file'\n\n" unless
            ($src) = extract_sgml_tag_attribute ($name="SrcLang", $tag);
        die "\n\nFATAL INPUT ERROR:  $name ('$src') in file '$file' inconsistent\n"
            ."                    with $name of source ('$src_lang')\n\n"
                unless $src eq $src_lang;
         
        die "\n\nFATAL INPUT ERROR:  no tag attribute '$name' in file '$file'\n\n" unless
            ($tgt) = extract_sgml_tag_attribute ($name="TrgLang", $tag);
        die "\n\nFATAL INPUT ERROR:  $name ('$tgt') in file '$file' inconsistent\n"
            ."                    with $name of the evaluation ('$tgt_lang')\n\n"
                unless (not defined $tgt_lang or $tgt eq $tgt_lang);
        $tgt_lang = $tgt;
 
        my $mtdata = $span;
        while (($tag, $span, $mtdata) = extract_sgml_tag_and_span ("Doc", $mtdata)) {
            die "\n\nFATAL INPUT ERROR:  no tag attribute '$name' in file '$file'\n\n" unless
                (my $sys) = extract_sgml_tag_attribute ($name="SysID", $tag);
             
            die "\n\nFATAL INPUT ERROR:  no tag attribute '$name' in file '$file'\n\n" unless
                $doc = extract_sgml_tag_attribute ($name="DocID", $tag);
             
            die "\n\nFATAL INPUT ERROR:  document '$doc' for system '$sys' in file '$file'\n"
                ."                    previously loaded from file '$docs->{$sys}{$doc}{FILE}'\n\n"
                    unless (not defined $docs->{$sys}{$doc});
 
            $span =~ s/[\s\n\r]+/ /g;  # concatenate records
            my $jseg=0, my $seg_data = $span;
            while (($tag, $span, $seg_data) = extract_sgml_tag_and_span ("Seg", $seg_data)) {
                ($docs->{$sys}{$doc}{SEGS}[$jseg++]) = NormalizeText ($span);
            }
            die "\n\nFATAL INPUT ERROR:  no segments in document '$doc' in file '$file'\n\n"
                if $jseg == 0;
            $docs->{$sys}{$doc}{FILE} = $file;
        }
    }
    return $id;
}
 
#################################
 
sub check_MT_data {
 
    @tst_sys = sort keys %tst_data;
    @ref_sys = sort keys %ref_data;
 
#every evaluation document must be represented for every system and every reference
    foreach my $doc (sort keys %eval_docs) {
        my $nseg_source = @{$eval_docs{$doc}{SEGS}};
        foreach my $sys (@tst_sys) {
            die "\n\nFATAL ERROR:  no document '$doc' for system '$sys'\n\n"
                unless defined $tst_data{$sys}{$doc};
            my $nseg = @{$tst_data{$sys}{$doc}{SEGS}};
            die "\n\nFATAL ERROR:  translated documents must contain the same # of segments as the source, but\n"
                ."              document '$doc' for system '$sys' contains $nseg segments, while\n"
                ."              the source document contains $nseg_source segments.\n\n"
                    unless $nseg == $nseg_source;
        }
 
        foreach my $sys (@ref_sys) {
            die "\n\nFATAL ERROR:  no document '$doc' for reference '$sys'\n\n"
                unless defined $ref_data{$sys}{$doc};
            my $nseg = @{$ref_data{$sys}{$doc}{SEGS}};
            die "\n\nFATAL ERROR:  translated documents must contain the same # of segments as the source, but\n"
                ."              document '$doc' for system '$sys' contains $nseg segments, while\n"
                ."              the source document contains $nseg_source segments.\n\n"
                    unless $nseg == $nseg_source;
        }
    }
}
 
#################################
 
sub compute_ngram_info {
 
    my ($ref, $doc, $seg);
    my (@wrds, $tot_wrds, %ngrams, $ngram, $mgram);
    my (%ngram_count, @tot_ngrams);
 
    foreach $ref (keys %ref_data) {
        foreach $doc (keys %{$ref_data{$ref}}) {
            foreach $seg (@{$ref_data{$ref}{$doc}{SEGS}}) {
                @wrds = split /\s+/, $seg;
                $tot_wrds += @wrds;
                %ngrams = %{Words2Ngrams (@wrds)};
                foreach $ngram (keys %ngrams) {
                    $ngram_count{$ngram} += $ngrams{$ngram};
                }
            }
        }
    }
     
    foreach $ngram (keys %ngram_count) {
        @wrds = split / /, $ngram;
        pop @wrds, $mgram = join " ", @wrds;
        $ngram_info{$ngram} = - log
            ($mgram ? $ngram_count{$ngram}/$ngram_count{$mgram}
                    : $ngram_count{$ngram}/$tot_wrds) / log 2;
        if (defined $opt_x and $opt_x eq "ngram info") {
            @wrds = split / /, $ngram;
            printf "ngram info:%9.4f%6d%6d%8d%3d %s\n", $ngram_info{$ngram}, $ngram_count{$ngram},
                $mgram ? $ngram_count{$mgram} : $tot_wrds, $tot_wrds, scalar @wrds, $ngram;
        }
    }
}
 
#################################
 
sub score_system {
 
    my ($sys, $ref, $doc, %SCOREmt);
    ($sys, %SCOREmt) = @_;
    my ($shortest_ref_length, $match_cnt, $tst_cnt, $ref_cnt, $tst_info, $ref_info);
    my ($cum_ref_length, @cum_match, @cum_tst_cnt, @cum_ref_cnt, @cum_tst_info, @cum_ref_info);
 
    $cum_ref_length = 0;
    for (my $j=1; $j<=$max_Ngram; $j++) {
        $cum_match[$j] = $cum_tst_cnt[$j] = $cum_ref_cnt[$j] = $cum_tst_info[$j] = $cum_ref_info[$j] = 0;
    }
         
    foreach $doc (sort keys %eval_docs) {
        ($shortest_ref_length, $match_cnt, $tst_cnt, $ref_cnt, $tst_info, $ref_info) = score_document ($sys, $doc);
 
#output document summary score
        if (($detail >= 1 ) && ($METHOD eq "NIST"))  {
            my %DOCmt = ();
            printf "$method score using   5-grams = %.4f for system \"$sys\" on document \"$doc\" (%d segments, %d words)\n",
            nist_score (scalar @ref_sys, $match_cnt, $tst_cnt, $ref_cnt, $tst_info, $ref_info, $sys, %DOCmt),
            scalar @{$tst_data{$sys}{$doc}{SEGS}}, $tst_cnt->[1];
        }
        if (($detail >= 1 ) && ($METHOD eq "BLEU"))  {
            my %DOCmt = ();
            printf "$method score using   4-grams = %.4f for system \"$sys\" on document \"$doc\" (%d segments, %d words)\n",
            bleu_score($shortest_ref_length, $match_cnt, $tst_cnt, $sys, %DOCmt),
            scalar @{$tst_data{$sys}{$doc}{SEGS}}, $tst_cnt->[1];
        }
         
        $cum_ref_length += $shortest_ref_length;
        for (my $j=1; $j<=$max_Ngram; $j++) {
            $cum_match[$j] += $match_cnt->[$j];
            $cum_tst_cnt[$j] += $tst_cnt->[$j];
            $cum_ref_cnt[$j] += $ref_cnt->[$j];
            $cum_tst_info[$j] += $tst_info->[$j];
            $cum_ref_info[$j] += $ref_info->[$j];
            printf "document info: $sys $doc %d-gram %d %d %d %9.4f %9.4f\n", $j, $match_cnt->[$j],
                $tst_cnt->[$j], $ref_cnt->[$j], $tst_info->[$j], $ref_info->[$j]
                    if (defined $opt_x and $opt_x eq "document info");
        }
    }
 
#x #output system summary score
#x    printf "$method score = %.4f for system \"$sys\"\n",
#x        $method eq "BLEU" ?  bleu_score($cum_ref_length, \@cum_match, \@cum_tst_cnt) :
#x          nist_score (scalar @ref_sys, \@cum_match, \@cum_tst_cnt, \@cum_ref_cnt, \@cum_tst_info, \@cum_ref_info, $sys, %SCOREmt);
    if ($method eq "BLEU")  {
        bleu_score($cum_ref_length, \@cum_match, \@cum_tst_cnt, $sys, %SCOREmt);
    }
    if ($method eq "NIST") {
        nist_score (scalar @ref_sys, \@cum_match, \@cum_tst_cnt, \@cum_ref_cnt, \@cum_tst_info, \@cum_ref_info, $sys, %SCOREmt);
    }
}
 
#################################
 
sub score_document {
 
    my ($sys, $ref, $doc);
    ($sys, $doc) = @_;
    my ($shortest_ref_length, $match_cnt, $tst_cnt, $ref_cnt, $tst_info, $ref_info);
    my ($cum_ref_length, @cum_match, @cum_tst_cnt, @cum_ref_cnt, @cum_tst_info, @cum_ref_info);
 
    $cum_ref_length = 0;
    for (my $j=1; $j<=$max_Ngram; $j++) {
        $cum_match[$j] = $cum_tst_cnt[$j] = $cum_ref_cnt[$j] = $cum_tst_info[$j] = $cum_ref_info[$j] = 0;
    }
         
#score each segment
    for (my $jseg=0; $jseg<@{$tst_data{$sys}{$doc}{SEGS}}; $jseg++) {
        my @ref_segments = ();
        foreach $ref (@ref_sys) {
            push @ref_segments, $ref_data{$ref}{$doc}{SEGS}[$jseg];
            printf "ref '$ref', seg %d: %s\n", $jseg+1, $ref_data{$ref}{$doc}{SEGS}[$jseg]
                if $detail >= 3;
        }
        printf "sys '$sys', seg %d: %s\n", $jseg+1, $tst_data{$sys}{$doc}{SEGS}[$jseg]
            if $detail >= 3;
        ($shortest_ref_length, $match_cnt, $tst_cnt, $ref_cnt, $tst_info, $ref_info) =
            score_segment ($tst_data{$sys}{$doc}{SEGS}[$jseg], @ref_segments);
 
#output segment summary score
#x      printf "$method score = %.4f for system \"$sys\" on segment %d of document \"$doc\" (%d words)\n",
#x            $method eq "BLEU" ?  bleu_score($shortest_ref_length, $match_cnt, $tst_cnt) :
#x              nist_score (scalar @ref_sys, $match_cnt, $tst_cnt, $ref_cnt, $tst_info, $ref_info),
#x              $jseg+1, $tst_cnt->[1]
#x                  if $detail >= 2;
        if (($detail >=2) && ($METHOD eq "BLEU")) {
            my %DOCmt = ();
            printf "  $method score using 4-grams = %.4f for system \"$sys\" on segment %d of document \"$doc\" (%d words)\n",
            bleu_score($shortest_ref_length, $match_cnt, $tst_cnt, $sys, %DOCmt), $jseg+1, $tst_cnt->[1];
        }
        if (($detail >=2) && ($METHOD eq "NIST")) {
            my %DOCmt = ();
            printf "  $method score using 5-grams = %.4f for system \"$sys\" on segment %d of document \"$doc\" (%d words)\n",
            nist_score (scalar @ref_sys, $match_cnt, $tst_cnt, $ref_cnt, $tst_info, $ref_info, $sys, %DOCmt), $jseg+1, $tst_cnt->[1];
        }
 
 
        $cum_ref_length += $shortest_ref_length;
        for (my $j=1; $j<=$max_Ngram; $j++) {
            $cum_match[$j] += $match_cnt->[$j];
            $cum_tst_cnt[$j] += $tst_cnt->[$j];
            $cum_ref_cnt[$j] += $ref_cnt->[$j];
            $cum_tst_info[$j] += $tst_info->[$j];
            $cum_ref_info[$j] += $ref_info->[$j];
        }
    }
    return ($cum_ref_length, [@cum_match], [@cum_tst_cnt], [@cum_ref_cnt], [@cum_tst_info], [@cum_ref_info]);
}
 
#################################
 
sub score_segment {
 
    my ($tst_seg, @ref_segs) = @_;
    my (@tst_wrds, %tst_ngrams, @match_count, @tst_count, @tst_info);
    my (@ref_wrds, $ref_seg, %ref_ngrams, %ref_ngrams_max, @ref_count, @ref_info);
    my ($ngram);
    my (@nwrds_ref);
    my $shortest_ref_length;
 
    for (my $j=1; $j<= $max_Ngram; $j++) {
        $match_count[$j] = $tst_count[$j] = $ref_count[$j] = $tst_info[$j] = $ref_info[$j] = 0;
    }
 
# get the ngram counts for the test segment
    @tst_wrds = split /\s+/, $tst_seg;
    %tst_ngrams = %{Words2Ngrams (@tst_wrds)};
    for (my $j=1; $j<=$max_Ngram; $j++) { # compute ngram counts
        $tst_count[$j]  = $j<=@tst_wrds ? (@tst_wrds - $j + 1) : 0;
    }
 
# get the ngram counts for the reference segments
    foreach $ref_seg (@ref_segs) {
        @ref_wrds = split /\s+/, $ref_seg;
        %ref_ngrams = %{Words2Ngrams (@ref_wrds)};
        foreach $ngram (keys %ref_ngrams) { # find the maximum # of occurrences
            my @wrds = split / /, $ngram;
            $ref_info[@wrds] += $ngram_info{$ngram};
            $ref_ngrams_max{$ngram} = defined $ref_ngrams_max{$ngram} ?
                max ($ref_ngrams_max{$ngram}, $ref_ngrams{$ngram}) :
                    $ref_ngrams{$ngram};
        }
        for (my $j=1; $j<=$max_Ngram; $j++) { # update ngram counts
            $ref_count[$j] += $j<=@ref_wrds ? (@ref_wrds - $j + 1) : 0;
        }
        $shortest_ref_length = scalar @ref_wrds # find the shortest reference segment
            if (not defined $shortest_ref_length) or @ref_wrds < $shortest_ref_length;
    }
 
# accumulate scoring stats for tst_seg ngrams that match ref_seg ngrams
    foreach $ngram (keys %tst_ngrams) {
        next unless defined $ref_ngrams_max{$ngram};
        my @wrds = split / /, $ngram;
        $tst_info[@wrds] += $ngram_info{$ngram} * min($tst_ngrams{$ngram},$ref_ngrams_max{$ngram});
        $match_count[@wrds] += my $count = min($tst_ngrams{$ngram},$ref_ngrams_max{$ngram});
        printf "%.2f info for each of $count %d-grams = '%s'\n", $ngram_info{$ngram}, scalar @wrds, $ngram
            if $detail >= 3;
    }
 
    return ($shortest_ref_length, [@match_count], [@tst_count], [@ref_count], [@tst_info], [@ref_info]);
}
 
#################################
                                                                                                                                                    
sub bleu_score {
 
    my ($shortest_ref_length, $matching_ngrams, $tst_ngrams, $sys, %SCOREmt) = @_;
 
    my $score = 0;
    my $iscore = 0;
    my $len_score = min (0, 1-$shortest_ref_length/$tst_ngrams->[1]);

    for (my $j=1; $j<=$max_Ngram; $j++) {
        if ($matching_ngrams->[$j] == 0) {
            $SCOREmt{$j}{$sys}{cum}=0;
        } else {
# Cumulative N-Gram score
            $score += log ($matching_ngrams->[$j]/$tst_ngrams->[$j]);
            $SCOREmt{$j}{$sys}{cum} = exp($score/$j + $len_score);
# Individual N-Gram score
            $iscore = log ($matching_ngrams->[$j]/$tst_ngrams->[$j]);
            $SCOREmt{$j}{$sys}{ind} = exp($iscore);
        }
    }
    return $SCOREmt{4}{$sys}{cum};
}
 
#################################
 
sub nist_score {
 
    my ($nsys, $matching_ngrams, $tst_ngrams, $ref_ngrams, $tst_info, $ref_info, $sys, %SCOREmt) = @_;
 
    my $score = 0;
    my $iscore = 0;
 
 
    for (my $n=1; $n<=$max_Ngram; $n++) {
        $score += $tst_info->[$n]/max($tst_ngrams->[$n],1);
        $SCOREmt{$n}{$sys}{cum} = $score * nist_length_penalty($tst_ngrams->[1]/($ref_ngrams->[1]/$nsys));
 
        $iscore = $tst_info->[$n]/max($tst_ngrams->[$n],1);
        $SCOREmt{$n}{$sys}{ind} = $iscore * nist_length_penalty($tst_ngrams->[1]/($ref_ngrams->[1]/$nsys));
    }
    return $SCOREmt{5}{$sys}{cum};
}
 
#################################
 
sub Words2Ngrams { #convert a string of words to an Ngram count hash
 
    my %count = ();
 
    for (; @_; shift) {
        my ($j, $ngram, $word);
        for ($j=0; $j<$max_Ngram and defined($word=$_[$j]); $j++) {
            $ngram .= defined $ngram ? " $word" : $word;
            $count{$ngram}++;
        }
    }
    return {%count};
}
 
#################################
 
sub NormalizeText {
    my ($norm_text) = @_;
 
# language-independent part:
    $norm_text =~ s/<skipped>//g; # strip "skipped" tags
    $norm_text =~ s/-\n//g; # strip end-of-line hyphenation and join lines
    $norm_text =~ s/\n/ /g; # join lines
    $norm_text =~ s/&quot;/"/g;  # convert SGML tag for quote to "
    $norm_text =~ s/&amp;/&/g;   # convert SGML tag for ampersand to &
    $norm_text =~ s/&lt;/</g;    # convert SGML tag for less-than to >
    $norm_text =~ s/&gt;/>/g;    # convert SGML tag for greater-than to <
 
# language-dependent part (assuming Western languages):
    $norm_text = " $norm_text ";
    $norm_text =~ tr/[A-Z]/[a-z]/ unless $preserve_case;
    $norm_text =~ s/([\{-\~\[-\` -\&\(-\+\:-\@\/])/ $1 /g;   # tokenize punctuation
    $norm_text =~ s/([^0-9])([\.,])/$1 $2 /g; # tokenize period and comma unless preceded by a digit
    $norm_text =~ s/([\.,])([^0-9])/ $1 $2/g; # tokenize period and comma unless followed by a digit
    $norm_text =~ s/([0-9])(-)/$1 $2 /g; # tokenize dash when preceded by a digit
    $norm_text =~ s/\s+/ /g; # one space only between words
    $norm_text =~ s/^\s+//;  # no leading space
    $norm_text =~ s/\s+$//;  # no trailing space
 
    return $norm_text;
}
 
#################################
 
sub nist_length_penalty {
 
    my ($ratio) = @_;
    return 1 if $ratio >= 1;
    return 0 if $ratio <= 0;
    my $ratio_x = 1.5;
    my $score_x = 0.5;
    my $beta = -log($score_x)/log($ratio_x)/log($ratio_x);
    return exp (-$beta*log($ratio)*log($ratio));
}
 
#################################
 
sub date_time_stamp {
 
    my ($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) = localtime();
    my @months = qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);
    my ($date, $time);
                                                                                                                                                    
    $time = sprintf "%2.2d:%2.2d:%2.2d", $hour, $min, $sec;
    $date = sprintf "%4.4s %3.3s %s", 1900+$year, $months[$mon], $mday;
    return ($date, $time);
}
 
#################################
 
sub extract_sgml_tag_and_span {
     
    my ($name, $data) = @_;
     
    ($data =~ m|<$name\s*([^>]*)>(.*?)</$name\s*>(.*)|si) ? ($1, $2, $3) : ();
}
 
#################################
 
sub extract_sgml_tag_attribute {
 
    my ($name, $data) = @_;
 
    ($data =~ m|$name\s*=\s*\"([^\"]*)\"|si) ? ($1) : ();
}
 
#################################
 
sub max {
 
    my ($max, $next);
 
    return unless defined ($max=pop);
    while (defined ($next=pop)) {
        $max = $next if $next > $max;
    }
    return $max;
}
 
#################################
 
sub min {
 
    my ($min, $next);
 
    return unless defined ($min=pop);
    while (defined ($next=pop)) {
        $min = $next if $next < $min;
    }
    return $min;
}
 
#################################
 
sub printout_report
{
  
    if ( $METHOD eq "BOTH" ) {
        foreach my $sys (sort @tst_sys) {
            printf "NIST score = %2.4f  BLEU score = %.4f for system \"$sys\"\n",$NISTmt{5}{$sys}{cum},$BLEUmt{4}{$sys}{cum};
        }
    } elsif ($METHOD eq "NIST" ) {
        foreach my $sys (sort @tst_sys) {
            printf "NIST score = %2.4f  for system \"$sys\"\n",$NISTmt{5}{$sys}{cum};
        }
    } elsif ($METHOD eq "BLEU" ) {
        foreach my $sys (sort @tst_sys) {
            printf "\nBLEU score = %.4f for system \"$sys\"\n",$BLEUmt{4}{$sys}{cum};
        }
    }
     
  
    printf "\n# ------------------------------------------------------------------------\n\n";
    printf "Individual N-gram scoring\n";
    printf "        1-gram   2-gram   3-gram   4-gram   5-gram   6-gram   7-gram   8-gram   9-gram\n";
    printf "        ------   ------   ------   ------   ------   ------   ------   ------   ------\n";
                                                                                                                                                    
    if (( $METHOD eq "BOTH" ) || ($METHOD eq "NIST")) {
        foreach my $sys (sort @tst_sys) {
            printf " NIST:";
            for (my $i=1; $i<=$max_Ngram; $i++) {
                printf "  %2.4f ",$NISTmt{$i}{$sys}{ind}
            }
            printf " \"$sys\"\n";
        }
        printf "\n";
    }
     
    if (( $METHOD eq "BOTH" ) || ($METHOD eq "BLEU")) {
        foreach my $sys (sort @tst_sys) {
            printf " BLEU:";
            for (my $i=1; $i<=$max_Ngram; $i++) {
                printf "  %2.4f ",$BLEUmt{$i}{$sys}{ind}
            }
           printf " \"$sys\"\n";
        }
    }
     
    printf "\n# ------------------------------------------------------------------------\n";
    printf "Cumulative N-gram scoring\n";
    printf "        1-gram   2-gram   3-gram   4-gram   5-gram   6-gram   7-gram   8-gram   9-gram\n";
    printf "        ------   ------   ------   ------   ------   ------   ------   ------   ------\n";
  
    if (( $METHOD eq "BOTH" ) || ($METHOD eq "NIST")) {
        foreach my $sys (sort @tst_sys) {
            printf " NIST:";
            for (my $i=1; $i<=$max_Ngram; $i++) {
                printf "  %2.4f ",$NISTmt{$i}{$sys}{cum}
            }
            printf " \"$sys\"\n";
        }
    }
    printf "\n";
                                                                                                                                                    
  
    if (( $METHOD eq "BOTH" ) || ($METHOD eq "BLEU")) {
        foreach my $sys (sort @tst_sys) {
            printf " BLEU:";
            for (my $i=1; $i<=$max_Ngram; $i++) {
                printf "  %2.4f ",$BLEUmt{$i}{$sys}{cum}
            }
            printf " \"$sys\"\n";
        }
    }
}
