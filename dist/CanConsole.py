#!/usr/bin/env python
import os, sys, md5, time, pickle
import VectorCan, CanJilParser

def int2bin(n, count=24): 
    return "".join([str((n >> y) & 1) for y in range(count-1, -1, -1)]) 

class CanFrame():
    def __init__(self, **kargs):
        self._repMsg = VectorCan.CanMsg
        # fields description 'name': (sB, eB)
        self._fields = {}
    
    def __get__(self, o, otype):
        dataStr = ''
        dataStr = dataStr + int2bin(i, 8)

    def __str__(self):
        dataStr = ''
        rep = ''
        
        try:
            for i in self._repMsg.msg:
                dataStr = dataStr + int2bin(i, 8)

            for field in self._fields:
                bitRange = self._fields[ field ]
                bStr = dataStr[ bitRange[0]:bitRange[1] ]
                rep = rep + field + ": 0x%.2X" % int(bStr, 2) + '\n'
        except :
            rep = str(self._repMsg)

        return rep

class SPOFile():
        md5HashData = ''
        pktStructData = {}
        
class PktViewer():
    def __init__(self, fname=''):
        #format pktStruct [id] = [0 FrameInfo, 1... fields info]
        self.pktStruct = {}
        
        f = open(fname, 'rb')
        data = f.read()
        f.close()
        
        md5Hash = md5.new()
        md5Hash.update(data)
        
        localSPO = os.path.split(fname)[1]+'.SPO'
        if os.path.exists('.'+os.path.sep+localSPO):
            print 'Found prepared SPO for the file %s' % os.path.split(fname)[1]
            
            fSPO = open(localSPO, 'rb')
            tmpSPOFile = pickle.load(fSPO)
            fSPO.close()
            
            if md5Hash.hexdigest() == tmpSPOFile.md5HashData:
                print 'MD5 hashes match! Using preparsed file.'
                self.pktStruct = tmpSPOFile.pktStructData
                return
            
        else:
            print 'Parsing %s ...' % fname
            obj = CanJilParser.FileAsObject(fname)

            allMsg = {}
            allData = {}

            for subElem in obj:
                try:
                    if subElem.component == 'CANUUDTMessage':
                        print subElem.componentname, int(subElem.Identifier, 16)
                        self.pktStruct[ int(subElem.Identifier, 16) ] = [subElem]
                except:
                    pass

                try:
                    if subElem.component == 'Message':
                        allMsg[ subElem.componentname ] = subElem
                    if subElem.component == 'Data':
                        allData[ subElem.componentname ] = subElem
                except:
                    pass

            for canId in self.pktStruct:
                msgDesc = allMsg[ self.pktStruct[canId][0].Message ] 
                
                if type(msgDesc.Data) == list:
                    for sigName in msgDesc.Data:
                        sig = allData[ sigName ]
                        sig = self._prepSignalForStruct(sig)
                        self.pktStruct[canId].append(sig)
                else:
                    sig = allData[ msgDesc.Data ]
                    sig = self._prepSignalForStruct(sig)
                    self.pktStruct[canId].append(sig)
            
            newSPOFile = SPOFile()
            newSPOFile.md5HashData = md5Hash.hexdigest()
            newSPOFile.pktStructData = self.pktStruct
            
            fSPO = open(localSPO, 'wb+')
            pickle.dump(newSPOFile, fSPO)
            fSPO.close()

    def _prepSignalForStruct(self, sig):
        sig.BytePosition = int(sig.BytePosition)
        sig.BitPosition = int(sig.BitPosition)
        sig.BitSize = int(sig.BitSize)
        
        sig.BitsOnRight = (8 - (((7 - sig.BitPosition) + sig.BitSize) % 8)) % 8
        sig.BytesOccupied = (sig.BitSize + sig.BitsOnRight + (7 - sig.BitPosition))/8
        sig.Mask = ((1 << sig.BitSize) - 1) << sig.BitsOnRight
        
        return sig

    def asStr(self, id, data):
        rep = ''
        try:
            fields = self.pktStruct[id][1:]
            
            for field in fields:
                sB = field.BytePosition
                eB = sB + field.BytesOccupied
                
                tmpInt = 0
                for canByte in data[sB : eB]:
                    tmpInt = tmpInt << 8
                    tmpInt = tmpInt|canByte
                tmpInt = (tmpInt & field.Mask) >> field.BitsOnRight
                
                rep = rep + '%25s: %X\n' % (field.componentname, tmpInt)
        except :
            rep = str(data)
        return rep

def CanoeParse(fname = ''):
    f = open(fname, 'r')
    data = f.read()
    f.close()
    
    pktData = []
    data = data.splitlines()
    
    for line in data:
        if line.find(' Rx   ') != -1:
            line = line.split()
            
            time  = int(line[0].replace('.',''))
            id    = int(line[2], 16)
            flags  = 0 #int(line[4], 16)
            dlc   = int(line[5], 16)
            payload = [int(x, 16) for x in line[6:]]
            
            m = VectorCan.CanMsg(id, payload, flags)
            m.time = time
            
            pktData.append(m)

    return pktData

def Play(chan=[], fname = ''):
    canTrace = CanoeParse(fname)
    
    c = VectorCan.Can()
    c.openChannels(chan, 125000, VectorCan.CanOutputFlags.OUTPUT_MODE_NORMAL)
    print c

    print len(canTrace)
    lastTime = 0
    for msg in canTrace:
        sleepTime = (msg.time - lastTime)/1000.0
        lastTime = msg.time
        time.sleep(1)
        #print msg
        c.write([msg])
        m = c.read()
        #print p.asStr(msg.id, msg.msg)
      

def Sniff(chan=[]):
    c = VectorCan.Can()
    c.openChannels(chan, 125000, VectorCan.CanOutputFlags.OUTPUT_MODE_TX_OFF)
    print c

    m=None
    while True:
        m = c.read()
        if len(m) != 0:
            for i in m:
                print p.asStr(i.id, i.msg)
        time.sleep(0.01)

#p = PktViewer(r'D:\EMF2010\DEVELOPMENT\EMF\V850\C_MINUS\Build\SOURCES\EMF2010_C_MINUS.JIL')
#Play([0], sys.argv[1])
#Sniff([0])
