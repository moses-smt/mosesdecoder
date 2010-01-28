#! /usr/bin/perl

use strict;

my %opt = ();
use Getopt::Long "GetOptions";

sub safesystem {
  print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
      print STDERR "Failed to execute: @_\n  $!\n";
      exit(1);
  }
  elsif ($? & 127) {
      printf STDERR "Execution of: @_\n  died with signal %d, %s coredump\n",
          ($? & 127),  ($? & 128) ? 'with' : 'without';
      exit(1);
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}

sub init(){

# Command line options processing
#
    GetOptions(
	       "version"=> sub { VersionMessage() },
	       "help" => sub { HelpMessage() },
	       "debug:i" => \$opt{debug},
	       "verbose:i" =>\$opt{verbose},
	       "config=s" =>\$opt{config},
	       "i=s" => \$opt{inputfile},
	       "inputfile=s" => \$opt{inputfile},
	       "inputtype=s" => \$opt{inputtype},
	       'w=s' => \$opt{w},
	       'lm=s' => \$opt{lm},
	       'tm=s' => \$opt{tm},
	       'd=s' => \$opt{d},
	       'I=s' => \$opt{I},
	       'n-best-list=s' => \$opt{nbestlist},
	      );

    DebugMessage("Debugging is level $opt{debug}.") if defined($opt{debug});
    VerboseMessage("Verbose level is $opt{verbose}.") if defined($opt{verbose});
    print_parameters() if defined($opt{verbose}) && $opt{verbose} > 1;
}

sub VersionMessage(){
    print STDERR "moses-virtual version 1.0\n"; 
    exit;
}

sub HelpMessage(){
    print STDERR "moses-virtual simulates the standard behavior of Moses\n"; 
    print STDERR "USAGE: moses-virtual\n"; 
    print_parameters(1);
    exit;
}

sub DebugMessage(){
  my ($msg) = @_;
  print STDERR "Debug: $msg\n";
}

sub VerboseMessage(){
  my ($msg) = @_;
  print STDERR "Verbose: $msg\n";
}

sub print_parameters(){
  my ($all) = @_;

  print STDERR "Parameters:\n";
  if ($all){
    foreach (sort keys %opt){
      print STDERR "-$_\n";
    }
  }else{
    foreach (sort keys %opt){
      print STDERR "-$_=$opt{$_}\n" if defined($opt{$_});
    }
  }

  print STDERR "pass_through parameters: @ARGV\n" if $#ARGV>=0;
}


######################
### Script starts here

### init() reads prameters from the command line
### you always have to call it
init();

my $pwd =`pwd`;
chomp($pwd);
my $archive_list = "$pwd/archive.list";
my $actual_index = "$pwd/actual.index";
print STDERR "archivelist: $archive_list\n";
print STDERR "actualindex is taken from: $actual_index\n";


my $index=0;
if (-e $actual_index){
        open(IN,"$actual_index");
        $index=<IN>; chomp($index);
        close(IN);
}
print STDERR "actualindex: $index\n";


open(IN,"$archive_list");
my ($out,$nbest);
for (my $i=0;$i<=$index;$i++){
        chomp($_=<IN>);
        ($out,$nbest) = split(/[ \t]+/,$_);
}
close(IN);

die "output filename is empty\n" if $out eq "";
die "nbest filename is empty\n" if $nbest eq "";

print STDERR "out: |$out|\n";
print STDERR "nbest: |$nbest|\n";

$opt{nbestlist} =~ s/\"//g;
my ($nbestfile,$nbestsize) = split(/\|/,$opt{nbestlist});
print STDERR "n-best-list: |",$opt{nbestlist},"|\n";
print STDERR "nbestfile: |$nbestfile|\n";
print STDERR "nbestsize: |$nbestsize|\n";

open(OUT,">$actual_index");
$index++;
print OUT "$index\n";
close(IN);

safesystem("cp $pwd/$nbest $nbestfile");
safesystem("cat $pwd/$out");


