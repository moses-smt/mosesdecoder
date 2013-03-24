#!/usr/bin/env python
# -*- coding: utf-8 -*-

# python port of client.perl

import xmlrpclib
import datetime

url = "http://localhost:8080/RPC2"
proxy = xmlrpclib.ServerProxy(url)

text = u"il a souhaité que la présidence trace à nice le chemin pour l' avenir ."
params = {"text":text, "align":"true", "report-all-factors":"true"}

result = proxy.translate(params)
print result['text']
if 'align' in result:
    print "Phrase alignments:"
    aligns = result['align']
    for align in aligns:
        print "%s,%s,%s" %(align['tgt-start'], align['src-start'], align['src-end'])
