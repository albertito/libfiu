#!/bin/bash

set -ex

# Very basic test, nothing enabled.
./wrap fiu-run true

# Same as above, but including the posix preloader.
./wrap fiu-run -x true

# Now with an unused failure point.
./wrap fiu-run -c "enable name=p1" true

# open() failure.
! ./wrap fiu-run -x -c "enable name=posix/io/oc/open" cat /dev/null


