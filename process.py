#!/usr/bin/python

import os
from itertools import tee

def grouped(iterable, n):
    "s -> (s0,s1,s2,...sn-1), (sn,sn+1,sn+2,...s2n-1), (s2n,s2n+1,s2n+2,...s3n-1), ..."
    return zip(*[iter(iterable)]*n)

file = open('ratios')

total = 0
compress = 0

for (f, r) in grouped(file, 2):
    f = f.strip()
    r = float(r)

    s = os.stat(f).st_size
    total += s
    compress += s * r / 8
    print(os.path.basename(f) + ';' + str(s) + ';' + str(r));
