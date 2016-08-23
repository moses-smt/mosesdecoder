#!/usr/bin/env python
# -*- coding: utf-8 -*-

# update the suffix-array phrasetable via XMLRPC

import xmlrpclib
import datetime

url = "http://localhost:8080/RPC2"
proxy = xmlrpclib.ServerProxy(url)

text = u"il a souhaité que la présidence trace à nice le chemin pour l' avenir ."

source = "Mein kleines Puppenhaus ."
target = "My small doll house ."
#align = "1-1 2-2 3-3 3-4 5-4"
align = "0-0 1-1 2-2 2-3 3-4"

params = {"source":source, "target":target, "alignment":align}
print "Updating with %s ..." %params

result = proxy.updater(params)
print result
