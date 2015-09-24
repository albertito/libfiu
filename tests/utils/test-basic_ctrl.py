#!/usr/bin/env python

import os
import subprocess
import time

def fiu_ctrl(p, args):
    subprocess.check_call("./wrap fiu-ctrl".split() + args + [str(p.pid)])

def launch_sh():
    # We use cat as a subprocess as it is reasonably ubiquitous, simple and
    # straightforward (which helps debugging and troubleshooting), but at the
    # same time it is interactive and we can make it do the operations we
    # want.
    # We also set LC_ALL=C as we test the output for the word "error", which
    # does not necessarily appear in other languages.
    p = subprocess.Popen("./wrap fiu-run -x cat".split(),
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=dict(os.environ, LC_ALL="C"))

    # Give it a moment to initialize and create the control files.
    time.sleep(0.2)
    return p

def send_cmd(p, cmd):
    p.stdin.write(cmd)
    p.stdin.close()

    # Give the control thread a moment to process the command.
    time.sleep(0.2)
    return p.stdout.read(), p.stderr.read()


# Launch a subprocess and check that it shows up in fiu-ls.
p = launch_sh()
out = subprocess.check_output("./wrap fiu-ls".split())
assert ("%s: cat" % p.pid) in out, out

# Send it a command and check that it works.
# Nothing interesting here from libfiu's perspective, but it helps make sure
# the test environment is sane.
out, err = send_cmd(p, "test\n")
assert out == 'test\n', out
assert err == '', err

# Launch and then make I/O fail at runtime.
p = launch_sh()
fiu_ctrl(p, ["-c", "enable name=posix/io/*"])
out, err = send_cmd(p, "test\n")
assert out == '', out
assert 'error' in err, err

# Same, but with failinfo.
p = launch_sh()
fiu_ctrl(p, ["-c", "enable name=posix/io/*,failinfo=3"])
out, err = send_cmd(p, "test\n")
assert out == '', out
assert 'error' in err, err

# Same, but with probability.
p = launch_sh()
fiu_ctrl(p, ["-c", "enable_random name=posix/io/*,probability=0.999"])
out, err = send_cmd(p, "test\n")
assert out == '', out
assert 'error' in err, err

