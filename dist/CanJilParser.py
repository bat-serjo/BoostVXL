#!/usr/bin/env python
import os, re, sys, time
from pyPEG import *

def comment():          return [re.compile(r"//.*"),re.compile("/\*.*?\*/",re.S), re.compile (r"#.*") ]
def literal():          return re.compile(r'[\d\w]+')
def symbol():           return re.compile(r"\w+")
def component():        return re.compile(r"\w+")
def componentname():    return re.compile(r"\w+")
def operator():         return re.compile(r"\=")
def variable():         return symbol, '=', literal, 0, ('{', -2, variable, '}'), ';'
def block():            return (0, component), componentname, "{", -2, [ block, variable], "};"
def lang():             return -2, block

def _variable(ns, tokenTuple):
    if len(tokenTuple) > 1:
        k = tokenTuple[1][0][1]
        v = tokenTuple[1][1][1]
        
        if ns.__dict__.has_key(k):
            tp = type( ns.__dict__[k] )
            
            if tp == list:
                ns.__dict__[ k ].append(v)
            else:
                ns.__dict__[ k ] = [ ns.__dict__[k], v ]
        else:
            ns.__dict__[k] = v

def _block(ns, tokenTuple):
    if len(tokenTuple) > 1:
        o = dClass()
        o.tokenType = tokenTuple[0]
        o.contents = tokenTuple[1]
        
        for elem in tokenTuple[1]:
            if len(elem) > 1 and type(elem[1]) != list:
                o.__dict__[elem[0]] = elem[1]
            else:
                asObject(elem, o)

        if ns != None:
            ns.__dict__[o.componentname] = o
        else:
            return o

def asObject(elem=(), ns=None):
    tokenName = '_'+elem[0]
    #exec 'ns = %s(ns, elem)' % tokenName
    ns = globals()[tokenName](ns, elem)
    return ns

class dClass():
    tokenType = None
    contents = []

    def __str__(self, quiet=True):
        r = '\n-------------------------------------\n'
        keys = self.__dict__.keys()
        keys.sort()
        for i in keys:
            if i == 'contents' and quiet == True: continue
            
            r += '#\t%s: ' % i
            lns = str(self.__dict__[i]).splitlines()
            for l in lns:
                r += '\t%s\n' % l
                
        return r

    def __iter__(self):
        for attr in self.__dict__:
            aelem = self.__dict__[attr]
            if isinstance(aelem, dClass) and aelem.tokenType == 'block':
                yield aelem
                
                for i in aelem:
                    yield i


def FileAsObject(fname=''):
    f = open(fname, 'r')
    data = f.read()
    f.close()

    tokens = []
    result = parseLine(data, lang(), tokens, True, comment)

    retObj = dClass()
    for t in tokens:
        asObject(t, retObj)
    return retObj
