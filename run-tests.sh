#!/bin/bash

echo "Running executable"
if [ "$(uname)" = "Linux" ]; then
  strace $*
else
  eval $*
fi
EXITCODE=$?
echo "Finished running with exit code: " $EXITCODE
exit $EXITCODE
