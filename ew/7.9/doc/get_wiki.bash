#!/bin/bash
wget --recursive --no-clobber --no-parent --progress=dot --html-extension --page-requisites --restrict-file-names=windows --convert-links --exclude-directories='/trac/ew/wiki/RecentChanges' --reject "*?action=*","*?format=*"  http://earthwormcentral.com/trac/ew/wiki/WikiStart
mv earthwormcentral.com/trac/ew/* .
rm -rf earthwormcentral.com


