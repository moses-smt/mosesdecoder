#!/bin/bash

if [ -z $1 ]; then
  echo please specify a tmp directory
  exit 1
fi

cd $1
if [ $? -gt 0 ]; then
  echo could not chdir to $1
  exit 1
fi

rm -rf release
mkdir -p release
cd release

base=`pwd`
cvs -z3 -d:pserver:anonymous@mosesdecoder.cvs.sourceforge.net:/cvsroot/mosesdecoder co .

for dir in moses moses-cmd irstlm; do
  cd $base
  cd $dir
  aclocal
  autoconf
  automake
  rm -rf aclocal.m4 autom4te.cache/
  rm -f moses-cmd.vcproj
  rm -f conf26031.sh
  rm -f config.h
  rm -f config.log
  rm -f config.status
  rm -f Makefile
  rm -f stamp-h1
  rm -f moses.sln
  rm -f moses.vcproj
  rm -rf Release
  rm -rf ReleaseNBest/
  rm -f regenerate-makefiles.sh
  rm -rf config
  find . -type d | grep CVS | xargs rm -rf
done

