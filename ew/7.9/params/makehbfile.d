#
# makehbfile.d  -  Configuration file for the makehbfile program
#
# Path     = Name of directory to contain heartbeat files.
# HbName   = Name of the heartbeat file (without path)
# Interval = Create a heartbeat file every "Interval" seconds
#
# Be sure the final line is terminated with a newline.
#
-HbName   hb.hollyhock
-Interval 60

# Solaris
-Path     /home/earthworm/outdir

# Windows
# -Path     c:\earthworm\outdir
