
# libfiu - Fault injection in userspace

[libfiu](https://blitiri.com.ar/p/libfiu) is a C library for fault injection.

It provides functions to mark "points of failure" inside your code (the *core
API*), and functions to enable/disable the failure of those points (the
*control API*). It's in the public domain, see the LICENSE file for more
information.

The *core API* is used inside the code wanting to perform fault injection on.
The *control API* is used inside the testing code, in order to control the
injection of failures.

Python bindings are available in the `bindings/` directory.

[![Gitlab CI status](https://gitlab.com/albertito/libfiu/badges/master/pipeline.svg)](https://gitlab.com/albertito/libfiu/pipelines)


## Documentation

You can find the user guide in the `doc/` directory, and a manpage in the
`libfiu/` directory. The manpage will be installed along the library.

Python bindings have embedded documentation, although it's not as complete.


## Building and installing

Running `make` (or `gmake`) should be enough for building, and `make install`
for installing. By default it installs into `/usr/local/`, but you can provide
an alternative prefix by running `make PREFIX=/my/prefix install`.

To build the Python 3 bindings, use `make python3`, to install them you can run
`make python3_install`.


## Where to report bugs

If you want to report bugs, or have any questions or comments, just let me
know at `albertito@blitiri.com.ar`. For more information about the library, you
can go to the [libfiu home page](https://blitiri.com.ar/p/libfiu).


