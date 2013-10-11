
"""
Test that we fail ONETIME points only one time.
"""

import fiu

fiu.enable('p1', flags = fiu.Flags.ONETIME)
fiu.enable('p2')

assert fiu.fail('p1')
for i in range(100):
    assert not fiu.fail('p1')

for i in range(100):
    assert fiu.fail('p2')

