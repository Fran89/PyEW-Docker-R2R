#
#       Top level makefile for Menlo contributions to Earthworm
#       based on the Earthworm master-makefile by Lucky Vidmar
#
#
# This makefile contains all the directives necessary to compile
# earthworm under Solaris and NT, as well as clean up the Menlo
# source and binary file tree. It replaces the suite of programs in
# vx.x/bin, such as make_ew, make_ew_nt, clean_bin, clean_bin_ew, rm_o,
# etc... All functionality of those scripts should be available within
# this makefile, as follows:
#
# 1. To build (compile) the Menlo-specific Earthworm code on a Sun:
#        make solaris
#
# 2. To build (compile) the Menlo-specific Earthworm code on an NT PC:
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
# 5. To clean up the bin directory, i.e. remove all Menlo-specific programs,
#    on a Sun:
#        make clean_bin_solaris
#
# 6. To clean up the bin directory, i.e. remove all Menlo-specific programs,
#    on a PC running NT:
#        nmake clean_bin_nt
#
#
# Whenever a new module is added to Menlo/src, it should be added
# to this makefile to the appropriate (or both) lists of 
# SOLARIS_MODULES and NT_MODULES. 
# The analogous procedure should be followed when a module is 
# removed from the Menlo/src Earthworm distribution.
#

# If a new subfolder is created, it should be added to the
#  list of SUBFOLDERs
#
SUBFOLDERS = \
	heli2go \
	pager


# List all modules to be compiled into the Solaris distribution
#
SOLARIS_MODULES = \
	acfilt \
	addpinno \
	arc2trignc \
	copyevt \
	cta \
	cta2cusp \
	cta2sac \
	cube2trig \
	cuspfeeder \
	cvarc \
	dupfile \
	eqwaves \
	evt2ring \
	evtc \
	fwsoh2snw \
	getevent \
	getqdds \
	heli1 \
	helicont \
	helinow \
	html2txt \
	import_ms \
	k2f2audit \
	k2faudit \
	ls_cta \
	makehbfile \
	msubl \
	organizefile \
	pathcheck \
	qa2tracebuf \
	qdds2ring \
	qdm2cube \
	qshake_feeder \
	read_cta \
	sac2arc \
	seg2fnc \
	seg2toew \
	send_cta \
	sendarc \
	sfilercp \
	sfilescp \
	sgram \
	shake_feeder \
	sniff_trace \
	snwclient \
	sortevt \
	stationlist \
	tarseg2 \
	udmap \
	update_cta \
	write_cta \
	write_seg2 \
	wsv_test


# List all modules to be compiled into the NT distribution
#
NT_MODULES = \
	acfilt \
	addpinno \
	arc2trignc \
	cleandir \
	cleanwebdir \
	cuspfeeder \
	dupfile \
	evtc \
	fwsoh2snw \
	gpsalarm \
	heli1 \
	helicont \
	helisend \
	html2txt \
	k2talk \
	makehbfile \
	organizefile \
	pathcheck \
	reset_router \
	reset_router_ew \
	seg2fnc \
	seg2toew \
	sendarc \
	sgram \
	sniff_trace \
	snwclient \
	tempalarm \
	tickgen \
	wdog \
	wdog_ew \
	wsv_test

#
# List all stand-alone non-Earthworm programs & scripts
#
STANDALONE_MODULES = \
	dos_scripts \
	reboot \
	recover_id

#
#  Top level directives
#
solaris: sol_modules
nt: nt_modules


#
#  Solaris directives
#
sol_modules:: FRC
	@for x in $(SUBFOLDERS); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make solaris); \
	done
	@for x in $(SOLARIS_MODULES); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make -f makefile.sol); \
	done
	@for x in $(STANDALONE_MODULES); \
	do \
		(echo ------; cd $$x; echo Copying executables from $$x; \
		cp $$x $(EW_HOME)/$(EW_VERSION)/bin/.; cd ..); \
	done


clean_solaris:
	@for x in $(SUBFOLDERS); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f makefile.sol clean); \
	done
	@for x in $(SOLARIS_MODULES); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f makefile.sol clean); \
	done

clean_bin_solaris:
	@for x in $(SUBFOLDERS); \
	do \
		(cd $$x; echo Cleaning binaries in:; pwd; \
		make -f makefile.sol clean_bin); \
	done
	@for x in $(SOLARIS_MODULES); \
	do \
		(cd $$x; echo Cleaning binaries in:; pwd; \
		make -f makefile.sol clean_bin); \
	done

#
#  NT directives
#
nt_modules:: FRC
	@echo "Making Earthworm modules"
#	@for %x in ($(SUBFOLDERS)) \
#	do \
#		@nt_folder %x nt
	@for %x in ($(NT_MODULES)) \
	do \
		@nt_comp %x
	@for %x in ($(STANDALONE_MODULES)) \
	do \
		@nt_copy_exe %x

clean_nt:: FRC
	@for %x in ($(SUBFOLDERS)) \
	do \
		@nt_folder %x clean_nt
	@for %x in ($(NT_MODULES)) \
	do \
		@nt_clean %x

clean_bin_nt:: FRC
	@for %x in ($(SUBFOLDERS)) \
	do \
		@nt_folder %x clean_bin_nt
	@for %x in ($(NT_MODULES)) \
	do \
		@nt_clbin %x

FRC:
