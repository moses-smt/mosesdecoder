#!/bin/bash -v

ssh -M -S /tmp/gittunnel.ctl -L 9996:github.com:22 junczys@XXX -N -f -v
