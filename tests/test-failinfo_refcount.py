
"""
Test that we keep references to failinfo as needed.
"""

import fiu

# Object we'll use for failinfo
finfo = [1, 2, 3]

fiu.enable('p1', failinfo = finfo)

assert fiu.fail('p1')
assert fiu.failinfo() is finfo

finfo_id = id(finfo)
del finfo

assert fiu.failinfo() == [1, 2, 3]
assert id(fiu.failinfo()) == finfo_id
