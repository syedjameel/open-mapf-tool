#!/bin/sh
# Fake CBS solver: speaks just enough of the CBSH2-RTC CLI to exercise the
# whole OpenMAPF experiment pipeline without any real solver installed.
# It "solves" every instance in 0.1s with a canned result.
out=""; paths=""
while [ $# -gt 0 ]; do
  case "$1" in
    -o) out="$2"; shift 2;;
    --outputPaths=*) paths="${1#--outputPaths=}"; shift;;
    *) shift;;
  esac
done
echo 'runtime,#high-level expanded,#high-level generated,#low-level expanded,#low-level generated,solution cost' > "$out"
echo '0.1,3,5,20,40,9' >> "$out"
printf 'Agent 0: (0,0)->(1,1)\n' > "$paths"
