#!/usr/bin/env python

import os, sys

def CanoeParse(fname = ''):
    f = open(fname, 'r')
    data = f.read()
    f.close()
    
    pktData = []
    data = data.splitlines()
    
    for line in data:
        if line.find(' Rx   ') != -1:
            line = line.split()
            
            time = int(line[0].replace('.',''))
            id = int(line[2], 16)
            flags = int(line[4], 16)
            dlc = int(line[5], 16)
            payload = [int(x, 16) for x in line[6:]]
            
            pktData.append([time, flags, id, dlc, payload])
    
    return pktData

r = CanoeParse(sys.argv[1])
for i in r:
    print '%d %X ID:%3X DLC:%X payload:%s' % (i[0], i[1], i[2], i[3], str(i[4]))
