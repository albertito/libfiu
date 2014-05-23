"""
libfiu python module for remote control

This module provides an easy way to run a command with libfiu enabled, and
controlling the failure points dynamically.

It provides similar functionality to the fiu-ctrl and fiu-run shell tools, but
is useful for programmed tests.

Note it assumes the preloading libraries are installed in @@PLIBPATH@@.
"""

import os
import tempfile
import subprocess
import shutil
import time


# Default path to look for preloader libraries.
PLIBPATH = "@@PLIBPATH@@"


class CommandError (RuntimeError):
    """There was an error running the command."""
    pass


class Flags:
	"""Contains the valid flag constants.

	ONETIME: This point of failure is disabled immediately after failing once.
	"""
	ONETIME = "onetime"


class _ControlBase (object):
    """Base class for remote control objects."""

    def run_raw_cmd(self, cmd, args):
        """Runs a new raw command. To be implemented by subclasses"""
        raise NotImplementedError

    def _basic_args(self, name, failnum, failinfo, flags):
        """Converts the common arguments to an args list for run_raw_cmd()."""
        args = ["name=%s" % name]
        if failnum:
            args.append("failnum=%s" % failnum)
        if failinfo:
            args.append("failinfo=%s" % failinfo)
        if flags:
            args.extend(flags)

        return args

    def enable(self, name, failnum = 1, failinfo = None, flags = ()):
        """Enables the given point of failure."""
        args = self._basic_args(name, failnum, failinfo, flags)
        self.run_raw_cmd("enable", args)

    def enable_random(self, name, probability, failnum = 1,
            failinfo = None, flags = ()):
        "Enables the given point of failure, with the given probability."
        args = self._basic_args(name, failnum, failinfo, flags)
        args.append("probability=%f" % probability)
        self.run_raw_cmd("enable_random", args)

    def enable_stack_by_name(self, name, func_name,
            failnum = 1, failinfo = None, flags = (),
            pos_in_stack = -1):
        """Enables the given point of failure, but only if 'func_name' is in
        the stack.

        'func_name' is be the name of the C function to look for.
        """
        args = self._basic_args(name, failnum, failinfo, flags)
        args.append("func_name=%s" % func_name)
        if pos_in_stack >= 0:
            args.append("pos_in_stack=%d" % pos_in_stack)
        self.run_raw_cmd("enable_stack_by_name", args)

    def disable(self, name):
        """Disables the given point of failure."""
        self.run_raw_cmd("disable", ["name=%s" % name])


def _open_with_timeout(path, mode, timeout = 3):
    """Open a file, waiting if it doesn't exist yet."""
    deadline = time.time() + timeout
    while not os.path.exists(path):
        time.sleep(0.01)
        if time.time() >= deadline:
            raise RuntimeError("Timeout waiting for file %r" % path)

    return open(path, mode)


class PipeControl (_ControlBase):
    """Control pipe used to control a libfiu-instrumented process."""
    def __init__(self, path_prefix):
        """Constructor.

        Args:
            path: Path to the control pipe.
        """
        self.path_in = path_prefix + ".in"
        self.path_out = path_prefix + ".out"

    def _open_pipes(self):
        # Open the files, but wait if they are not there, as the child process
        # may not have created them yet.
        fd_in = _open_with_timeout(self.path_in, "a")
        fd_out = _open_with_timeout(self.path_out, "r")
        return fd_in, fd_out

    def run_raw_cmd(self, cmd, args):
        """Send a raw command over the pipe."""
        # Note we open the pipe each time for simplicity, and also to simplify
        # external intervention that can be used for debugging.
        fd_in, fd_out = self._open_pipes()

        s = "%s %s\n" % (cmd, ','.join(args))
        fd_in.write(s)
        fd_in.flush()

        r = int(fd_out.readline())
        if r != 0:
            raise CommandError


class EnvironmentControl (_ControlBase):
    """Pre-execution environment control."""
    def __init__(self):
        self.env = ""

    def run_raw_cmd(self, cmd, args):
        """Add a raw command to the environment."""
        self.env += "%s %s\n" % (cmd, ','.join(args))


class Subprocess (_ControlBase):
    """Wrapper for subprocess.Popen, but without immediate execution.

    This class provides a wrapper for subprocess.Popen, which can be used to
    run other processes under libfiu.

    However, the processes don't start straight away, allowing the user to
    pre-configure some failure points.

    The process can then be started with the start() method.

    After the process has been started, the failure points can be controlled
    remotely via the same functions.

    Processes can be started only once.
    Note that using shell=True is not recommended, as it makes the pid of the
    controlled process to be unknown.
    """
    def __init__(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs

        # Note this removes fiu_enable_posix from kwargs if it's there, that
        # way kwargs remains "clean" for passing to Popen.
        self.fiu_enable_posix = kwargs.pop('fiu_enable_posix', False)

        self._proc = None
        self.tmpdir = None

        # Initially, this is an EnvironmentControl so we can do preparation;
        # once we start the command, we will change this to be PipeControl.
        self.ctrl = EnvironmentControl()

    def run_raw_cmd(self, cmd, args):
        self.ctrl.run_raw_cmd(cmd, args)

    def start(self):
        self.tmpdir = tempfile.mkdtemp(prefix = 'fiu_ctrl-')

        env = os.environ
        env['LD_PRELOAD'] = env.get('LD_PRELOAD', '')
        if self.fiu_enable_posix:
            env['LD_PRELOAD'] += ' ' + PLIBPATH + '/fiu_posix_preload.so'
        env['LD_PRELOAD'] += ' ' + PLIBPATH + '/fiu_run_preload.so '
        env['FIU_CTRL_FIFO'] = self.tmpdir + '/ctrl-fifo'
        env['FIU_ENABLE'] = self.ctrl.env

        self._proc = subprocess.Popen(*self.args, **self.kwargs)

        fifo_path = "%s-%d" % (env['FIU_CTRL_FIFO'], self._proc.pid)
        self.ctrl = PipeControl(fifo_path)

        return self._proc

    def __del__(self):
        # Remove the temporary directory.
        # The "'fiu_ctrl-' in self.tmpdir" check is just a safeguard.
        if self.tmpdir and 'fiu_ctrl-' in self.tmpdir:
            shutil.rmtree(self.tmpdir)

