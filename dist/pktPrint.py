#!/usr/bin/env python
import os, sys, time, pickle
import VectorCan, CanJilParser


pktInfo = {}

def int2bin(n, count=24): 
    return "".join([str((n >> y) & 1) for y in range(count-1, -1, -1)]) 

def ParseSimpleBits(bits=''):
    byte, bit = bits.split('.')
    byte = int(byte) - 1
    bit = int(bit)
    
    b = (int(byte)*8) + int(bit)
    return [b,b+1]

def ParseComplexBits(bits=''):
    bt1, bt2 = bits.split('-')
    bt1 = bt1.strip().split('.')
    bt2 = bt2.strip().split('.')
    
    bt1 = (int(bt1[0])-1)*8 + int(bt1[1])
    bt2 = (int(bt2[0])-1)*8 + int(bt2[1])
    if (bt1 > bt2):
        tmp = bt2
        bt2 = bt1
        bt1 = tmp
    
    return [bt1 , bt2]

def ParseBitFields(bits=''):
    if bits.find('-') != -1:
        return ParseComplexBits(bits)
    return ParseSimpleBits(bits)
    
def PrintFields(id=0, data=[]):
    dataStr = ''
    for i in data:
        dataStr = dataStr + int2bin(i, 8)

    rep = ''
    try:
        fields = pktInfo[id][1]
        
        for field in fields:
            slc = ParseBitFields(field[0])
            bStr = dataStr[slc[0]:slc[1]]
            rep = rep + field[1] + ": 0x%.2X" % int(bStr, 2) + '\n'
    except :
        rep = str(data)
    
    return rep

def Sniff(chan=[]):
	c = VectorCan.Can()
	c.openChannels(chan)
	print c
	

	while True:
		m = c.read()
		if len(m) != 0:
			for i in m:
				print i, PrintFields(i.id, i.msg)
		time.sleep(0.001)



f = open('pktStruct', 'rb')
pktInfo = pickle.load(f)
f.close()

