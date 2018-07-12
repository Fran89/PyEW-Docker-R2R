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
# both) lists of UNIX_MODULES and NT_MODULES. The analogous
# procedure should be followed when a module is removed from the
# Earthworm distribution.
#


#
# List all modules (for cleaning purposes)
#
ALL_MODULES = \
	menlo_report \
	tankplayer \
	ew2mseed \
	tankplayer_tools \
	trig2disk \
	wave_serverV \
	waveman2disk \
        cont_trig \
        runwws \
	archman \
	mseedarchplayer \
	ewmseedarchiver \
	ew2disk
	# compiling mole (ew2moledb) requires 'cmake'
	# mole


#
# List all modules to be compiled into the Solaris distribution
#
UNIX_MODULES = \
	ew2mseed \
	menlo_report \
	tankplayer \
	tankplayer_tools \
	trig2disk \
	wave_serverV \
	waveman2disk \
        cont_trig \
	archman \
	nq2wws \
	ewmseedarchiver \
	mseedarchplayer
	# compiling mole (ew2moledb) requires 'cmake'
	# mole

#
# List all modules to be compiled into the NT distribution
#
NT_MODULES = \
	menlo_report \
	runwws \
	tankplayer \
	tankplayer_tools \
	trig2disk \
	wave_serverV \
	waveman2disk \
        cont_trig \
	ewmseedarchiver \
	archman  \
	# compiling mole (ew2moledb) fails on MSVS 2015
	# mole



#
#  Top level directives
#
solaris: unix_modules
nt: nt_modules
unix: unix_modules


#
#  Unix directives
#
unix_modules:: FRC
	@for x in $(UNIX_MODULES); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make -f makefile.unix); \
	done


#these next two are for backwards compat purposes.
clean_solaris:
	make clean_unix	
clean_bin_solaris:
	make clean_bin_unix	

clean_unix:
	@for x in $(UNIX_MODULES); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f makefile.unix clean); \
	done

clean_bin_unix:
	@for x in $(UNIX_MODULES); \
	do \
		(cd $$x; echo Cleaning binaries in:; pwd; \
		make -f makefile.unix clean_bin); \
	done

#
#  NT directives
#
nt_modules:: FRC
	@echo "Making Earthworm archiving modules"
	@nt_make_subdirs $(NT_MODULES)

clean_nt:: FRC
	@for %x in ($(NT_MODULES)) \
	do \
		@nt_clean %x

clean_bin_nt:: FRC
	@for %x in ($(NT_MODULES)) \
	do \
		@nt_clbin %x

FRC:
