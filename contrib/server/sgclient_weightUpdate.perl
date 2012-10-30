#!/usr/bin/perl -w
use strict;
use Frontier::Client;

my $output_suffix = $ARGV[0];
$output_suffix = "" if (not $output_suffix);

my $port = "50015";
my $url = "http://localhost:".$port."/RPC2";
my $server = Frontier::Client->new('url' => $url, 'encoding' => 'UTF-8');
my $verbose=0;

my $translations="translations$output_suffix.out";
open TR, ">:utf8", $translations;
my $sg_out="searchGraph$output_suffix.out";
open SG, ">:utf8", $sg_out;

my $i=0;
while (my $text = <STDIN>)
{
    my $date = `date`; 
    chop($date); 
    print "[$date] sentence $i: translate\n" if $verbose;
    
    # update weights
    my $core_weights = "0.0314787,-0.138354,1,0.0867223,0.0349965,0.104774,0.0607203,0.0516889,0.113694,0.0947218,0.0642702,0.0385324,0.0560749,0.0434684,0.0805031";
    #my $core_weights = "0.0314787,-0.138354,1,0.0867223,0.0349965,0.104774,0.0607203,0.0516889,0.113694,0.0947218,0.0642702,0.0385324,0.0560749,0.0434684,0.0805031,0";
    #my $sparse_weights = "pp_dummy~dummy=0.001";
    my $sparse_weights = "";
    my %param = ("core-weights" => $core_weights, "sparse-weights" => $sparse_weights);
    $server->call("setWeights",(\%param));

    #my $core_weight_update = "0.1,0.1,0,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1";
    #my $sparse_weight_update = "pp_dummy~dummy=0.1";
    #my %param_update = ("core-weights" => $core_weight_update, "sparse-weights" => $sparse_weight_update);
    #$server->call("addWeights",(\%param_update));

    # translate
    #my %param = ("text" => $server->string($SENTENCE[$i]) , "sg" => "true");
    %param = ("text" => $text, "id" => $i, "sg" => "true");
    my $result = $server->call("translate",(\%param));

    $date = `date`; 
    chop($date); 
    print "[$date] sentence $i: process translation\n" if $verbose;

    # process translation
    my $mt_output = $result->{'text'};
    $mt_output =~ s/\|\S+//g; # no multiple factors, only first
    print "sentence $i >> $translations \n";
    print TR $mt_output."\n";

    # print out search graph
    print "sentence $i >> $sg_out \n";
    my $sg_ref = $result->{'sg'};
    foreach my $sgn (@$sg_ref) {
	# print out in extended format
	if ($sgn->{hyp} eq 0) {
	    print SG "$i hyp=$sgn->{'hyp'} stack=$sgn->{'stack'} forward=$sgn->{'forward'} fscore=$sgn->{'fscore'} \n";
	}
	else {
	    print SG "$i hyp=$sgn->{'hyp'} stack=$sgn->{'stack'} back=$sgn->{'back'} score=$sgn->{'score'} transition=$sgn->{'transition'} ";
	    if ($sgn->{"recombined"}) {
		print SG "recombined=$sgn->{'recombined'} ";
	    }	    
	    print SG "forward=$sgn->{'forward'} fscore=$sgn->{'fscore'} covered=$sgn->{'cover-start'}-$sgn->{'cover-end'} ";
	    print SG "scores=\"$sgn->{'scores'}\" src-phrase=\"$sgn->{'src-phrase'}\" tgt-phrase=\"$sgn->{'tgt-phrase'}\" \n";
	}
    }

    ++$i;
}

close(SG);
