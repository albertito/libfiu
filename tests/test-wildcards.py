"""
Test the behaviour of the wildcard failure points.
"""

import fiu

fiu.enable("a:b:c")
assert fiu.fail("a:b:c")

fiu.enable("a:b:*")
assert fiu.fail("a:b:c")
assert fiu.fail("a:b:x")
assert fiu.fail("a:b:c:d")

fiu.enable("a:b:*")  # Test repeated enabling of a wildcard.

fiu.enable("a:b:c:d")
assert fiu.fail("a:b:c:d")

fiu.disable("a:b:c")
assert fiu.fail("a:b:c")

fiu.disable("a:b:*")
assert not fiu.fail("a:b:c")
assert not fiu.fail("a:b:x")
assert fiu.fail("a:b:c:d")

fiu.disable("a:b:c:d")
assert not fiu.fail("a:b:c:d")


s = "x"
for i in range(200):
    fiu.enable(s + "/*")
    s += "/x"

s = "x"
for i in range(200):
    assert fiu.fail(s + '/asdf')
    fiu.disable(s + "/*")
    s += "/x"

fiu.enable("*")
assert fiu.fail("asdf")
fiu.disable("*")
assert not fiu.fail("asdf")
