#/* Copyright 2001 Kjetil S. Matheussen
#
#This program is free software; you can redistribute it and/or
#modify it under the terms of the GNU General Public License
#as published by the Free Software Foundation; either version 2
#of the License, or (at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */

import sys,os,traceback

try:
    print 0
    sys.path.append(os.path.join(os.path.abspath(os.path.dirname(sys.argv[0])),
                                 "packages/dvarrazzo-py-setproctitle-c35a1bf/build/lib."+str(sys.version_info[0])))
    print 1
    print sys.path
    print 2
    import setproctitle
    setproctitle.setproctitle('radium')
except:
    print "Unable to set proctitle to radium"
    traceback.print_exc()


import radium,keybindingsparser

from common import *

ra=radium

class KeyHandler:

    def __init__(self):
        self.keyslist=[]
        self.handlers=[]

    def addHandle(self,keys,handle):
        for lokke in range(len(self.keyslist)):
            if self.keyslist[lokke]==keys:
                return false

        self.keyslist.append(keys)
        self.handlers.append(handle)
        return true
            
# keys is a constant!
    def exe(self,windownum,keys):
        for lokke in range(len(self.keyslist)):
            if self.keyslist[lokke]==keys:
                try:
                    eval(self.handlers[lokke])
                except:
                    traceback.print_exc(file=sys.stdout)
                break
        return


def getKeyHandler(num):
    return KeyHandler()

keyhandles=map(getKeyHandler,range(len(keybindingsparser.keysub)))

# key and keys are constants!
def gotKey(windownum,key,keys):
    print "*********** key: " + keybindingsparser.keysub[key] + ". keys: " + str(map(lambda k:keybindingsparser.keysub[k], keys))
#    key=keys.pop(0)
    keyhandles[key].exe(windownum,keys);    


try:
    infilehandle=open(sys.argv[1],'r')
except:
    print "Cant open %s" % sys.argv[1]
    sys.exit(1)

try:
    outfilehandle=open("eventreceiverparser_generated.py",'w+')
except:
    print "Cant open file for writing"
    sys.exit(2)

outfilehandle.write("#Do not edit. This file is automaticly generated from keybindings.config.\n")


print "Parsing keybindings.conf..."
#profile.run("KeyConfer(infilehandle,outfilehandle)","fooprof")
if keybindingsparser.start(keyhandles,infilehandle,outfilehandle,)==false:
    sys.exit(5)

try:
    outfilehandle.close()
except:
    print "Could not close file. Out of disk space?"
    sys.exit(3)

import eventreceiverparser_generated

#import os
#os.system("/usr/bin/givertcap")


import os
pid = os.getpid()

if os.fork()==0:
    import signal
    def signal_handler(signalnum, frame):
        print "You pressed Ctrl+C. As a work-around, I'm going to kill radium with signal.SIGABRT."
        os.kill(pid,signal.SIGABRT)
        sys.exit(0)
    signal.signal(signal.SIGINT, signal_handler)
    signal.pause()


if len(sys.argv)>2:
    ra.init_radium(sys.argv[2],gotKey)
else:
    ra.init_radium("",gotKey)
    
