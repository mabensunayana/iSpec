"""
    This file is part of Spectra.
    Copyright 2011-2012 Sergi Blanco Cuaresma - http://www.marblestation.com
    
    Spectra is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Spectra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with Spectra.  If not, see <http://www.gnu.org/licenses/>.
"""
import numpy as np
import os
cimport numpy as np


cdef extern from "stdio.h":
    ctypedef struct FILE

cdef extern from "spectrum276e/spectrum.h":
    struct memo:
        int lyman
        int balmer
        int paschen
        int brackett
        int pfund
        int humphreys
        int hprofl
        int helium
        int strong
        int interval
    ctypedef struct linedata:
        double wave
        double code
        int iso
        double atomass
        double abund
        double chi1
        double chi2
        double chi3
        double chi4
        double chi
        double Eu
        double El
        double gf
        double wavel
        double waveh
        float  xnum[100]
        float  a[100]
        float  dopp[100]
        float  capnu[100]
        float  dlg[100]
        char   T[5]
        double alp
        double sig
        double gammar
        double gammas
        double gammaw
        double fac
        int ai
        int flag

cdef extern from "synthesizer_func.h":
    int synthesize_spectrum(char *atmosphere_model_file, char *linelist_file, char *abundances_file, double microturbulence_vel, int verbose, int num_measures, double* waveobs, double *fluxes)
    # Global variables
    cdef int Ntau
    cdef float **bkap
    cdef float **bkap2
    cdef float **bkap3
    cdef float **bkap4
    cdef double inc
    cdef int flagr
    cdef int flagc
    cdef int flagk
    cdef int flagg
    cdef int flagmgh
    cdef int flagI
    cdef int flagt
    cdef int flagp
    cdef int flagP
    cdef int flagu
    cdef int flagO
    cdef int flagC
    cdef int mghla
    cdef int mghlb
    cdef float *velgrad
    cdef double mu
    cdef int NI
    # variables for isotopes
    cdef double ra1H,ra2H,ra12C,ra13C,ra14N,ra15N,ra16O,ra17O,ra18O
    cdef double ra24Mg,ra25Mg,ra26Mg,ra28Si,ra29Si,ra30Si,ra40Ca,ra42Ca
    cdef double ra43Ca,ra44Ca,ra46Ca,ra48Ca,ra46Ti,ra47Ti,ra48Ti,ra49Ti
    cdef double ra50Ti
    cdef memo reset
    cdef FILE *opout
    cdef linedata *oneline


# waveobs in armstrong
# microtturbulence velocity in km/s
def spectrum(np.ndarray[np.double_t,ndim=1] waveobs, char* atmosphere_model_file, char* linelist_file = "input/luke.lst", char* abundances_file = "input/stdatom.dat", double microturbulence_vel = 2.0, int verbose = 0):
    if not os.path.exists(atmosphere_model_file):
        raise Exception("Atmosphere model file '%s' does not exists!" % atmosphere_model_file)
    if not os.path.exists(linelist_file):
        raise Exception("Line list file '%s' does not exists!" % linelist_file)
    if not os.path.exists(abundances_file):
        raise Exception("Abundances file '%s' does not exists!" % abundances_file)
    global Ntau
    global flagr
    global flagc
    global flagk
    global flagg
    global flagmph
    global flagI
    global flagt
    global flagp
    global flagP
    global flagu
    global flagO
    global flagC
    global mghla
    global mghlb
    global mu
    global NI
    Ntau = 72  # 72 layers for castelli-kurucz atmosphere models
    flagr = 0
    flagc = 0
    flagk = 0
    flagg = 0
    flagmgh = 0
    flagI = 0   # Isotopes (1: True, 0: False) If 1 it produces segmentation fault (original SPECTRUM problem)
    flagt = 0
    flagp = 0
    flagP = 0
    flagu = 0
    flagO = 0
    flagC = 0
    mghla = 0
    mghlb = 0
    mu = 1.0
    NI = 0

    
    
    cdef int num_measures = len(waveobs)
    cdef np.ndarray[np.double_t,ndim=1] fluxes = np.zeros(num_measures, dtype=float)
    if num_measures <= 1:
        # We need at least 2 wavelengths, if not return an zeroed result
        return flux
    
    # waveobs is multiplied by 10.0 in order to be converted from nm to armstrongs
    synthesize_spectrum(atmosphere_model_file, linelist_file, abundances_file, microturbulence_vel, verbose, num_measures, <double*> waveobs.data, <double*> fluxes.data)
    
    return fluxes
    
    


