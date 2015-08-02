"""
Tests to make sure cache invalidation works.
"""

import fiu

# Unknown - add - fail - remove - not fail.
# The initial unknown is relevant because it places a negative match in the
# cache.
assert not fiu.fail('p1')
fiu.enable('p1')
assert fiu.fail('p1')
fiu.disable('p1')
assert not fiu.fail('p1')

# Same as above, but with wildcards.
assert not fiu.fail('p2/x')
fiu.enable('p2/*')
assert fiu.fail('p2/x')
fiu.disable('p2/*')
assert not fiu.fail('p2/x')

