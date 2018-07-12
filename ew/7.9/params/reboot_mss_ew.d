#
#                     reboot_mss_ew's Configuration File
#
InRing           WAVE_RING      # Transport ring to find waveform data on,
MyModuleId   MOD_REBOOT_MSS_EW  # Module id for this instance of reboot_mss_ew,
HeartbeatInt          15        # Heartbeat interval in secs (if 0, send no heartbeats)
RebootGap            600        # Gaps longer than this many seconds cause an MSS reboot
ProgName          reboot_mss    # Name of stand-alone reboot program
Logout                 1        # If 0, reboot the MSS100.
                                # If 1, logout the MSS100 serial port.
#
#   These are the SCNL's to monitor
#   NOTE: if old Scn command is used, Loc will be set to "--".
#
#                                    MSS100
#     Sta Comp Net Loc      IP Address     aPassword  pPassword
#     ----------------      -----------------------------------
SCNL  TEST EHZ  UW --       172.25.16.73   abcde      fghij
