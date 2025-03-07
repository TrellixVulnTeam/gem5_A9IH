# Copyright (c) 2007 The Hewlett-Packard Development Company
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

microcode = '''
def macroop CVTSS2SD_XMM_XMM {
    cvtf2f xmml, xmmlm, destSize=8, srcSize=4, ext=Scalar
};

def macroop CVTSS2SD_XMM_M {
    ldfp ufp1, seg, sib, disp, dataSize=8
    cvtf2f xmml, ufp1, destSize=8, srcSize=4, ext=Scalar
};

def macroop CVTSS2SD_XMM_P {
    rdip t7
    ldfp ufp1, seg, riprel, disp, dataSize=8
    cvtf2f xmml, ufp1, destSize=8, srcSize=4, ext=Scalar
};

def macroop CVTSD2SS_XMM_XMM {
    cvtf2f xmml, xmmlm, destSize=4, srcSize=8, ext=Scalar
};

def macroop CVTSD2SS_XMM_M {
    ldfp ufp1, seg, sib, disp, dataSize=8
    cvtf2f xmml, ufp1, destSize=4, srcSize=8, ext=Scalar
};

def macroop CVTSD2SS_XMM_P {
    rdip t7
    ldfp ufp1, seg, riprel, disp, dataSize=8
    cvtf2f xmml, ufp1, destSize=4, srcSize=8, ext=Scalar
};

def macroop CVTPS2PD_XMM_XMM {
    cvtf2f xmmh, xmmlm, destSize=8, srcSize=4, ext=2
    cvtf2f xmml, xmmlm, destSize=8, srcSize=4, ext=0
};

def macroop CVTPS2PD_XMM_M {
    ldfp ufp1, seg, sib, disp, dataSize=8
    cvtf2f xmmh, ufp1, destSize=8, srcSize=4, ext=2
    cvtf2f xmml, ufp1, destSize=8, srcSize=4, ext=0
};

def macroop CVTPS2PD_XMM_P {
    rdip t7
    ldfp ufp1, seg, riprel, disp, dataSize=8
    cvtf2f xmmh, ufp1, destSize=8, srcSize=4, ext=2
    cvtf2f xmml, ufp1, destSize=8, srcSize=4, ext=0
};

def macroop CVTPD2PS_XMM_XMM {
    cvtf2f xmml, xmmlm, destSize=4, srcSize=8, ext=0
    cvtf2f xmml, xmmhm, destSize=4, srcSize=8, ext=2
    lfpimm xmmh, 0
};

def macroop CVTPD2PS_XMM_M {
    ldfp ufp1, seg, sib, "DISPLACEMENT", dataSize=8
    ldfp ufp2, seg, sib, "DISPLACEMENT + 8", dataSize=8
    cvtf2f xmml, ufp1, destSize=4, srcSize=8, ext=0
    cvtf2f xmml, ufp2, destSize=4, srcSize=8, ext=2
    lfpimm xmmh, 0
};

def macroop CVTPD2PS_XMM_P {
    rdip t7
    ldfp ufp1, seg, riprel, "DISPLACEMENT", dataSize=8
    ldfp ufp2, seg, riprel, "DISPLACEMENT + 8", dataSize=8
    cvtf2f xmml, ufp1, destSize=4, srcSize=8, ext=0
    cvtf2f xmml, ufp2, destSize=4, srcSize=8, ext=2
    lfpimm xmmh, 0
};

# F16C extension: packed single precision -> packed half precision.

def macroop VCVTPS2PH_XMM_XMM_I {
    cvtf2f xmml, xmmlm, destSize=2, srcSize=4, ext=0
    cvtf2f xmml, xmmhm, destSize=2, srcSize=4, ext=2
    lfpimm xmmh, 0
};

def macroop VCVTPS2PH_M_XMM_I {
    cvtf2f t7, xmmlm, destSize=2, srcSize=4, ext=0
    cvtf2f t7, xmmhm, destSize=2, srcSize=4, ext=2
    stfp t7, seg, sib, disp, dataSize=4
};

def macroop VCVTPS2PH_P_XMM_I {
    rdip t7
    cvtf2f t0, xmmlm, destSize=2, srcSize=4, ext=0
    cvtf2f t0, xmmhm, destSize=2, srcSize=4, ext=2
    stfp t0, seg, riprel, disp, dataSize=4
};

# F16C extension: packed half precision -> packed single precision.

def macroop VCVTPH2PS_XMM_XMM {
    cvtf2f xmmh, xmmlm, destSize=4, srcSize=2, ext=2
    cvtf2f xmml, xmmlm, destSize=4, srcSize=2, ext=0
};

def macroop VCVTPH2PS_XMM_M {
    ldfp ufp1, seg, sib, "DISPLACEMENT", dataSize=8
    cvtf2f xmmh, ufp1, destSize=4, srcSize=2, ext=2
    cvtf2f xmml, ufp1, destSize=4, srcSize=2, ext=0
};

def macroop VCVTPH2PS_XMM_P {
    rdip t7
    ldfp ufp1, seg, riprel, "DISPLACEMENT", dataSize=8
    cvtf2f xmmh, ufp1, destSize=4, srcSize=2, ext=2
    cvtf2f xmml, ufp1, destSize=4, srcSize=2, ext=0
};
'''
