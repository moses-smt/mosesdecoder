#!/usr/bin/env perl

use XMLRPC::Lite;

$url = "http://localhost:1234/RPC2";
$proxy = XMLRPC::Lite->proxy($url);

#my %param = ("text" => "das ist ein haus das ist ein haus das ist ein haus");
my %param = ("text" => "je ne sais pas . ");
$result = $proxy->call("translate",\%param)->result;
print $result->{'text'} . "\n";
