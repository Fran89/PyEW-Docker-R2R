# This is a mark-ed up command file for setting up an MSS100 to be connected
# to a K2 in the field. It is intended to be used as a guide for setting up
# your own MSS100s, using EZWebCon.
#
# In order to do the setup of an MSS100, follow the instructions in the
# Installation Guide for setting its IP address initially. This is best done
# with a network connection and address in your office subnet. Then connect to
# the MSS100 with EZWebCon and extract a cmd file to save to disk. Using this
# cmd file as a guide, make the necessary changes to the file you just
# downloaded. EZWebCon is on the CD that comes with the MSS, as well as at the
# Lantronix FTP site.
#
# See the MSS reference manual, available at the Lantronix Web site:
# http://www.lantronix.com/support/docs/
# Also on the CD that comes with the MSS.
#
# In this file, `[LOCAL]' means a parameter that will have a local value for
# your MSS, not the one that is listed here. `***' means an important setting
# that you should watch out for. `*' means a less important setting.
#
# Once you have made the changes to your cmd file, then you can use EZWebCon to
# upload that file back into the MSS. This uploading process will make all the
# settings listed here.
#
# You should consider setting password protection on the MSS100s.
#
# When you are ready to move the MSS into the field, you will need to set its
# IP address, subnet mask and gateway to the appropriate values for its new
# location. You could do this manually, by telnet-ing into the MSS config port
# (the standard telnet port) and issuing the commands. Note that these changes
# take effect as soon as you reboot the MSS, so you need to make sure you set
# the right values here. This process will need to be practiced to make sure
# you can do it. If all else fails, see the Note at the bottom of page 2 of
# the Installation Guide for setting factory defaults (press the reset button
# during power-up and boot.)

#Command File
#Source: Extract
#Script: javaman.mss.MSSScript
#Script Version: V1.1/8
#Server Name: 192.168.7.3
#Server Platform: MSS100
#Server Hardware Address: 00-80-a3-21-26-20
#Server Software Version: V3.6/3(000201)
#Java Version: 1.1.8
#Java Vendor: Sun Microsystems Inc., ported by the Blackdown Java-Linux Porting Team
#OS Name: Linux
#OS Architecture: x86
#OS Version: 2.2.12-20
#!% change ipaddress 192.168.7.3                [LOCAL] Set by other means
change access Dynamic                           ***
change autobaud Disabled                        ***
change autostart Disabled
change backward switch "NONE"
change bootgate NONE
change bootp Enabled
change break Local
change buffering 4096
change charsize 8                               ***
change dhcp Disabled                            ***
change domain "MSS-1"                           [LOCAL] *
change dsrlogout Disabled
change dtrwait Disabled
change flow control CTSRTS                      ***
change forward switch "NONE"
change gateway 192.168.7.1                      [LOCAL] ***
change inactive logout Disabled
change inactive timer 30
change incoming nopassword                      [LOCAL] *
change incoming telnet
change lat circtimer 80
change lat groups 0
change lat identification "MSS100"              [LOCAL]
change loadhost 192.168.7.1                     [LOCAL] ***
change local switch "NONE"                      *
change modem control Disabled                   *
change name "MSS_212620"                        [LOCAL]
change nameserver 0.0.0.0                       [LOCAL] *
change netware encapsulation 802_2 enabled
change netware encapsulation ether_ii enabled
change netware encapsulation native enabled
change netware encapsulation snap enabled
change netware internal network a3212620        [LOCAL]
change netware loadhost "NONE"                  [LOCAL]
change netware routing enabled
change parity None                              ***
change passflow Disabled                        ***
change password incoming Disabled               [LOCAL] ***
change password limit 3                         [LOCAL]
change password protect Disabled                [LOCAL] ***
change portname "Port_1"
change preferred none                           ***
change rarp Enabled
change retransmit limit 10                      *
change rlogin Disabled                          *
change secondary gateway 0.0.0.0                [LOCAL] *
change secondary loadhost 0.0.0.0               [LOCAL]
change secondary nameserver 0.0.0.0             [LOCAL] ***
change serial delay 30
change session limit 4
change signal check Disabled
change silentboot Disabled
change snmpsetcomm "none"
change software "MSS100.SYS"
change speed 19200                              [LOCAL] ***
change startup file NONE
change stopbits 1                               ***
change subnet mask 255.255.255.0                [LOCAL] ***
change telnetpad Enabled
change termtype "NONE"
change verify Enabled
