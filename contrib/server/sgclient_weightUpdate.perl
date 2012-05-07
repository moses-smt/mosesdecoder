#!/usr/bin/perl -w
use strict;

use Encode;
use Frontier::Client;

#my $proxy = XMLRPC::Lite->proxy($url);

my $port = "8080";
my $url = "http://localhost:".$port."/RPC2";
my $server = Frontier::Client->new('url' => $url, 'encoding' => 'UTF-8');
my $verbose=0;

my $translations="translations.out";
open TR, ">:utf8", $translations;
my $sg_out="searchGraph.out";
open SG, ">:utf8", $sg_out;

#for (my $i=0;$i<scalar(@SENTENCE);$i++)
my $i=0;
while (my $text = <STDIN>)
{
    my $date = `date`; 
    chop($date); 
    print "[$date] sentence $i: translate\n" if $verbose;
    
    # update weights
    my $core_weights = "0.0314787,-0.138354,1,0.0867223,0.0349965,0.104774,0.0607203,0.0516889,0.113694,0.0947218,0.0642702,0.0385324,0.0560749,0.0434684,0.0805031";
    #my $core_weights = "0.3,-1,1,0.3,0.3,0.3,0.3,0.3,0.3,0.5,0.2,0.2,0.2,0.2,0.2";
    #my $core_weights = "0.031,-0.138,1.000,0.095,0.064,0.039";
    #my $core_weights = "0.001,-0.001,1.000,1,1,1";
    my $sparse_weights = "pp_dummy~dummy=0.001";
    my %param = ("core-weights" => $core_weights, "sparse-weights" => $sparse_weights);
    $server->call("updateWeights",(\%param));

    # translate
    #my %param = ("text" => $server->string($SENTENCE[$i]) , "sg" => "true");
    %param = ("text" => $text, "sg" => "true");
    my $result = $server->call("translate",(\%param));

    $date = `date`; 
    chop($date); 
    print "[$date] sentence $i: process translation\n" if $verbose;

    # process translation
    my $mt_output = Encode::encode('UTF-8',$result->{'text'}); # no idea why that is necessary
    $mt_output =~ s/\|\S+//g; # no multiple factors, only first
    print "sentence $i >> $translations \n";
    print TR $mt_output."\n";

    # print out search graph
    print "sentence $i >> $sg_out \n";
    my $sg_ref = $result->{'sg'};
    foreach my $sgn (@$sg_ref) {
	# print out in extended format
	if ($sgn->{hyp} eq 0) {
	    print SG "$i hyp=$sgn->{hyp} stack=$sgn->{stack} forward=$sgn->{forward} fscore=$sgn->{fscore} \n";
	}
	else {
	    print SG "$i hyp=$sgn->{hyp} stack=$sgn->{stack} back=$sgn->{back} score=$sgn->{score} transition=$sgn->{transition} ";
	    if ($sgn->{"recombined"}) {
		print SG "recombined=$sgn->{recombined} ";
	    }	    
	    print SG "forward=$sgn->{forward} fscore=$sgn->{fscore} covered=$sgn->{'cover-start'}-$sgn->{'cover-end'} ";
	    print SG "scores=\"$sgn->{scores}\" out=\"$sgn->{out}\" \n";
	}
    }

    ++$i;
}

close(SG);
