#!/bin/bash

# NOTE:
# Versions 1.9 (or higher) of aclocal and automake are required.
# And version >=2.60 of autoconf
# And version >=1.4.7 of m4

# For Mac OSX users:
# Standard distribution usually includes versions 1.6.
# Get versions 1.9 or higher
# Set the following variable to the correct paths
#ACLOCAL="/path/to/aclocal-1.9"
#AUTOMAKE="/path/to/automake-1.9"

function die () {
  echo "$@" >&2

  # Try to be as helpful as possible by detecting OS and making recommendations
  if (( $(lsb_release -a | fgrep -ci "ubuntu") > 0 )); then
      echo >&2
      echo >&2 "Need to install build autotools on Ubuntu? Use:"
      echo >&2 "sudo aptitude install autoconf automake libtool build-essential"
  fi
  if (( $(uname -a | fgrep -ci "darwin") > 0 )); then
      echo >&2
      echo >&2 "Having problems on Mac OSX?"
      echo >&2 "You might have an old version of aclocal/automake. You'll need to upgrade these."
  fi
  exit 1
}

if [ -z "$ACLOCAL" ]; then
    ACLOCAL=`which aclocal`
    [ -n "$ACLOCAL" ] || die "aclocal not found on your system. Please install it or set $ACLOCAL"
fi

if [ -z "$AUTOMAKE" ]; then
    AUTOMAKE=`which automake`
    [ -n "$AUTOMAKE" ] || die "automake not found on your system. Please install it or set $AUTOMAKE"
fi

if [ -z "$AUTOCONF" ]; then
    AUTOCONF=`which autoconf`
    [ -n "$AUTOCONF" ] || die "autoconf not found on your system. Please install it or set $AUTOCONF"
fi

if [ -z "$LIBTOOLIZE" ]; then
    LIBTOOLIZE=`which libtoolize`

    if [ -z "$LIBTOOLIZE" ]; then
        LIBTOOLIZE=`which glibtoolize`
    fi

    [ -n "$LIBTOOLIZE" ] || die "libtoolize/glibtoolize not found on your system. Please install it or set $LIBTOOLIZE"
fi

echo >&2 "Detected aclocal: $($ACLOCAL --version | head -n1)"
echo >&2 "Detected autoconf: $($AUTOCONF --version | head -n1)"
echo >&2 "Detected automake: $($AUTOMAKE --version | head -n1)"
echo >&2 "Detected libtoolize: $($LIBTOOLIZE --version | head -n1)"

echo "Calling $ACLOCAL -I m4..."
$ACLOCAL -I m4 || die "aclocal failed"

echo "Calling $AUTOCONF..."
$AUTOCONF  || die "autoconf failed"

echo "Calling $LIBTOOLIZE"
$LIBTOOLIZE || die "libtoolize failed"

echo "Calling $AUTOMAKE --add-missing..."
$AUTOMAKE --add-missing || die "automake failed"

case `uname -s` in
    Darwin)
        cores=$(sysctl -n hw.ncpu)
        ;;
    Linux)
        cores=$(cat /proc/cpuinfo | fgrep -c processor)
        ;;
    *)
        echo "Unknown platform."
        cores=
        ;;
esac

if [ -z "$cores" ]; then
    cores=2 # assume 2 cores if we can't figure it out
    echo >&2 "Assuming 2 cores"
else
    echo >&2 "Detected $cores cores"
fi

echo
echo "You should now be able to configure and build:"
echo "   ./configure [--with-srilm=/path/to/srilm] [--with-irstlm=/path/to/irstlm] [--with-randlm=/path/to/randlm] [--with-synlm] [--with-xmlrpc-c=/path/to/xmlrpc-c-config]"
echo "   make -j ${cores}"
echo

