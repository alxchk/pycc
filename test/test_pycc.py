#!/usr/bin/python

import pycc

tags = (
    (2, 7),
    (3, 5),
    (3, 6),
    (3, 7),
    (3, 8),
    (3, 9)
)

for _ in range(256):
    for tag in tags:
        ctx = pycc.Ctx(*tag)
        ctx.compile('1+1')
