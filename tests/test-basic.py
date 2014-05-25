"""
Basic tests for general functionality.
"""

import fiu

# Test unknown failure point.
assert not fiu.fail('unknown')

# Test enable/disable.
fiu.enable('p1')
assert fiu.fail('p1')
fiu.disable('p1')
assert not fiu.fail('p1')

# Test enable_random.
fiu.enable_random('p1', probability = 0.5)
result = { True: 0, False: 0 }
for i in range(1000):
    result[fiu.fail('p1')] += 1

assert 400 < result[True] < 600, result
assert 400 < result[False] < 600, result

# Test repeated enabling/disabling.
fiu.enable('p1')
fiu.enable('p1')
assert fiu.fail('p1')
fiu.disable('p1')
assert not fiu.fail('p1')
