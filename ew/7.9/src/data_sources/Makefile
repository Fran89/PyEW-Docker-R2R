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
# List all modules (for cleaning purposes)
#
ALL_MODULES = \
	adsend \
	grf2ew \
	k2ew \
	naqs2ew \
	psnadsend \
	q2ew \
	q3302ew \
	reboot_mss \
	reboot_mss_ew \
	scream2ew  \
	nmxptool \
	srpar2ew \
	srusb2ew \
	winsdr2ew \
	samtac2ew


# note that the MAJORITY of the data_source modules do NOT work on Mac OS X
UNIX_MODULES = \
	grf2ew\
	k2ew \
	nmxptool \
	psnadsend \
	q2ew \
	q3302ew \
	scream2ew \
	srpar2ew \
	srusb2ew

#	gcf2ew
# gcf2ew is known to compile on FC4, may not work on other linuxes

#
# List all modules to be compiled into the Solaris distribution
#
SOLARIS_MODULES = \
	grf2ew\
	k2ew \
	naqs2ew \
	nmxptool \
	q2ew \
	q3302ew \
	reboot_mss \
	reboot_mss_ew \
	scream2ew

#
# List all modules to be compiled into the NT distribution
#

# turned off adsend because most folks don't have the NI includes.

NT_MODULES = \
	grf2ew\
	k2ew \
	naqs2ew \
	psnadsend \
	reboot_mss \
	q3302ew \
	reboot_mss_ew \
	sara \
	scream2ew  \
	srpar2ew \
	srusb2ew \
	winsdr2ew \
	samtac2ew

#       if you've got the NI stuff, uncomment adsend and put it back in 
#       the list above
#
#	adsend \
#
#
# DK 082402  Note:  The NT tree compilation will fail at adsend, unless you 
# comment out the adsend line above, or you have the National Instruments
#	header files to compile adsend.

#
#  Top level directives
#
solaris: solaris_modules
nt: nt_modules
unix: unix_modules


#
#  Unix directives
#

LINUX_MODULES=$(UNIX_MODULES)

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
	@echo "Making Earthworm data_sources modules"
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
