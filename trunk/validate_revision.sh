#!/bin/bash

rev="$1"

if [ "$rev" == "" ]; then
  cat << KONEC
./validate_revision.sh <revnumber>
  This will check, if the given revision was compilable (using irstlm).
  These tasks will be performed:
    svn update -r <revnumber>
    compile and install irstlm to a temp directory
    compile moses with irstlm
    delete the temp directory
KONEC
  exit 1;
fi

tempdir=/tmp/validatemoses

function die() {
  rm -rf $tempdir
  echo "$@"
  exit 1
}

svn up -r $rev || die "Failed to update to rev. $rev"
# dump the information
svn info

./regenerate-makefiles.sh || die "Failed to regenerate makefiles in mosesdecoder"


cd irstlm || die "Failed to chdir to irstlm"
./regenerate-makefiles.sh || die "Failed to regenerate makefiles in irstlm"
./configure --prefix=$tempdir/irstlm || die "Failed to configure irstlm"
make || die "Failed to compile irstlm"
make install || die "Failed to install irstlm"
cd ..

./configure --with-irstlm=$tempdir/irstlm || die "Failed to configure moses"
make || die "Failed to compile moses"
  
rm -rf $tempdir || die "Failed to remove tempdir $tempdir"
echo "Moses successfully compiled"
