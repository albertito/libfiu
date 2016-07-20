
Simulating failures in the POSIX API
====================================

When developing robust software, developers often consider the cases when the
classic POSIX functions return failure.

Testing that fault-handling code is a problem because under normal conditions
it's hard to make the POSIX functions fail, and generating abnormal conditions
is usually difficult.

For example, getting *malloc()* to fail can represent using up all your
memory, and at that point your test case might not even work. Or getting I/O
operations to fail might involve filling up the disk which is very
undesirable, or generating a very special environment which is difficult to
reproduce.

libfiu comes with some tools that can be used to perform fault injection in
the POSIX API (which includes the C standard library functions) *without*
having to modify the application's source code, that can help to simulate
scenarios like the ones described above in an easy and reproducible way.


fiu-run
-------

The first of those tools is an application called *fiu-run*.

Suppose you want to run the classic program "fortune" (which some would
definitely consider mission critical) and see how it behaves on the presence
of *read()* errors. With *fiu-run*, you can do it like this::

  $ fiu-run -x -c "enable_random name=posix/io/rw/read,probability=0.05" fortune

That enables the failure point with the name *posix/io/rw/read* with 5%
probability to fail *on each call*, and then runs fortune. The *-x*
parameter tells *fiu-run* to enable fault injection in the POSIX API.

Run it several times and you can see that sometimes it works, but sometimes it
doesn't, reporting an error reading, which means a *read()* failed as
expected.

When fortune is run, every *read()* has a 5% chance to fail, selecting an
*errno* at random from the list of the ones that read() is allowed to return.
If you want to select a specific *errno*, you can do it by passing its
numerical value using the *-i* parameter.

The name of the failure points are fixed, and there is at least one for each
function that libfiu supports injecting failures to. Not all POSIX functions
are included, but most of the important pieces are, and it can be easily
extended. See below for details.

To see the list of supported functions and names, see the (automatically
generated) *preload/posix/function_list* file that comes in the libfiu
tarball.


fiu-ctrl
--------

Sometimes it is more interesting to simulate failures at a given point in time
instead of from the beginning, as *fiu-run* does.

To that end, you can combine *fiu-run* with the second tool, called
*fiu-ctrl*.

Let's suppose we want to see what the "top" program does when it can't open
files. First, we run it with *fiu-run*::

  $ fiu-run -x top

Everything should look normal. Then, in another terminal, we make *open()*
fail unconditionally::

  $ fiu-ctrl -c "enable name=posix/io/oc/open" `pidof top`

After that moment, the top display will probably be empty, because it can't
read process information. Now let's disable that failure point, so *open()*
works again::

  $ fiu-ctrl -c "disable name=posix/io/oc/open" `pidof top`

And everything should have gone back to normal.


How does it work
----------------

libfiu comes with two preload libraries: *fiu_run_preload* and
*fiu_posix_preload*.

The first one is loaded using *LD_PRELOAD* (see *ld.so(8)* for more
information) by *fiu-run*, and can enable failure points and start libfiu's
remote control capabilities before the program begins to run.

The second one is also loaded using *LD_PRELOAD* by *fiu-run* when the
*-x* parameter is given, and provides libfiu-enabled wrappers for the POSIX
functions, allowing the user to inject failures in them.

*fiu-ctrl* communicates with the applications launched by
*fiu-run* via the libfiu remote control capabilities.


