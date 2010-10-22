#!/usr/bin/env python

import sys
from om.spocp.spocpy import SyncSpocp

def main(num):
	s = SyncSpocp("localhost",5678)                                     
	s.connect()
	for i in range(1,int(num)):
		print s.query("","(8:ldaptest(4:home12:foo.it.su.se)(4:away5:world))")
	s.logout()
	s.close()

if __name__ == "__main__":
	main(sys.argv[1])
