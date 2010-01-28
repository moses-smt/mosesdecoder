#!/bin/bash

if [ -z $1 ]; then
  echo please specify a tmp directory
  exit 1
fi

cd $1
topdir=`pwd`
if [ $? -gt 0 ]; then
  echo could not chdir to $1
  exit 1
fi


rm -rf release
mkdir -p release
cd release

svn co https://svn.sourceforge.net/svnroot/mosesdecoder/trunk mosesdecoder

cd mosesdecoder
base=`pwd`

aclocal
# add AM_MAINTAINER_MODE

perl -e '$c=0; while(<>) { print; $c++; if ($c==5) {print "AM_MAINTAINER_MODE\n"; } }' < configure.in > conf.tmp
mv conf.tmp configure.in

autoconf
automake
rm -f Makefile
rm -f stamp-h1
rm -f regenerate-makefiles.sh
rm -rf aclocal.m4 autom4te.cache/
find . -type d | grep .svn | xargs rm -rf

cd irstlm
aclocal
autoconf
automake
rm -f Makefile
rm -f stamp-h1
rm -f regenerate-makefiles.sh
rm -rf aclocal.m4 autom4te.cache/
cd ..

for dir in moses moses-cmd irstlm; do
  cd $base
  cd $dir
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
  rm -rf config
  rm -rf .*
  rm -f acsite*
done

cd $base
tar cf moses-release.tar moses/ moses-cmd/ irstlm/ BUILD-INSTRUCTIONS configure Makefile.in Makefile.am install-sh config.h.in depcomp
gzip moses-release.tar
mv moses-release.tar.gz $topdir/moses-release.tar.gz
cd $topdir

echo tar-ball: $topdir/moses-release.tar.gz
echo Don\'t forget to remove $topdir/release

