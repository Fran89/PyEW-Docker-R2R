1. Download PDL products (PDL)
	Run ProductClient using config.ini from EW's pdl2ew directory, after modifying [listener_exec] section's command 
		- your copy of pdl2ark (from the pdl2ew part of EW)
		- the directory for the original PC ARC (ark) messages to go into
2. Configure a copy of ark2arc to read from the directory of ark files and write to the directory of arc files
    It also needs to know where a sqlite db to keep track of these messages is.
    IMPORTANT: All paths must be absolute
3. Configure a copy of file2ew to read from the directory written to above, so it can ingest those messages into EW
4. (Optional) Configure a copy of eqfilter to filter out messages too far from locations specified in its config file
5. Conigure a copy of gmewhtmlemail:
	InRing: what ring to read from
	site_file: hypoinverse-compatible file of stations
	dam_file: CSV of list of dams to report on
	HTMLFile: basename/path of HTML files to be created
	EmailProgram: program to send email with
	EmailRecipient: whoe to send email to; can appaer multiple times
	db_path: Path to sqlite db file (same as specified for ark2arc)
	MaxSiteDist (defaults to 100): Maximum distance (in km) from origin for stations to appear in email
