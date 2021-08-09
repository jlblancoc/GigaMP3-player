
COPT= -wnod -wpro -wuse -C
LOPT= /m

example2.exe: example2.obj \
              ataioint.obj ataioisa.obj ataiopci.obj ataiopio.obj \
              ataioreg.obj ataiosub.obj ataiotmr.obj ataiotrc.obj
   tlink $(LOPT) @&&!
c:\bc31\lib\c0s +
example2 +
ataioint +
ataioisa +
ataiopci +
ataiopio +
ataioreg +
ataiosub +
ataiotmr +
ataiotrc
example2
nul
c:\bc31\lib\emu +
c:\bc31\lib\maths +
c:\bc31\lib\cs
!

example2.obj: example2.c ataio.h
   bcc -c $(COPT) example2.c

ataioint.obj: ataioint.c  ataio.h ataiopd.h
   bcc -c $(COPT) ataioint.c

ataioisa.obj: ataioisa.c  ataio.h ataiopd.h
   bcc -c $(COPT) ataioisa.c

ataiopci.obj: ataiopci.c  ataio.h ataiopd.h
   bcc -c $(COPT) ataiopci.c

ataiopio.obj: ataiopio.c  ataio.h ataiopd.h
   bcc -c -S $(COPT) ataiopio.c
   tasm ataiopio.asm

ataioreg.obj: ataioreg.c  ataio.h ataiopd.h
   bcc -c $(COPT) ataioreg.c

ataiosub.obj: ataiosub.c  ataio.h ataiopd.h
   bcc -c $(COPT) ataiosub.c

ataiotmr.obj: ataiotmr.c  ataio.h ataiopd.h
   bcc -c $(COPT) ataiotmr.c

ataiotrc.obj: ataiotrc.c  ataio.h ataiopd.h
   bcc -c $(COPT) ataiotrc.c

