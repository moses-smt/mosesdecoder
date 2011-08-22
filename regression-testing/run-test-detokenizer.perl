#!/usr/bin/perl -w
#
# Detokenization tests.
#

use strict;
# This is here to suppress (false) warnings about OLDOUT and OLDERR being used only once.  Maybe there is a less brutish way to suppress that, but I don't know it.
no warnings 'once';
use utf8;

use Cwd ('abs_path');
use File::Spec::Functions;
use File::Basename ('dirname');
use IPC::Run3;
use Getopt::Long;
use Test::More;

GetOptions("detokenizer=s" => \(my $detokenizer),
           "results-dir=s"=> \(my $results_dir)
          ) or exit 1;

unless (defined $results_dir) {
    print STDERR "Usage: run-test-detokenizer.t --results-dir <RESULTS-DIRECTORY> [--detokenizer <DETOKENIZER-SCRIPT>]\n";
    exit 1;
}

die "ERROR: Results directory ".$results_dir." doesn't exist or is not a writable directory. Dying" unless (-d $results_dir && -w $results_dir);

$detokenizer = catfile(dirname(dirname(abs_path($0))), "scripts", "tokenizer", "detokenizer.perl") unless $detokenizer;
die "ERROR: Detokenizer script ".$detokenizer." does not exist. Dying" unless -f $detokenizer;


my @testCases = ();

######################################
# Definitions of individual test cases
######################################

# A simple English test
&addDetokenizerTest("TEST_ENGLISH_EASY", "en",
<<'TOK'
This sentence is really simple , so it should not be hard to detokenize .
This one is no more difficult , but , hey , it is on a new line .
TOK
,
<<'EXP'
This sentence is really simple, so it should not be hard to detokenize.
This one is no more difficult, but, hey, it is on a new line.
EXP
);

# An English test involving double-quotes
&addDetokenizerTest("TEST_ENGLISH_DOUBLEQUOTES", "en",
<<'TOK'
This is a somewhat " less simple " test .
TOK
,
<<'EXP'
This is a somewhat "less simple" test.
EXP
);

# A simple French test
&addDetokenizerTest("TEST_FRENCH_EASY", "fr",
<<'TOK'
Voici une phrase simple .
TOK
,
<<'EXP'
Voici une phrase simple.
EXP
);

# A French test involving an apostrophe
&addDetokenizerTest("TEST_FRENCH_APOSTROPHE", "fr",
<<'TOK'
Moi , j' ai une apostrophe .
TOK
,
<<'EXP'
Moi, j'ai une apostrophe.
EXP
);

# A French test involving an apostrophe on the second-last word
&addDetokenizerTest("TEST_FRENCH_APOSTROPHE_PENULTIMATE", "fr",
<<'TOK'
de musique rap issus de l' immigration
TOK
,
<<'EXP'
de musique rap issus de l'immigration
EXP
);

# A German test involving non-ASCII characters
# Note: We don't specify a language because the detokenizer errors if you pass in a language for which it has no special rules, of which German is an example.
&addDetokenizerTest("TEST_GERMAN_NONASCII", undef,
<<'TOK'
Ich hoffe , daß Sie schöne Ferien hatten .
Frau Präsidentin ! Frau Díez González und ich hatten einige Anfragen
TOK
,
<<'EXP'
Ich hoffe, daß Sie schöne Ferien hatten.
Frau Präsidentin! Frau Díez González und ich hatten einige Anfragen
EXP
);

# A simple Chinese test
&addDetokenizerTest("TEST_CHINESE_EASY", undef,
<<'TOK'
这 是 一个 简单 的的 汉语 句子 。
TOK
,
<<'EXP'
这是一个简单的的汉语句子。
EXP
);

# A simple Japanese test
&addDetokenizerTest("TEST_JAPANESE_EASY", undef,
<<'TOK'
どう しょ う か な 。
どこ で 食べ たい 。
TOK
,
<<'EXP'
どうしょうかな。
どこで食べたい。
EXP
);


######################################
# Now run those babies ...
######################################

plan tests => scalar(@testCases);

foreach my $testCase (@testCases) {
    &runDetokenizerTest($testCase);
}

############
## Utilities
############

# Creates a new detokenizer test case, adds it to the array of test cases to be run, and returns it.
sub addDetokenizerTest {
    my ($testName, $language, $tokenizedText, $rightAnswer) = @_;

    my $testCase = new DetokenizerTestCase($testName, $language, $tokenizedText, $rightAnswer);
    push(@testCases, $testCase);
    return $testCase;
}

sub runDetokenizerTest {
    my ($testCase) = @_;

    my $testOutputDir = catfile($results_dir, $testCase->getName());
    my $tokenizedFile = catfile($testOutputDir, "input.txt");
    my $expectedFile = catfile($testOutputDir, "expected.txt");

    # Fail if we can't make the test output directory
    unless (mkdir($testOutputDir)) {
	return fail($testCase->getName().": Failed to create output directory ".$testOutputDir." [".$!."]");
    }
    
    open TOK, ">".$tokenizedFile;
    binmode TOK, ":utf8";
    print TOK $testCase->getTokenizedText();
    close TOK;
    
    open TRUTH, ">".$expectedFile;
    binmode TRUTH, ":utf8";
    print TRUTH $testCase->getRightAnswer();
    close TRUTH;

    &runTest($testCase->getName(), $testOutputDir, $tokenizedFile, sub {
	return defined($testCase->getLanguage()) ? [$detokenizer, "-l", $testCase->getLanguage()] : [$detokenizer];
    }, sub {
	&verifyIdentical($testCase->getName(), $expectedFile, catfile($testOutputDir, "stdout.txt"))
    }, 1, $testCase->getFailureExplanation());
}

