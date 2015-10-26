      FUNCTION QTRAV(TETA,HP,J,JA)
*
*-----------------------------------------------------------------------
*
* HERE THE PARTITION FUNCTIONS ACCORDING TO TRAVING ET AL., ABH. HAMB.
* STERNW. VIII, 1 (1966) ARE COMPUTED. THE SYMBOLS ARE GIVEN
* IN THE COMMENTS AT THE BEGINNING OF SUBROUTINE INJON.
* FUNCTION QAS IS CALLED.
*
* Export version  1988-03-24  ********* Olof Morell *** Uppsala
*
* DIMENSIONS NECESSARY
* A(5),ASDE(KMAX),H(5),QPRIM(KMAX)
* KMAX IS THE TOTAL NUMBER OF ELECTRON CONFIGURATIONS.
* DIMENSIONS OF ARRAYS IN COMMON /CI3/ ARE COMMENTED ON IN SUBROUTINE
* INJON.
*
*-----------------------------------------------------------------------
*
      DIMENSION ASDE(80),H(5),QPRIM(80)
*
      COMMON/CI3/ ALFA(300),GAMMA(300),G0(45),G2(80),XION(80),XL(80),
     &            JBBEG(45),JCBEG(45),NK(45),NL(80),IFISH
      COMMON/CI7/ A(5),PFISH,ITP

*
* STATEMENT FUNCTION FOR 10.**
*
      EXP10(X)=EXP(2.302585*X)
C
      FLJ=J
      JB=JBBEG(JA)
      JC1=JCBEG(JA)
      NKP=NK(JA)
      QSUM=0.
*
* WE START THE LOOP OVER DIFFERENT ELECTRON CONFIGURATIONS, 'THE K-LOOP'
*
      DO 5 K=1,NKP
        JC2=NL(JB)+JC1-1
*
* IS TETA=PRECEDING TETA
*
        IF(ITP.GT.0) GOTO 4
        PRA=XION(JB)*TETA
        IF(PRA.LT.12.) GOTO 1
        ASDE(JB)=0.
        GOTO 2
    1   ASDE(JB)=G2(JB)*EXP10(-PRA)
*
    2   QPRIM(JB)=0.
        IF(NL(JB).LE.0) GOTO 4
        DO 3 L=JC1,JC2
          PRE=GAMMA(L)*TETA
          IF(PRE.GT.12.) GOTO 3
          QPRIM(JB)=QPRIM(JB)+ALFA(L)*EXP10(-PRE)
    3   CONTINUE
    4   JC1=JC2+1
        QSUM=QPRIM(JB)+ASDE(JB)*QAS(HP,XL(JB),A(J),FLJ,PFISH,IFISH)
     &               +QSUM
        JB=JB+1
    5 CONTINUE
*
* END OF 'THE K-LOOP'
*
      QTRAV=G0(JA)+QSUM
*
      RETURN
      END