
libfiu - Fault injection in userspace
=====================================

Introduction
------------

You, as a programmer, know many things can fail, and your software is often
expected to be able to handle those failures. But how do you test your failure
handling code, when it's not easy to make a failure appear in the first place?
One way to do it is to perform *fault injection*.

According to Wikipedia, "fault injection is a technique for improving the
coverage of a test by introducing faults in order to test code paths, in
particular error handling code paths, that might otherwise rarely be followed.
It is often used with stress testing and is widely considered to be an
important part of developing robust software".

libfiu is a library that you can use to add fault injection to your code. It
aims to be easy to use by means of a simple API, with minimal code impact and
little runtime overhead when enabled.

That means that the modifications you have to do to your code (and build
system) in order to support libfiu should be as little intrusive as possible.


Code overview
-------------

Let's take a look to a small (fictitious) code sample to see what's the
general idea behind libfiu.

Assume that you have this code that checks if there's enough free space to
store a given file::

        size_t free_space() {
                [code to find out how much free space there is]
                return space;
        }

        bool file_fits(FILE *fd) {
                if (free_space() < file_size(fd)) {
                        return false;
                }
                return true;
        }

With current disk sizes, it's very unusual to run out of free space, which
makes the scenario where *free_space()* returns 0 hard to test. With libfiu,
you can do the following small addition::

        size_t free_space() {
                fiu_return_on("no_free_space", 0);

                [code to find out how much free space there is]
                return space;
        }

        bool file_fits(FILE *fd) {
                if (free_space() < file_size(fd)) {
                        return false;
                }
                return true;
        }

The *fiu_return_on()* annotation is the only change you need to make to your
code to create a *point of failure*, which is identified by the name
**no_free_space**. When that point of failure is enabled, the function will
return 0.

In your testing code, you can now do this::

        fiu_init();
        fiu_enable("no_free_space", 1, NULL, 0);
        assert(file_fits("tmpfile") == false);

The first line initializes the library, and the second *enables* the point of
failure. When the point of failure is enabled, *free_space()* will return 0,
so you can test how your code behaves under that condition, which was
otherwise hard to trigger.

libfiu's API has two "sides": a core API and a control API.  The core API is
used inside the code to be fault injected. The control API is used inside the
testing code, in order to control the injection of failures.

In the example above, *fiu_return_on()* is a part of the core API, and
*fiu_enable()* is a part of the control API.


Using libfiu in your project
----------------------------

To use libfiu in your project, there are three things to consider: the build
system, the fault injection code, and the testing code.


The build system
~~~~~~~~~~~~~~~~

The first thing to do is to enable your build system to use libfiu. Usually,
you do not want to make libfiu a runtime or build-time dependency, because
it's often only used for testing.

To that end, you should copy *fiu-local.h* into your source tree, and then
create an option to do a *fault injection build* that #defines the constant
*FIU_ENABLE* (usually done by adding ``-DFIU_ENABLE=1`` to your compiler
flags) and links against libfiu (usually done by adding ``-lfiu`` to your
linker flags).

That way, normal builds will not have a single trace of fault injection code,
but it will be easy to create a binary that does, for testing purposes.


The fault injection code
~~~~~~~~~~~~~~~~~~~~~~~~

Adding fault injection to your code means inserting points of failure in it,
using the core API.

First, you should ``#include "fiu-local.h"`` in the files you want to add
points of failure to. That header allows you to avoid libfiu as a build-time
dependency, as mentioned in the last section.

Then, to insert points of failure, sprinkle your code with calls like
``fiu_return_on("name", -1)``, ``fiu_exit_on("name")``, or more complex code
using ``fiu_fail("name")``. See the libfiu's manpage for the details on
the API.

It is recommended that you use meaningful names for your points of failure, to
be able to easily identify their purpose. You can also name them
hierarchically (for example, using names like *"io/write"*, *"io/read"*, and
so on), to be able to enable entire groups of points of failure (like
*"io/\*"*). To this end, any separator will do, the *'/'* is not special at
all.


The testing code
~~~~~~~~~~~~~~~~

Testing can be done in too many ways, so I won't get into specific details
here. As a general approach, usually the idea with fault injection is to write
tests similar in spirit to the one shown above: initialize the library, enable
one or more failures using the control API, and then check if the code behaves
as expected.

Initially, all points of failure are disabled, which means your code should run
as usual, with a very small performance impact.

The points of failure can be enabled using different strategies:

Unconditional (*fiu_enable()*)
  Enables the point of failure in an unconditional way, so it always fails.

Random (*fiu_enable_random()*)
  Enables the point of failure in a non-deterministic way, which will fail with
  the given probability.

External (*fiu_enable_external()*)
  Enables the point of failure using an external function, which will be called
  to determine whether the point of failure should fail or not.

You can also use an asterisk *at the end* of a name to enable all the points
of failure that begin with the given name (excluding the asterisk, of course).

Check libfiu's manpage for more details about the API.

If you prefer to avoid writing the test code in C, you can use the Python
bindings, and/or the *fiu-run* and *fiu-ctrl* utilities.