# $stdinFile, if defined, is a file to send to the command via STDIN
# $buildCommandRoutineReference is a reference to a zero-argument subroutine that returns the
#                               system command to run in the form of an array reference
# $validationRoutineReference is a reference to a zero-argument subroutine that makes exactly one call
#                             to ok() or similar to validate the contents of the output directory
# $separateStdoutFromStderr is an optional boolean argument; if omitted or false, the command's
#                           STDOUT and STDERR are mixed together in out output file called
#                           stdout-and-stderr.txt; otherwise, they are printed to separate output
#                           files called stdout.txt and stderr.txt, respectively
# $failureExplanation is an explanation of why the test is expected to fail.  If the test is expected
#                     to pass, then this should be left undefined.  Even in the case of a test that
#                     is expected to fail, the system command is still expected to exit normally --
#                     only the validation routine is expected to fail.
sub runTest {
    my ($testName, $outputDir, $stdinFile, $buildCommandRoutineReference, $validationRoutineReference, $separateStdoutFromStderr, $failureExplanation) = @_;

    my ($stdoutFile, $stderrFile);
    if ($separateStdoutFromStderr) {
	$stdoutFile = catfile($outputDir, "stdout.txt");
	$stderrFile = catfile($outputDir, "stderr.txt");
    } else {
	$stdoutFile = catfile($outputDir, "stdout-and-stderr.txt");
	$stderrFile = $stdoutFile;
    }

    my $commandRef = $buildCommandRoutineReference->();
    my $exitStatus = &runVerbosely($commandRef, $stdinFile, $stdoutFile, $stderrFile);
    return fail($testName.": command exited with status ".$exitStatus) unless $exitStatus == 0;

    if (defined $failureExplanation) {
      TODO: {
	  local $TODO = $failureExplanation;
	  $validationRoutineReference->();
	}
    } else {
	$validationRoutineReference->();
    }
}

# Announce that we're going to run the given command, then run it.
# $stdinFile, if defined, is a file to send to the command via STDIN
# $stdoutFile and $stderrFile, if defined, are file paths to which the command's standard output
# and standard error, respectively, are written. They can be the same file.
# The exit code of the command is returned.
sub runVerbosely {
    my ($commandRef, $stdinFile, $stdoutFile, $stderrFile) = @_;
    my @command = @{$commandRef};
    note("Executing command:\n  @command\n");
    note("standard input coming from: ".$stdinFile) if defined $stdinFile;
    note("standard output going to: ".$stdoutFile) if defined $stdoutFile;
    note("standard error going to: ".$stderrFile) if defined $stderrFile;
    run3($commandRef, $stdinFile, $stdoutFile, $stderrFile);
    return $?;
}

# Verify that the given output file is identical to the given reference file.
sub verifyIdentical {
    my ($testName, $referenceFile, $outputFile) = @_;

    open(REF, $referenceFile) or return fail($testName.": Can't open reference file ".$referenceFile." [".$!."].");
    open(OUT, $outputFile) or return fail($testName.": Can't open output file ".$outputFile." [".$!."].");
    my @referenceFileAsArray = <REF>;
    my @outputFileAsArray = <OUT>;
    close(REF);
    close(OUT);
    is_deeply(\@outputFileAsArray, \@referenceFileAsArray, $testName.": Output file ".$outputFile." matches reference file ".$referenceFile.".");
}


##%%%%%%%%%%%%%%%%%%%%%%%%%%%##
## DetokenizerTestCase class ##

package DetokenizerTestCase;

# Constructor
sub new {
    my $class = shift;
    my $self = {
	_name                 => shift,
	_language             => shift,
	_tokenizedText        => shift,
	_rightAnswer          => shift,

	_failureExplanation   => undef
    };
    bless $self, $class;
}

sub getName {
    my ($self) = @_;
    return $self->{_name};
}

sub getLanguage {
    my ($self) = @_;
    return $self->{_language};
}

sub getTokenizedText {
    my ($self) = @_;
    return $self->{_tokenizedText};
}

sub getRightAnswer {
    my ($self) = @_;
    return $self->{_rightAnswer};
}

# Call this routine to indicate that this test case is expected to fail.
# (The detokenizer script is still expected to exit normally, but the output is not expected to
# match the right answer because of a bug or unimplemented use case.)
sub setExpectedToFail {
    my ($self, $failureExplanation) = @_;
    $self->{_failureExplanation} = $failureExplanation || "This test is expected to fail.";
}

# Returns a string explaining why this test is expected to fail, or undef if this test is expected
# to pass.
sub getFailureExplanation {
    my ($self) = @_;
    return $self->{_failureExplanation};
}
