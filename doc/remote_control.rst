
Remote control
==============

The library has remote controlling capabilities, so external, unrelated
processes can enable and disable failure points.

It has a very simple request/reply protocol that can be performed over
different transports. At the moment, the only transport available is named
pipes.

Remote control must be enabled by the controlled process using *fiu_rc_fifo()*
(for named pipes). A set of utilities are provided to enable remote control
without having to alter the application's source code, which can be useful for
performing fault injection in libraries, see *fiu-run* and *fiu-ctrl* for more
information.


Remote control protocol
-----------------------

It is a line based request/reply protocol. Lines end with a newline character
(no carriage return). A request is composed of a command and 0 or more
parameters, separated with a single space. The following commands are
supported at the moment:

 - ``enable <name> <failnum> <failinfo> [flags]``
 - ``enable_random <name> <failnum> <failinfo> <probability> [flags]``
 - ``disable <name>``

Where:

 - *name* is the name of the point of failure (which, at the moment, cannot
   have spaces inside).
 - *failnum* the same as the *failnum* parameter of *fiu_enable()* (see the
   manpage for more details).
 - failinfo the same as the *failinfo* parameter of *fiu_enable()* (see the
   manpage for more details).
 - *flags* can be either absent or ``one``, which has the same meaning as
   passing ``FIU_ONETIME`` in the *flags* parameter to *fiu_enable()*.

The reply is always a number: 0 on success, < 0 on errors.

