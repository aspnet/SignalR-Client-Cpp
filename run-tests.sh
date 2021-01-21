#!/bin/bash

echo "Running executable"

PID=0
EXITCODE=0

# launch process in background
eval $* &
# capture process ID to monitor
PID=$!

# Timeout duration
TIME=240

while (($TIME > 0)); do
  # polling interval
  sleep 5

  # check if process is still running
  ps ax | grep $PID | grep -v grep > /dev/null
  if test "$?" != "0"
  then
    # process exited so lets get the exit code
    wait $PID
    EXITCODE=$?
    echo "Finished running with exit code: "$EXITCODE
    exit $EXITCODE
  fi
  ((TIME -= 5))
done

# process taking too long, kill it
kill -9 $PID

echo "Process took too long"
exit 1
