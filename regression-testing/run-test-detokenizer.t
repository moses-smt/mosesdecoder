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

use MosesRegressionTesting;

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


use Test::More;

######################################
# Definitions of individual test cases
######################################

# A simple English test
&runDetokenizerTest("TEST_ENGLISH_EASY", "en",
<<'TOK',
This sentence is really simple , so it should not be hard to detokenize .
This one is no more difficult , but , hey , it is on a new line .
TOK
<<'EXP'
This sentence is really simple, so it should not be hard to detokenize.
This one is no more difficult, but, hey, it is on a new line.
EXP
);

# A simple French test
&runDetokenizerTest("TEST_FRENCH_EASY", "fr",
<<'TOK',
Ici une phrase simple .
TOK
<<'EXP'
Ici une phrase simple.
EXP
);

######################################
# end of individual test cases
######################################

done_testing();


############
## Utilities
############

sub runDetokenizerTest {
    my ($testName, $language, $tokenizedString, $expectedString) = @_;

    my $testOutputDir = catfile($results_dir, $testName);
    my $tokenizedFile = catfile($testOutputDir, "input.txt");
    my $expectedFile = catfile($testOutputDir, "expected.txt");

    # Fail if we can't make the test output directory
    unless (mkdir($testOutputDir)) {
	fail($testName.": Failed to create output directory ".$testOutputDir." [".$!."]");
	exit;
    }
    
    open TOK, ">".$tokenizedFile;
    binmode TOK, ":utf8";
    print TOK $tokenizedString;
    close TOK;
    
    open TRUTH, ">".$expectedFile;
    binmode TRUTH, ":utf8";
    print TRUTH $expectedString;
    close TRUTH;

    &runTest($testName, $testOutputDir, $tokenizedFile, sub {
	return [$detokenizer, "-l", $language];
    }, sub {
	&verifyIdentical($testName, $expectedFile, catfile($testOutputDir, "stdout.txt"))
    }, 1);
}

# $stdinFile, if defined, is a file to send to the command via STDIN
# $buildCommandRoutineReference is a reference to a zero-argument subroutine that returns the
#                               command to run in the form of an array reference
# $validationRoutineReference is a reference to a zero-argument subroutine that makes some calls
#                             to ok() or similar to validate the contents of the output directory
# $separateStdoutFromStderr is an optional boolean argument; if omitted or false, the command's
#                           STDOUT and STDERR are mixed together in out output file called
#                           stdout-and-stderr.txt; otherwise, they are printed to separate output
#                           files called stdout.txt and stderr.txt, respectively
sub runTest {
    my ($testName, $outputDir, $stdinFile, $buildCommandRoutineReference, $validationRoutineReference, $separateStdoutFromStderr) = @_;

    # Note: You may need to upgrade your version of the Perl module Test::Simple in order to get this 'subtest' thing to work. (Perl modules are installed/upgraded using CPAN; google 'how do I upgrade a perl module')
    subtest $testName => sub {
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
	return unless is($exitStatus, 0, $testName.": command exited with status 0");

	$validationRoutineReference->();
    };
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
