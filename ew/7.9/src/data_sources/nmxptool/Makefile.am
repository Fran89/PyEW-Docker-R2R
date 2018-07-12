# mtheo
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib src
dist_doc_DATA = BUGS HISTORY README earthworm seiscomp_templates makefile.unix doc/html doc/man

EWMAKEFILEUX=earthworm/makefile.ux.nognu

dist-hook:
	ln -f -s ${EWMAKEFILEUX} makefile.unix
	rm -rf `find $(distdir)/earthworm -name CVS`
	rm -rf `find $(distdir)/seiscomp_templates -name CVS`
	rm -rf `find $(distdir)/earthworm -name .svn`
	rm -rf `find $(distdir)/seiscomp_templates -name .svn`
	rm -rf `find $(distdir)/doc -name CVS`
	rm -rf `find $(distdir)/doc -name .svn`

binary_distdir_FILES= \
	AUTHORS \
        BUGS \
        COPYING \
        HISTORY  \
        INSTALL \
        README  \
        README_DISTBIN \
        bin/nmxptool${EXEEXT} \
        earthworm/nmxptool.d \
        earthworm/nmxptool.desc \
        earthworm/nmxptool.list.txt \
	earthworm/nmxptool_cmd.html \
	earthworm/nmxptool_ovr.html \
        seiscomp_templates

if ENABLE_DLL_PTHREAD
binary_distdir_FILES+= bin/${DLL_PTHREAD}
endif


binary_distdir_suffix=${BINARY_DIST_NAME_SUFFIX}
binary_distdir=${distdir}-bin-${binary_distdir_suffix}

TMPTARFILE=tmp_${binary_distdir}.tar

dist-bin:
	echo ${binary_distdir}
	uname -a > README_DISTBIN
	rm -fr bin && mkdir bin && cp src/nmxptool${EXEEXT} bin/
if ENABLE_DLL_PTHREAD
	test -f "${DIR_PTHREAD}/${DLL_PTHREAD}" && cp ${DIR_PTHREAD}/${DLL_PTHREAD} bin/
endif
	rm -fr ${binary_distdir}
	mkdir ${binary_distdir}
	${AMTAR} -cf ${TMPTARFILE} ${binary_distdir_FILES}
	cd ${binary_distdir} && ${AMTAR} xpf ../${TMPTARFILE}
	rm -f ${TMPTARFILE}
	rm -fr `find ${binary_distdir} -name CVS`
	rm -fr `find ${binary_distdir} -name .svn`
	${AMTAR} chvf - ${binary_distdir} |  GZIP=$(GZIP_ENV) gzip -c > ${binary_distdir}.tar.gz
	rm -fr ${binary_distdir}

# http://www.gnu.org/software/automake/manual/html_node/Extending.html#Extending
clean-local:
	@echo "example of clean-local"

NMXPTOOLBINNAME=nmxptool${EXEEXT}
NMXPTOOLBINSRC=src/${NMXPTOOLBINNAME}

DIRBINEW=${EW_HOME}/${EW_VERSION}/bin
NMXPTOOLBINEW=${DIRBINEW}/${NMXPTOOLBINNAME}

SEISCOMPDIRTEMPLATES=${SEISCOMPDIR}/acquisition/templates/source
DIRBINSEISCOMP=${SEISCOMPDIR}/acquisition/bin
NMXPTOOLBINSEISCOMP=${DIRBINSEISCOMP}/${NMXPTOOLBINNAME}

if ENABLE_EW
install-ew-bin: ${NMXPTOOLBINSRC}
	test -d "${DIRBINEW}"
	${INSTALL_PROGRAM} '${NMXPTOOLBINSRC}' '${NMXPTOOLBINEW}'

install-ew-conf:
	test -d "${EW_PARAMS}"
	${INSTALL_DATA} 'earthworm/nmxptool.d' '${EW_PARAMS}/nmxptool.d'
	${INSTALL_DATA} 'earthworm/nmxptool.desc' '${EW_PARAMS}/nmxptool.desc'
	${INSTALL_DATA} 'earthworm/nmxptool.list.txt' '${EW_PARAMS}/nmxptool.list.txt'

