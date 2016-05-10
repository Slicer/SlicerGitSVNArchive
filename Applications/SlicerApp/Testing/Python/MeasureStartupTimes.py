#!/usr/bin/env python

import os
import subprocess
import sys
import time

"""
Usage:
    MeasureStartupTimes.py /path/to/Slicer
"""

EXIT_FAILURE=1
EXIT_SUCCESS=0

def run(slicer, command):
    start = time.time()
    args = ['--no-splash', '--exit-after-startup']
    args.extend(command)
    print("%s %s" % (os.path.basename(slicer), " ".join(args)))
    args.insert(0, slicer)
    p = subprocess.Popen(args=args, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()

    if p.returncode != EXIT_SUCCESS:
        print('STDOUT: ' + stdout)
        print('STDERR: ' + stderr)

    print("{:.3f} seconds".format(time.time() - start))
    print("")

if __name__ == '__main__':

    if len(sys.argv) != 2:
        print(os.path.basename(sys.argv[0]) +" /path/to/Slicer")
        exit(EXIT_FAILURE)

    slicer = os.path.expanduser(sys.argv[1])

    tests = [
        [],
        ['--disable-builtin-cli-modules'],
        ['--disable-builtin-loadable-modules'],
        ['--disable-builtin-scripted-loadable-modules'],
        ['--disable-builtin-cli-modules', '--disable-builtin-scripted-loadable-modules'],
        ['--disable-modules']
    ]

    for test in tests:
        run(slicer, test)

    for test in tests:
        test.insert(0, '--disable-python')
        run(slicer, test)

