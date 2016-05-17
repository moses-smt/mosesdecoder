#!/usr/bin/env python
# -*- coding: utf-8 -*-
# python port of client.perl
# Author: Ulrich Germann
import xmlrpclib, requests, json, datetime, sys

url = "http://%s/RPC2"%sys.argv[1]
proxy = xmlrpclib.ServerProxy(url)

text = sys.stdin.readlines()
params = {}
for t in text:
    params["text"] = t
    result = proxy.translate(params)
    print result['text']
