#
#       makefile for Earthworm
#             Lucky Vidmar Mon Jan 11 21:31:51 MST 1999
#
#
# This makefile contains all the directives necessary to compile
# earthworm modules in a directory under Solaris and NT, as well as 
# clean up the Earthworm source and binary file tree. 
#
# 1. To build (compile) the Earthworm modules on a Sun:
#        make solaris
#
# 2. To build (compile) the Earthworm modules on an NT PC:
#        nmake nt
#
# 3. To clean up the source tree, i.e. remove all *.o, *.obj, *~, *%,
#    and core files from all modules, on a Sun:
#        make clean_solaris
#
# 4. To clean up the source tree, i.e. remove all *.o, *.obj, *~, *%,
#    and core files from all modules, on a PC running NT:
#        nmake clean_nt
#
# 5. To clean up the bin directory, i.e. remove all Earthworm programs,
#    on a Sun:
#        make clean_bin_solaris
#
# 6. To clean up the bin directory, i.e. remove all Earthworm programs,
#    on a PC running NT:
#        nmake clean_bin_nt
#
#
#
# Whenever a new module is added to Earthworm, it should be added to
# this makefile to the list of ALL_MODULES, and to the appropriate (or
# both) lists of SOLARIS_MODULES and NT_MODULES. The analogous
# procedure should be followed when a module is removed from the
# Earthworm distribution.
#
#

#
# List all modules (for cleaning purposes)
#
ALL_MODULES = \
	config_ws \
	dumpwave \
	file2ring \
	findwave \
	gaplist \
	getmenu \
	getstation \
	latency_mon \
	maketrace \
	putpick \
	sniffring \
	sniffrings \
	sniffwave \
	template \
	wave_probe \
	wsv_test


#
# List all modules to be compiled into the Solaris distribution
#
SOLARIS_MODULES = \
	dumpwave \
	file2ring \
	findwave \
	gaplist \
	getmenu \
	getstation \
	putpick \
	sniffring \
	sniffrings \
	sniffwave \
	template \
	wsv_test

#
# List all modules to be compiled into the NT distribution
#
NT_MODULES = \
	config_ws \
	dumpwave \
	file2ring \
	findwave \
	gaplist \
	getmenu \
	getstation \
	latency_mon \
	maketrace \
	putpick \
	sniffring \
	sniffrings \
	sniffwave \
	template \
	wave_probe \
	wsv_test

#
# List all modules to be compiled into the UNIX distribution
#
UNIX_MODULES = \
	dumpwave \
	file2ring \
	findwave \
	gaplist \
	getmenu \
	getstation \
	putpick \
	sniffring \
	sniffrings \
	sniffwave \
	template 

LINUX_MODULES = $(UNIX_MODULES)

#
#  Top level directives
#
solaris: solaris_modules
nt: nt_modules
unix: unix_modules

#
#  Unix directives
#
unix_modules:: FRC
	@for x in $(LINUX_MODULES); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make -f makefile.unix); \
	done

clean_unix:
	@for x in $(LINUX_MODULES); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f makefile.unix clean); \
	done

clean_bin_unix:
	@for x in $(LINUX_MODULES); \
	do \
		(cd $$x; echo Cleaning binaries in:; pwd; \
		make -f makefile.unix clean_bin); \
	done

solaris_modules:: FRC
	@for x in $(SOLARIS_MODULES); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make -f makefile.unix); \
	done

clean_solaris:
	@for x in $(SOLARIS_MODULES); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f makefile.unix clean); \
	done

clean_bin_solaris:
	@for x in $(SOLARIS_MODULES); \
	do \
		(cd $$x; echo Cleaning binaries in:; pwd; \
		make -f makefile.unix clean_bin); \
	done



#
#  NT directives
#
nt_modules:: FRC
	@echo "Making Earthworm diagnostic_tools"
	@nt_make_subdirs $(NT_MODULES)

clean_nt:: FRC
	@for %x in ($(ALL_MODULES)) \
	do \
		@nt_clean %x

clean_bin_nt:: FRC
	@for %x in ($(ALL_MODULES)) \
	do \
		@nt_clbin %x

FRC:
