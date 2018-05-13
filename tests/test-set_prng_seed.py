"""
Test that we get reproducible results with manually set PRNG seeds.
"""

import fiu


fiu.set_prng_seed(1234)
fiu.enable_random('p1', probability = 0.5)
result = { True: 0, False: 0 }
for i in range(1000):
    result[fiu.fail('p1')] += 1

assert result == {False: 516, True: 484}, result


fiu.set_prng_seed(4321)
fiu.enable_random('p1', probability = 0.5)
result = { True: 0, False: 0 }
for i in range(1000):
    result[fiu.fail('p1')] += 1

assert result == {False: 495, True: 505}, result
