#
#       Top level makefile for Earthworm Glass
#                  adapted from
#             Lucky Vidmar Mon Feb 14 21:48:28 GMT 2000
#
#
#
# 2. To build (compile) glass on a Win32 PC:
#        nmake nt
#
# 4. To clean up the source tree, i.e. remove all *.o, *.obj, *~, *%,
#    and core files  on a PC running NT:
#        nmake clean_nt
#
# 6. To clean up the bin directory, i.e. remove all Earthworm programs,
#    on a PC running NT:
#        nmake clean_bin_nt
#
#

#
# If a new subfolder is created, it should be added to the 
#  list of SUBFOLDERs
#
SUBFOLDERS = \
	lib \
	modules \
	apps


#
#  Top level directives
#
nt: nt_modules


#
#  NT directives
#

clean_nt:: FRC
	@echo "Cleaning Glass"
	@nt_clean_tree $(SUBFOLDERS)
	del ..\lib\GlobalDebug.exp; del ..\lib\GlobalDebug.lib 

nt_modules:: FRC
	@echo "Making Glass for Earthworm"
	@nt_make_tree $(SUBFOLDERS)


FRC: