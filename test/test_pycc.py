#!/usr/bin/python

import threading
import pycc

tags = (
    (2, 7),
    (3, 5),
    (3, 6),
    (3, 7),
    (3, 8),
    (3, 9)
)


def test():
    for _ in range(256):
        for tag in tags:
            ctx = pycc.Ctx(*tag)
            ctx.compile('1+1')


def test_mt(tag):
    for _ in range(256):
        ctx = pycc.Ctx(*tag)
        ctx.compile('1+1')


print('[+] Single thread')
test()

print('[+] Multi-thread')

threads = []
for tag in tags:
    thread = threading.Thread(target=test_mt, args=(tag,))
    thread.start()
    threads.append(thread)

for thread in threads:
    thread.join()

print('[+] Done')
