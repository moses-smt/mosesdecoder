#!/usr/bin/env perl

#
# Sample client for mosesserver, illustrating allignment info and
# report all factors
#
use strict;
use Encode;
use XMLRPC::Lite;
use utf8;

my $url = "http://localhost:8080/RPC2";
my $proxy = XMLRPC::Lite->proxy($url);

my @doc = ("monsieur le président , ce que nous devrons toutefois également faire à biarritz , c' est regarder un peu plus loin .",
	"les élus que nous sommes avons au moins autant le devoir de l' encourager à progresser , en dépit de l' adversité , que de relayer les messages que nous recevons de l' opinion publique dans chacun de nos pays .",
	"au regard des événements de ces derniers temps , la question du prix de l' essence me semble elle aussi particulièrement remarquable .",
	"à l' heure actuelle , le conseil est en train d' examiner l' inclusion de tels mécanismes dans l' article 7 .",
	"deuxièmement , dans la transparence pour les citoyens , qui connaissent à présent les droits dont ils disposent vis-à-vis de ceux qui appliquent et élaborent le droit européen , et pour ceux qui , justement , appliquent et élaborent ce droit européen .");
#print STDERR scalar(@doc);

for (my $i = 0; $i < scalar(@doc); ++$i) {
  my $text = $doc[$i];

  # Work-around for XMLRPC::Lite bug
  my $encoded = SOAP::Data->type(string => Encode::encode("utf8",$text));
  #$encoded = SOAP::Data->type(string => $text);

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



