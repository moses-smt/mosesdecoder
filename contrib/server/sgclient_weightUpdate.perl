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
open(TR, ">$translations");
my $sg_out="seachGraph.out";
open(SG, ">$sg_out"); 

#for (my $i=0;$i<scalar(@SENTENCE);$i++)
my $i=0;
while (my $text = <STDIN>)
{
    my $date = `date`; 
    chop($date); 
    print "[$date] sentence $i: translate\n" if $verbose;
    
    # update weights
    #my $core_weights = "0.031,-0.138,1.000,0.087,0.035,0.105,0.061,0.052,0.114,0.095,0.064,0.039,0.056,0.043,0.081";
    #my $core_weights = "0.031,-0.138,1.000,0.095,0.064,0.039";
    my $core_weights = "0.001,-0.001,1.000,1,1,1";
    my $sparse_weights = "pp_europea~European=0.015 pp_es~is=0.03 pp_es,=~is,equal=0.0001";
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
