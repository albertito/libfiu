"""
Tests for the fiu_ctrl.py module.

Note the command line utility is covered by the utils/ tests, not from here,
this is just for the Python module.
"""

import subprocess
import fiu_ctrl
import errno
import time

fiu_ctrl.PLIBPATH = "./libs/"

def run_cat(**kwargs):
    return fiu_ctrl.Subprocess(["./small-cat"],
        stdin = subprocess.PIPE, stdout = subprocess.PIPE,
        stderr = subprocess.PIPE, **kwargs)

# Run without any failure point being enabled.
cmd = run_cat()
p = cmd.start()
out, err = p.communicate('test\n')
assert out == 'test\n', out
assert err == '', err

# Enable before starting.
cmd = run_cat(fiu_enable_posix = True)
cmd.enable('posix/io/rw/*', failinfo = errno.ENOSPC)
p = cmd.start()
out, err = p.communicate('test\n')
assert out == '', out
assert 'space' in err, err

# Enable after starting.
cmd = run_cat(fiu_enable_posix = True)
p = cmd.start()
cmd.enable('posix/io/rw/*', failinfo = errno.ENOSPC)
out, err = p.communicate('test\n')
assert out == '', out
assert 'space' in err, err

# Enable-disable.
cmd = run_cat(fiu_enable_posix = True)
p = cmd.start()
cmd.enable('posix/io/rw/*', failinfo = errno.ENOSPC)
cmd.disable('posix/io/rw/*')
out, err = p.communicate('test\n')
assert out == 'test\n', (out, err)

# Enable random.
# This relies on cat doing a reasonably small number of read and writes, which
# our small-cat does.
result = { True: 0, False: 0 }
for i in range(50):
    cmd = run_cat(fiu_enable_posix = True)
    p = cmd.start()
    cmd.enable_random('posix/io/rw/*', failinfo = errno.ENOSPC,
            probability = 0.5)
    out, err = p.communicate('test\n')
    if 'space' in err:
        result[False] += 1
    elif out == 'test\n':
        result[True] += 1
    else:
        assert False, (out, err)

assert 10 < result[True] < 40, result
assert 10 < result[False] < 40, result

