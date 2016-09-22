#!/usr/bin/env perl

#
# Sample client for mosesserver, illustrating allignment info and
# report all factors
#
use strict;
use Encode;
use XMLRPC::Lite;
use utf8;

binmode(STDIN,  ":utf8");


my $url = "http://localhost:8080/RPC2";
my $proxy = XMLRPC::Lite->proxy($url);

my $text;
while ($text = <STDIN>) {
#for (my $i = 0; $i < scalar(@doc); ++$i) {
#  my $text = $doc[$i];

  # Work-around for XMLRPC::Lite bug
  #my $encoded = SOAP::Data->type(string => Encode::encode("utf8",$text));
  my $encoded = SOAP::Data->type(string => $text);

  my %param = ("text" => $encoded, "align" => "true", "report-all-factors" => "true");
  my $result = $proxy->call("translate",\%param)->result;
  print $result->{'text'} . "\n";
  if ($result->{'align'}) {
      print "Phrase alignments: \n";
      my $aligns = $result->{'align'};
      foreach my $align (@$aligns) {
        print $align->{'tgt-start'} . "," . $align->{'src-start'} . "," 
            . $align->{'src-end'} . "\n"; 
      }
  }
}



