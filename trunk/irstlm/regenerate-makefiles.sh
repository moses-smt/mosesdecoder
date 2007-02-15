#!/bin/bash

function die () {
  echo "$@" >&2
  exit 1
}

echo "Calling aclocal..."
aclocal || die "aclocal failed"
echo "Calling autoconf..."
autoconf || die "autoconf failed"
echo "Calling automake..."
automake || die "automake failed"

echo
echo "You should now be able to configure and build:"
echo "   ./configure --prefix=/path/to/irstlm"
echo "   make -j 4"
echo