install-ew-doc:
	test -d "${EW_HOME}/${EW_VERSION}/ewdoc/WEB_DOC/cmd"
	${INSTALL_DATA} 'earthworm/nmxptool_cmd.html' '${EW_HOME}/${EW_VERSION}/ewdoc/WEB_DOC/cmd/nmxptool_cmd.html'
	test -d "${EW_HOME}/${EW_VERSION}/ewdoc/WEB_DOC/ovr"
	${INSTALL_DATA} 'earthworm/nmxptool_ovr.html' '${EW_HOME}/${EW_VERSION}/ewdoc/WEB_DOC/ovr/nmxptool_ovr.html'

uninstall-ew-bin:
	rm -f ${NMXPTOOLBINEW}
endif

if ENABLE_SEEDLINK
install-seiscomp-bin: ${NMXPTOOLBINSRC}
	test -d "${DIRBINSEISCOMP}"
	${INSTALL_PROGRAM} '${NMXPTOOLBINSRC}' '${NMXPTOOLBINSEISCOMP}'

install-seiscomp-templates:
	test -d "${SEISCOMPDIRTEMPLATES}" 
	test -d "${SEISCOMPDIRTEMPLATES}/135_nmxptool"  || ./install-sh -c -d '${SEISCOMPDIRTEMPLATES}/135_nmxptool'
	${INSTALL_DATA} 'seiscomp_templates/135_nmxptool/chain_digiserv_station.tpl' '${SEISCOMPDIRTEMPLATES}/135_nmxptool/chain_digiserv_station.tpl'
	${INSTALL_DATA} 'seiscomp_templates/135_nmxptool/chain_digiserv_station_trig.tpl' '${SEISCOMPDIRTEMPLATES}/135_nmxptool/chain_digiserv_station_trig.tpl'
	${INSTALL_DATA} 'seiscomp_templates/135_nmxptool/key' '${SEISCOMPDIRTEMPLATES}/135_nmxptool/key'
	${INSTALL_DATA} 'seiscomp_templates/135_nmxptool/plugins.ini.tpl' '${SEISCOMPDIRTEMPLATES}/135_nmxptool/plugins.ini.tpl'
	${INSTALL_DATA} 'seiscomp_templates/135_nmxptool/seedlink_plugin.tpl' '${SEISCOMPDIRTEMPLATES}/135_nmxptool/seedlink_plugin.tpl'
	${INSTALL_DATA} 'seiscomp_templates/135_nmxptool/seedlink_station.tpl' '${SEISCOMPDIRTEMPLATES}/135_nmxptool/seedlink_station.tpl'
	test -d "${SEISCOMPDIRTEMPLATES}/136_nmxptool_dod"  || ./install-sh -c -d '${SEISCOMPDIRTEMPLATES}/136_nmxptool_dod'
	${INSTALL_DATA} 'seiscomp_templates/136_nmxptool_dod/chain_digiserv_station.tpl' '${SEISCOMPDIRTEMPLATES}/136_nmxptool_dod/chain_digiserv_station.tpl'
	${INSTALL_DATA} 'seiscomp_templates/136_nmxptool_dod/chain_digiserv_station_trig.tpl' '${SEISCOMPDIRTEMPLATES}/136_nmxptool_dod/chain_digiserv_station_trig.tpl'
	${INSTALL_DATA} 'seiscomp_templates/136_nmxptool_dod/key' '${SEISCOMPDIRTEMPLATES}/136_nmxptool_dod/key'
	${INSTALL_DATA} 'seiscomp_templates/136_nmxptool_dod/plugins.ini.tpl' '${SEISCOMPDIRTEMPLATES}/136_nmxptool_dod/plugins.ini.tpl'
	${INSTALL_DATA} 'seiscomp_templates/136_nmxptool_dod/seedlink_plugin.tpl' '${SEISCOMPDIRTEMPLATES}/136_nmxptool_dod/seedlink_plugin.tpl'
	${INSTALL_DATA} 'seiscomp_templates/136_nmxptool_dod/seedlink_station.tpl' '${SEISCOMPDIRTEMPLATES}/136_nmxptool_dod/seedlink_station.tpl'

uninstall-seiscomp-bin:
	rm -f ${NMXPTOOLBINSEISCOMP}
endif

