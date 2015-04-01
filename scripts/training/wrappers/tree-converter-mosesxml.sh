#!/usr/bin/env sh

# Usage: tree-converter-mosesxml.sh CMD... < in > out
#
# Wrapper for Travatar's tree-converter that does conversion from Moses XML to
# Penn format, runs tree-converter, then does conversion back to Moses XML.
#
# The tree-converter command must use the 'penn' type for input and output.

`dirname $0`/mosesxml2berkeleyparsed.perl | # Convert to Berkeley format \
sed 's/^(\(.*\))$/\1/' |                    # Strip outer parentheses \
$* |                                        # Run tree-converter \
sed 's/()//' |                              # Remove empty trees (failures) \
sed 's/^(/( (/' |                           # Add opening ( + blank \
sed 's/)$/))/' |                            # Add closing ) \
sed 's/^$/(())/' |                          # Restore empty trees (Berkeley) \
`dirname $0`/berkeleyparsed2mosesxml.perl   # Convert back to Moses XML
