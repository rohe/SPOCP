#!/usr/bin/env python
# encoding: utf-8
"""
memory.py

Created by Roland Hedberg on 2007-03-11.
Copyright (c) 2007 __MyCompanyName__. All rights reserved.
"""

import sys
import getopt
import re

help_message = '''
The help message goes here.
'''

logline = re.compile(r'\d\d\d\d-\d\d-\d\d \d\d:\d\d:\d\d \[\d+\]: (.*)')
memline = [
    re.compile(r'0x([0-9a-f]+)\((\d+),(\d+)\)-([a-z]+)'),
    re.compile(r'0x([0-9a-f]+)\((\d+)\)-([a-z]+)'),
    re.compile(r'0x([0-9a-f]+)-([a-z]+)'),
    ]


class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg


def main(argv=None):
    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.getopt(argv[1:], "ho:v", ["help", "output="])
        except getopt.error, msg:
            raise Usage(msg)
    
        # option processing
        for option, value in opts:
            if option == "-v":
                verbose = True
            if option in ("-h", "--help"):
                raise Usage(help_message)
            if option in ("-o", "--output"):
                output = value
    
    except Usage, err:
        print >> sys.stderr, sys.argv[0].split("/")[-1] + ": " + str(err.msg)
        print >> sys.stderr, "\t for help use --help"
        return 2

    f = open(sys.argv[1],"r")
    memloc = {}
    started = 0
    while True:
        l = f.readline()
        l = l.strip()
        if not l:
            break
        for m in logline.findall(l):
            if m == "Running server!!":
                started = 1
            if not started:
                continue
            for ma in memline:
                s = ma.match(m)
                if s:
                    tup = s.groups()
                    if tup[-1] == "free":
                        try:
                            del memloc[tup[0]]
                        except KeyError:
                            print "deleted something I haven't seen allocated %s" % (tup[0])
                    else:
                        memloc[tup[0]] = tup[1:]
                
    print len(memloc)
    print memloc

if __name__ == "__main__":
    sys.exit(main())
