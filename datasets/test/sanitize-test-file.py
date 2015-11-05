# Copyright (c) 2011, Chris Pearce & Yun Sing Koh
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#  * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#  * Neither the name of the authors nor the names of contributors may be
# used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


import sys
import csv
import os
import math

def writeOutput(line, sOutput, mode):
    f = open(sOutput, mode)
    f.write(line)
    f.close()

def splitFile(filename, split):
    reader = csv.reader(file(filename),dialect='excel', skipinitialspace = True, delimiter='\t')
    split = int(split) * 1000
    i = 0
    for row in reader:
        if i < split:
            print row[0]
            i = i + 1
        

def parseFile(filename):
    try:
        reader = csv.reader(file(filename),dialect='excel', skipinitialspace = True, delimiter='\t')
        strTemp = ""
        previous = ""
        for row in reader:
            if row[0] == previous:
                strTemp += row[1] + ","
            else:
                print strTemp
                strTemp = row[1] + ","
            previous = row[0]
                                    
                
                                                            
    except:
        print row
        print "Unexpected error:", sys.exc_info()[0]
        raise

if __name__ == '__main__':
    
    while True:
        try:
            if len(sys.argv) < 1:
                print "convertFile: There must be at least one input file."
                sys.exit()
            ###parseFile(sys.argv[1])
            splitFile(sys.argv[1], sys.argv[2])
            break
        except:
            print "Unexpected error:", sys.exc_info()[0]
            raise