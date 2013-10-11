
"""
Test the creation of many failure points.
"""

import fiu

N = 1000

for i in range(N):
    fiu.enable(str(i))

for i in range(N):
    assert fiu.fail(str(i))

# Remove only half and check again; this will stress the shrinking of our data
# structures.
for i in range(N / 2):
    fiu.disable(str(i))

for i in range(N / 2, N):
    assert fiu.fail(str(i))

for i in range(N / 2, N):
    fiu.disable(str(i))
