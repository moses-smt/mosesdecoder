#!/usr/bin/env perl

use XMLRPC::Lite;

$url = "http://localhost:8080/RPC2";
$proxy = XMLRPC::Lite->proxy($url);

#my %param = ("text" => "das ist ein haus das ist ein haus das ist ein haus");
my %param = ("text" => "je ne sais pas . ", "align" => "true");
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
