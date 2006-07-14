#!/bin/sh

echo "Calling autoconf..."
autoconf
echo "Calling automake..."
automake

echo
echo "You should now be able to configure and build:"
echo "   env LDFLAGS=-static ./configure --with-boost=/home/ws06/cdyer/boost-stage --with-srilm=/home/ws06/cdyer/srilm --with-moses=\`pwd\`/../moses"
echo "   make -j 4"
echo
