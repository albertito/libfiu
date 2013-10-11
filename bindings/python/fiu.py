
"""
libfiu python wrapper

This module is a wrapper for the libfiu, the fault injection C library.

It provides an almost one-to-one mapping of the libfiu functions, although its
primary use is to be able to test C code from within Python.

For fault injection in Python, a native library would be more suitable.

See libfiu's manpage for more detailed documentation.
"""

import fiu_ll as _ll


def fail(name):
	"Returns the failure status of the given point of failure."
	return _ll.fail(name)

def failinfo(name):
	"""Returns the information associated with the last failure. Use with
	care, can be fatal if the point of failure was not enabled via
	Python."""
	return _ll.failinfo()

class Flags:
	"""Contains the valid flag constants.

	ONETIME: This point of failure is disabled immediately after failing once.
	"""
	ONETIME = _ll.FIU_ONETIME


# To be sure failinfo doesn't dissapear from under our feet, we keep a
# name -> failinfo table. See fiu_ll's comments for more details.
_fi_table = {}

def enable(name, failnum = 1, failinfo = None, flags = 0):
	"Enables the given point of failure."
	_fi_table[name] = failinfo
	r = _ll.enable(name, failnum, failinfo, flags)
	if r != 0:
		del _fi_table[name]
		raise RuntimeError(r)

def enable_random(name, probability, failnum = 1, failinfo = None, flags = 0):
	"Enables the given point of failure, with the given probability."
	_fi_table[name] = failinfo
	r = _ll.enable_random(name, failnum, failinfo, flags, probability)
	if r != 0:
		del _fi_table[name]
		raise RuntimeError(r)

def enable_external(name, cb, failnum = 1, flags = 0):
	"""Enables the given point of failure, leaving the decision whether to
	fail or not to the given external function, which should return 0 if
	it is not to fail, or 1 otherwise.

	The cb parameter is a Python function that takes three parameters,
	name, failnum and flags, with the same values that we receive.

	For technical limitations, enable_external() cannot take
	failinfo."""
	# in this case, use the table to prevent the function from
	# dissapearing
	_fi_table[name] = cb
	r = _ll.enable_external(name, failnum, flags, cb)
	if r != 0:
		del _fi_table[name]
		raise RuntimeError(r)

def enable_stack_by_name(name, func_name,
		failnum = 1, failinfo = None, flags = 0,
		pos_in_stack = -1):
	"""Enables the given point of failure, but only if 'func_name' is in
	the stack.

	'func_name' is be the name of the C function to look for.
	"""
	_fi_table[name] = failinfo
	r = _ll.enable_stack_by_name(name, failnum, failinfo, flags,
			func_name, pos_in_stack)
	if r != 0:
		del _fi_table[name]
		raise RuntimeError(r)

def disable(name):
	"""Disables the given point of failure, undoing the actions of the
	enable*() functions."""
	if name in _fi_table:
		del _fi_table[name]
	r = _ll.disable(name)
	if r != 0:
		raise RuntimeError(r)

def rc_fifo(basename):
	"""Enables remote control over a named pipe that begins with the given
	basename. The final path will be "basename-$PID"."""
	r = _ll.rc_fifo(basename)
	if r != 0:
		raise RuntimeError(r)

