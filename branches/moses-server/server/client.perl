#!/usr/bin/env perl

use XMLRPC::Lite;

$url = "http://localhost:9084/RPC2";
$proxy = XMLRPC::Lite->proxy($url);

#my %param = ("text" => "das ist ein haus das ist ein haus das ist ein haus");
#my %param = ("text" => "je ne sais pas . ");
#my %param = ("text" => "actes pris en application des traités ce  euratom dont la publication est obligatoire");
#my %param = ("text" => "actes pris en application des " );
#my %param = ("text" => "je ne sais pas . ", "align" => "true");
my %param = ("text" => "hello !");
$result = $proxy->call("translate",\%param)->result;
print $result->{'text'} . "\n";
if ($result->{'align'}) {
    print "Phrase alignments: \n";
    $aligns = $result->{'align'};
    foreach my $align (@$aligns) {
        print $align->{'tgt-start'} . "," . $align->{'src-start'} . "," 
            . $align->{'src-end'} . "\n"; 
    }
}
