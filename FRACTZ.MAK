#=============================================================
#
#	FRACTZ.MAK - Makefile for project E:\FRACTZ\FRACTZ.PRJ
#		Created on 01/19/94 at 13:07
#
#=============================================================

.AUTODEPEND

#=============================================================
#		Translator Definitions
#=============================================================
CC = bcc +FRACTZ.CFG
TASM = tasm.exe
TLIB = tlib.exe
TLINK = tlink
RC = brcc.exe
RB = rc.exe
LIBPATH = E:\BCOS2\LIB
INCLUDEPATH = E:\BCOS2\INCLUDE


#=============================================================
#		Implicit Rules
#=============================================================
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

.asm.obj:
  $(TASM) -Mx $*.asm,$*.obj

.rc.res:
  $(RC) -r $*.rc

#=============================================================
#		List Macros
#=============================================================
LINK_EXCLUDE =  \
 fractz.res

LINK_INCLUDE =  \
 fractz.obj \
 fractz.def

#=============================================================
#		Explicit Rules
#=============================================================
fractz.exe: fractz.cfg $(LINK_INCLUDE) $(LINK_EXCLUDE)
  $(TLINK) /v /x /Toe /aa /L$(LIBPATH) @&&|
E:\BCOS2\LIB\C02.OBJ+
fractz.obj
fractz
		# no map file
E:\BCOS2\LIB\C2MT.LIB+
E:\BCOS2\LIB\OS2.LIB
fractz.def
|
  rc.exe FRACTZ.res fractz.exe

#=============================================================
#		Individual File Dependencies
#=============================================================
FRACTZ.res: fractz.cfg FRACTZ.RC 
	brcc.exe -R -I$(INCLUDEPATH) -FO FRACTZ.res FRACTZ.RC

FRACTZ.obj: fractz.cfg FRACTZ.CPP 

#=============================================================
#		Compiler Configuration File
#=============================================================
fractz.cfg: fractz.mak
  copy &&|
-Oi
-Oz
-Ob
-Oe
-Oc
-L$(LIBPATH)
-I$(INCLUDEPATH)
-vi
-sm
-k-
-O
-Ot
-wamp
| fractz.cfg


