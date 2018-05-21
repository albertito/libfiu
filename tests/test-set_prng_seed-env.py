"""
Test that we get reproducible results when setting PRNG seed via the
environment.
"""

# Set the environment variable _before_ initializing the library.
import os
os.environ["FIU_PRNG_SEED"] = "1234"

import fiu

fiu.enable_random('p1', probability = 0.5)
result = { True: 0, False: 0 }
for i in range(1000):
    result[fiu.fail('p1')] += 1

assert result == {False: 516, True: 484}, result
