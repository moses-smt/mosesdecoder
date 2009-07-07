#!/usr/bin/env perl

use XMLRPC::Lite;

$url = "http://localhost:8080/RPC2";
$proxy = XMLRPC::Lite->proxy($url);

my %param = ("text" => "das ist ein haus das ist ein haus das ist ein haus");
$result = $proxy->call("translate",\%param)->result;
print $result->{'text'} . "\n";
