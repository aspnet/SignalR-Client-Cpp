#!/bin/bash

echo "Running executable"
eval $*
EXITCODE=$?
echo "Finished running with exit code: " $EXITCODE
exit $EXITCODE
