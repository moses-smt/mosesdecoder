#!/bin/sh

function die () {
  echo "$@" >&2
  exit 1
}

if [ -z "$ACLOCAL" ]
then
    ACLOCAL=`which aclocal`
fi

if [ -z "$AUTOMAKE" ]
then
    AUTOMAKE=`which automake`
fi

if [ -z "$AUTOCONF" ]
then
    AUTOCONF=`which autoconf`
fi


echo "Calling $ACLOCAL..."
$ACLOCAL || die "aclocal failed"
echo "Calling $AUTOCONF..."
$AUTOCONF || die "autoconf failed"
echo "Calling $AUTOMAKE..."
$AUTOMAKE || die "automake failed"

echo
echo "You should now be able to configure and build:"
echo "   ./configure --with-srilm=/path/to/srilm"
echo "   make -j 4"
echo

