#!/usr/bin/env perl

#
# Sample client to print out the search graph
#

use Encode;
use XMLRPC::Lite;
use utf8;

$url = "http://localhost:8086/RPC2";
$proxy = XMLRPC::Lite->proxy($url);

$text = "il a souhaité que la présidence trace à nice le chemin pour l' avenir .";
#$text = "je ne sais pas";

# Work-around for XMLRPC::Lite bug
$encoded = SOAP::Data->type(string => Encode::encode("utf8",$text));

#my %param = ("text" => $encoded );
my %param = ("text" => $encoded , "sg" => "true", "topt" => "true");
#my %param = ("text" => $encoded , "topt" => "true");
die "translation failed" unless $result = $proxy->call("translate",\%param)->result;
print $result->{'text'} . "\n";
exit;

if ($result->{'sg'}) {
    print "Search graph: \n";
    $sg = $result->{'sg'};
    foreach my $sgn (@$sg) {
        foreach my $key (keys %$sgn) {
            my $value = $sgn->{$key};
            print "$key=$value ";
        }
        print "\n";
    }
}

if ($result->{'topt'}) {
    print "Translation options: \n";
    $sg = $result->{'topt'};
    foreach my $sgn (@$sg) {
        foreach my $key (keys %$sgn) {
            my $value = $sgn->{$key};
            if (ref($value) eq 'ARRAY') {
                $value = join(",", @$value);
            }
            print "$key=$value ";
        }
        print "\n";
    }
}
