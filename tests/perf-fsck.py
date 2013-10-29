#!/usr/bin/env python

"""
Performance tests using fsck.ext2 on a test file.

It can be tuned with the following environment variables:

 - TEST_FILE: The name of the file used for testing. By default, ".test_fs".
 - TEST_FILE_SIZE_MB: The size of the test file, in megabytes. Only used if
   the file doesn't exist. Default: 10.
 - VERBOSE: Show verbose output (from 0 to 2). Default: 0.
 - LD_LIBRARY_PATH: Library path to pass on to the subcommands.
   Default: <directory of this file>/../libfiu/, which is usually the one
   containing the current build.
"""

import os
import sys
import subprocess
import time

test_file = os.environ.get('TEST_FILE', '.test_fs')

test_file_size = int(os.environ.get('TEST_FILE_SIZE_MB', 10))

ld_library_path = os.environ.get('LD_LIBRARY_PATH',
        os.path.abspath(
            os.path.abspath(os.path.dirname(sys.argv[0]))
            + '/../libfiu/'))

verbose = int(os.environ.get('VERBOSE', 0))

dev_null = open('/dev/null', 'w')

def run_child(name, args, stdout = dev_null, stderr = dev_null):
    """Runs the subprocess, returns the Popen object."""
    env = dict(os.environ)
    env['LD_LIBRARY_PATH'] = ld_library_path

    if verbose and stdout == stderr == dev_null:
        stdout = stderr = None

    child = subprocess.Popen(args, env = env,
            stdout = stdout, stderr = stderr)
    return child

def run_and_time(name, args):
    """Run the given arguments, print the times."""
    start = time.time()
    child = run_child(name, args)
    _, status, rusage = os.wait4(child.pid, 0)
    end = time.time()

    if verbose == 2:
        print 'Ran %s -> %d' % (args, status)

    if status != 0:
        print 'Error running %s: %s' % (args[0], status)
        raise RuntimeError

    print '%-10s u:%.3f  s:%.3f  r:%.3f' % (
            name, rusage.ru_utime, rusage.ru_stime, end - start)


def run_fsck(name, fiu_args):
    """Runs an fsck with the given fiu arguments."""
    child = run_child(name,
            ["fiu-run", "-x"] + fiu_args
                + "fsck.ext2 -n -t -f".split() + [test_file],
                stdout = subprocess.PIPE,
                stderr = subprocess.PIPE)
    stdout, stderr = child.communicate()

    if child.returncode != 0:
        print 'Error running fsck: %s' % child.returncode
        raise RuntimeError

    # Find the times reported by fsck.
    # Not very robust, but useful as it measures the real program time run,
    # and not the startup overhead.
    # The line looks like:
    #   Memory used: 560k/0k (387k/174k), time:  2.18/ 2.17/ 0.00
    user_time = sys_time = real_time = -1
    for l in stdout.split('\n'):
        if not l.startswith("Memory used"):
            continue

        times = l.split(':')[-1].split('/')
        times = [s.strip() for s in times]
        if len(times) != 3:
            continue

        real_time = float(times[0])
        user_time = float(times[1])
        sys_time = float(times[2])
        break

    print '%-10s u:%.3f  s:%.3f  r:%.3f' % (
            name, user_time, sys_time, real_time)


def check_test_file():
    if os.path.exists(test_file):
        return

    with open(test_file, 'w') as fd:
        fd.truncate(test_file_size * 1024 * 1024)

    retcode = subprocess.call(
            ["mkfs.ext2", "-F", test_file],
            stdout = open('/dev/null', 'w'))
    if retcode != 0:
        print 'Error running mkfs.ext2:', retcode
        return


if __name__ == '__main__':
    check_test_file()

    run_and_time("base", "fsck.ext2 -n -f".split() + [test_file])

    # 1 all-matching wildcard.
    run_fsck("w1", ["-c", "enable_random name=*,probability=0"])

    # 1k final failure points, no matches.
    args = []
    for i in range(1000):
        args += ["-c", "enable_random name=none/%d,probability=0" % i]
    run_fsck("f1k", args)

    # 1k wildcard failure points, no matches.
    args = []
    for i in range(1000):
        args += ["-c", "enable_random name=none/%d/*,probability=0" % i]
    run_fsck("w1k", args)

    # 1k wildcarded failure points, and 1 match.
    args = []
    for i in range(1000):
        args += ["-c", "enable_random name=none/%d/*,probability=0" % i]
    args += ["-c", "enable_random name=*,probability=0"]
    run_fsck("w1k+1", args)

    # 1k final failure points, *all* matches.
    args = []
    for i in range(1000):
        args += ["-c", "enable_random name=*,probability=0"]
    run_fsck("m1k", args)
