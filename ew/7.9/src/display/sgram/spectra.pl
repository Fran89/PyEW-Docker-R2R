#!/opt/PUBperl53/bin/perl -w -I/opt/PUBperl/lib/perl

$rootdir = "/webd1/ncweb";
$webdir = "/waveforms/sgramhr";
$gifdir = "$rootdir"."$webdir";
$self  = "spectra.pl";
$gwid  = "240";
$ghgt  = "421";
$numpergroup = 4;
	
if(open(NAMES, "$gifdir/znamelist.dat")) {
 	while(<NAMES>) {
		chomp;
		($sitename = $_) =~ s/([^.]*)\.([^.]*).*/$1/;
		($siteid = $_) =~ s/([^.]*)\.([^.]*).*/$2/;
		$longname{$sitename} = $siteid;
 	}
	close(NAMES);
}

$query_string = $ENV{'QUERY_STRING'};

if ( ! defined($query_string) || $query_string eq "") {
   &make_index;
}
else {
   $datehr = "$query_string";
	($querytype,$qdate,$qsite) = split ('_',$query_string);
   if($querytype eq 1) { &show_site_thumbs; }
   if($querytype eq 2) { &show_date_thumbs; }
}

exit;


########################################################################
sub make_index {

print STDOUT <<EOT;
Content-type: text/html

<HTML>
<HEAD><TITLE>
Long Valley  - Spectra 
</TITLE></HEAD>
<BODY BGCOLOR=#EEEEEE TEXT=#333333 vlink=purple>
<A NAME="top"></A>

<H3><IMG SRC="/waveforms/smusgs.gif" ALT="USGS Logo" ALIGN="bottom" HSPACE=30>
</H3>

<H3> <FONT color="green"> <CENTER>
Long Valley Page - Spectra 
</CENTER> </FONT> </H3>
<P>
 The Spectrograms displayed here are a selection of channels currently
 being recorded in the Long Valley area and served by the Menlo Park Earthworm waveservers.  
 These are each updated every five minutes to provide a (nearly) current record.
 Each panel represents two hours of data, sampled and processed once a minute.
 For an overview, see the 
 <A HREF="/cgi-bin/sgram.pl">Daily Spectrogram</A> site.
 <P><HR>

EOT

	$list = `ls $gifdir`;
	@list = split (' ',$list);
# Get the lists of available sites and datetimes
	@site = ();
	@dates = ();
	foreach $item (@list) {
		($sitename = $item) =~ s/([^.]*)\.([^.]*).*/$2/;
		($filedate = $item) =~ s/([^.]*)\.([^.]*)\.(\d{10}).*\..*/$3/;
		($suffix = $item) =~ s/([^.]*)\.([^.]*)\.([^.]*)\.([^.]*).*/$4/;
		if($suffix eq "gif") {
			$flag = 1;
			foreach $name (@site) {
				if($sitename eq $name) { $flag = 0; }
			}
		    if($flag) { @site = (@site,$sitename); }
			$flag = 1;
			foreach $idate (@dates) {
				if($filedate eq $idate) { $flag = 0; }
			}
		    if($flag) { @dates = (@dates,$filedate); }
		}
	}
	@dates = sort(@dates);

# List the Sites
	print STDOUT "<P>The following sites have spectrograms.<H4>";
	foreach $name (@site) { print STDOUT " | $name |"; }
	print STDOUT "</H4><P>\n";
	
# List the available times
	print STDOUT "<P><HR><B>Thumbnail groupings are available for the following dates and hours:</B><BR>\n";
	$lastdate = 0;
	foreach $item (@dates) {
		$inq = "2_"."$item"."_0";
		($datetime = $item) =~ s/(\d{8})(\d{2})/$1_$2/;
		($date,$hr) = split ('_',$datetime);
		if ( $date eq $lastdate ) { 
			print STDOUT "   <A HREF=\"/cgi-bin/$self\?$inq\"> $hr</A>\n";
		}
		else {
			($datepart = $date) =~ s/(\d{4})(\d{2})(\d{2})/$1_$2_$3/;
			($year,$mon,$day) = split ('_',$datepart);
			print STDOUT "<BR><B>$day/$mon/$year |</B> \n";
			print STDOUT "   <A HREF=\"/cgi-bin/$self\?$inq\"> $hr</A>\n";
			$lastdate = $date;
		}
	}
	
# List the Index of all images
	print STDOUT "<P><HR><B>Following is an index of all available spectrograms:</B>\n";
	$i = 0;
	foreach $name (@site) {
		$i = $i + 1;
		$lname = $longname{$name};
		$lastdate = 0;
		print STDOUT "<P><HR>$name \n";
		if($lname) {print STDOUT " ( $lname )  ";}
		foreach $item (@list) {
			($sitename = $item) =~ s/([^.]*)\.([^.]*).*/$2/;
			if($sitename eq $name) {
				($filedate = $item) =~ s/([^.]*)\.([^.]*)\.(\d{10}).*\..*/$3/;
				($datetime = $item) =~ s/([^.]*)\.([^.]*)\.(\d{8})(\d{2}).*\..*/$3_$4/;
				($date,$hr) = split ('_',$datetime);
				if ( $date eq $lastdate ) { 
					print STDOUT "   <A HREF=\"$webdir/$item\"> $hr</A>\n";
				}
				else {
					($datepart = $date) =~ s/(\d{4})(\d{2})(\d{2})/$1_$2_$3/;
					($year,$mon,$day) = split ('_',$datepart);
					$inq = "1_"."$date"."00_"."$i";
					print STDOUT " <BR>   <A HREF=\"/cgi-bin/$self\?$inq\"> <B>$day/$mon/$year</B> </A><B> | </B>\n";
					print STDOUT "   <A HREF=\"$webdir/$item\"> $hr</A>\n";
					$lastdate = $date;
				}
			}
		}
	}
	print STDOUT "<P><A HREF=\"/cgi-bin/$self\"><B>remake index</B></A> \n";

# Finish up the html file:
print STDOUT <<EOT;

<P><HR><font color=red></font>
<P><A HREF="#top">Top of this page</A>
 | <A HREF="../waveforms/wavesall/index.html">EQ waves (all)</A>
 | <A HREF="../waveforms/wavesbig/index.html">EQ waves (big)</A>
 | <A HREF="../waveforms/faultplane/index.html">Focal Mechanisms</A>
 <P>
 | <A HREF="../cgi-bin/sgram.pl">Daily Spectrograms</A>
 | <A HREF="../cgi-bin/spectra.pl">Bi-Hourly Spectrograms</A>
 | <A HREF="../cgi-bin/helirecord.pl">NCSN Helicorders</A>
 | <A HREF="../cgi-bin/helicorder.pl">Selected Helicorders</A>
 <P>
 | <A HREF="http://quake.wr.usgs.gov/recenteqs/Quakes/quakes.big.html">big earthquake list</A>
 | <A HREF="http://quake.wr.usgs.gov/recenteqs/Quakes/quakes0.html">all earthquakes list</A>
 | <A HREF="http://quake.wr.usgs.gov/">top page</A>
 | <A HREF="http://quake.wr.usgs.gov/credits.html">Credits</A>

<P>Click here for more info on the 
<A HREF="http://quake.wr.usgs.gov/VOLCANOES/LongValley/">Long Valley volcano monitoring effort</A>. 

<P> <HR> <P>
EOT

return;

}

########################################################################
# Display thumbnails of all spectrograms for one site for one day
# querytype = 1
# $qdate of form YYYYMMDDHH - same as in file name
# $qsite is index to array of site names
########################################################################
sub show_site_thumbs {

	$sec0 = 0;
	$min0 = 0;
	$hour0 = 0;
	$mday0 = substr ($qdate,6,2);
	$mon0 = substr ($qdate,4,2) - 1;
	$year0 = substr ($qdate,2,2);
	$cent0 = substr ($qdate,0,2);
	$wday0 = 0;
	$yday0 = 0;
	$isdst0 = 0;
	@t0 = ($sec0,$min0,$hour0,$mday0,$mon0,$year0,$wday0,$yday0,$isdst0);

	$delhr = -24;
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = hr_increment (@t0,$delhr);
	$monplus = $mon + 1;
	$pdate = sprintf "%2.2d%2.2d%2.2d%2.2d%2.2d", $cent0,$year,$monplus,$mday,$hour;

	$delhr = 24;
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = hr_increment (@t0,$delhr);
	$monplus = $mon + 1;
	$ndate = sprintf "%2.2d%2.2d%2.2d%2.2d%2.2d", $cent0,$year,$monplus,$mday,$hour;


print STDOUT <<EOT;
Content-type: text/html

<HTML>
<HEAD><TITLE>
NCSN  - Spectrograms 
</TITLE></HEAD>
<BODY BGCOLOR=#EEEEEE TEXT=#333333 vlink=purple>
<A NAME="top"></A>

<H3><IMG SRC="/waveforms/smusgs.gif" ALT="USGS Logo" ALIGN="bottom" HSPACE=30>
<EM>U.S. Geological Survey - Internal Page</EM></H3>

<H3> <FONT color="green"> <CENTER>
NCSN - Spectrograms 
</CENTER> </FONT> </H3>

<P>
EOT

	$list = `ls $gifdir *.gif`;
	@list = split (' ',$list);
	@site = ();
	@dates = ();
	$max = 0;
	foreach $item (@list) {
		($sitename = $item) =~ s/([^.]*)\.([^.]*).*/$2/;
		($filedate = $item) =~ s/([^.]*)\.([^.]*)\.(\d{8})(\d{2}).*\..*/$3/;
		($suffix = $item) =~ s/([^.]*)\.([^.]*)\.([^.]*)\.([^.]*).*/$4/;
		if($suffix eq "gif") {
			$flag = 1;
			foreach $name (@site) {
				if($sitename eq $name) { $flag = 0; }
			}
		    if($flag) { @site = (@site,$sitename); $max = $max + 1; }
			$flag = 1;
			foreach $idate (@dates) {
				if($filedate eq $idate) { $flag = 0; }
			}
		    if($flag) { @dates = (@dates,$filedate); }
		}
	}
	
	$testdate = substr ($qdate,0,8);
	($datepart = $testdate) =~ s/(\d{4})(\d{2})(\d{2})/$1_$2_$3/;
	($year,$mon,$day) = split ('_',$datepart);
	$name = $site[$qsite-1];
	$lname = $longname{$name};
	print STDOUT "<P><HR><P><H4>$name ";
	if($lname) {print STDOUT " ( $lname )  ";}
	print STDOUT " - $day/$mon/$year</H4><P><BR>\n";
	foreach $item (@list) {
		($sitename = $item) =~ s/([^.]*)\.([^.]*).*/$2/;
		($filedate = $item) =~ s/([^.]*)\.([^.]*)\.(\d{10}).*\..*/$3/;
		($datetime = $item) =~ s/([^.]*)\.([^.]*)\.(\d{8})(\d{2}).*\..*/$3_$4/;
		($date,$hr) = split ('_',$datetime);
		($suffix = $item) =~ s/([^.]*)\.([^.]*)\.([^.]*)\.([^.]*).*/$4/;
		if($suffix eq "gif") {
			if($sitename eq $site[$qsite-1]) {
				if($date eq $testdate) {
					print STDOUT " <A HREF=\"$webdir\/$item\"><IMG SRC=\"${webdir}/$item\" WIDTH=$gwid HEIGHT=$ghgt></A> ";
				}
			}
		}
	}
	
	print STDOUT " <P> <HR> <P> ";

	$psite = $qsite - 1;
	if($psite < 1) { print STDOUT " Previous Site || "; }
	else {	
		$name = $site[$psite-1];
		$inq = "1_"."$qdate"."_"."$psite";
		print STDOUT " <A HREF=\"/cgi-bin/$self\?$inq\">Previous Site ($name)</A> || ";
	}
	
	$nsite = $qsite + 1;
	if($nsite > $max) { print STDOUT " Next Site || "; }
	else {	
		$name = $site[$nsite-1];
		$inq = "1_"."$qdate"."_"."$nsite";
		print STDOUT " <A HREF=\"/cgi-bin/$self\?$inq\">Next Site ($name)</A> || ";
	}
	
	$flagp = 1;
	$flagn = 1;
	$tpdate = substr ($pdate,0,8);
	$tndate = substr ($ndate,0,8);
	foreach $item (@dates) {
		$tdate = substr ($item,0,8);
		if($tpdate eq $tdate) { $flagp = 0; }
		if($tndate eq $tdate) { $flagn = 0; }
	}
	if($flagp) { print STDOUT " Previous Day || "; }
	else {	
		$inq = "1_"."$pdate"."_"."$qsite";
		print STDOUT " <A HREF=\"/cgi-bin/$self\?$inq\">Previous Day</A> || ";
	}
	
	if($flagn) { print STDOUT " Next Day || "; }
	else {	
		$inq = "1_"."$ndate"."_"."$qsite";
		print STDOUT " <A HREF=\"/cgi-bin/$self\?$inq\">Next Day</A> || ";
	}
	print STDOUT " <A HREF=\"/cgi-bin/$self\">Index</A> || ";
	

return;

}

########################################################################
# Display thumbnails of spectrograms for all sites for one bi-hour
# querytype = 2
# $qdate of form YYYYMMDDHH - same as in file name
########################################################################
sub show_date_thumbs {

	$date = $qdate;
	$sec0 = 0;
	$min0 = 0;
	$hour0 = substr ($date,8,2);
	$mday0 = substr ($date,6,2);
	$mon0 = substr ($date,4,2) - 1;
	$year0 = substr ($date,2,2);
	$cent0 = substr ($date,0,2);
	$wday0 = 0;
	$yday0 = 0;
	$isdst0 = 0;
	@t0 = ($sec0,$min0,$hour0,$mday0,$mon0,$year0,$wday0,$yday0,$isdst0);

	$prev = $qdate;
	$delhr = -2;
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = hr_increment (@t0,$delhr);
	$monplus = $mon + 1;
	$prev = sprintf "%2.2d%2.2d%2.2d%2.2d%2.2d", $cent0,$year,$monplus,$mday,$hour;

	$next = $qdate;
	$delhr = 2;
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = hr_increment (@t0,$delhr);
	$monplus = $mon + 1;
	$next = sprintf "%2.2d%2.2d%2.2d%2.2d%2.2d", $cent0,$year,$monplus,$mday,$hour;

print STDOUT <<EOT;
Content-type: text/html

<HTML>
<HEAD><TITLE>
Bi-Hourly Spectrograms 
</TITLE></HEAD>
<BODY BGCOLOR=#EEEEEE TEXT=#333333 vlink=purple>
<A NAME="top"></A>

<H3><IMG SRC="/waveforms/smusgs.gif" ALT="USGS Logo" ALIGN="bottom" HSPACE=30>
<EM>U.S. Geological Survey - Internal Page</EM></H3>

<H3> <FONT color="green"> <CENTER>
Bi-Hourly Spectrograms 
</CENTER> </FONT> </H3>

<P>
EOT

	$list = `ls $gifdir *.gif`;
	@list = split (' ',$list);
	
	@gsite = ();
	@site  = ();
	@dates = ();
	foreach $item (@list) {
		($sitename = $item) =~ s/([^.]*)\.([^.]*).*/$2/;
		($filedate = $item) =~ s/([^.]*)\.([^.]*)\.(\d{10}).*\..*/$3/;
		($suffix = $item)   =~ s/([^.]*)\.([^.]*)\.([^.]*)\.([^.]*).*/$4/;
		if($suffix eq "gif") {
			if($filedate eq $qdate) {
				$flag = 1;
				foreach $name (@gsite) {
					if($sitename eq $name) { $flag = 0; }
				}
			    if($flag) { @gsite = (@gsite,$sitename); }
			}
			$flag = 1;
			foreach $name (@site) {
				if($sitename eq $name) { $flag = 0; }
			}
		    if($flag) { @site = (@site,$sitename); }
			$flag = 1;
			foreach $idate (@dates) {
				if($filedate eq $idate) { $flag = 0; }
			}
		    if($flag) { @dates = (@dates,$filedate); }
		}
	}

# List the Sites
	$day = substr ($date,6,2);
	$mon = substr ($date,4,2);
	$year = substr ($date,0,4);
	$hour1 = $hour0 + 2;
	print STDOUT "<P><H4>$day/$mon/$year - $hour0:00->$hour1:00 UTC</H4><P>\n";
	foreach $name (@gsite) { print STDOUT " | $name |"; }
	print STDOUT "<HR>";
	
	print STDOUT " <H4> ";
	foreach $item (@list) {
		($filedate = $item) =~ s/([^.]*)\.([^.]*)\.(\d{10}).*\..*/$3/;
		($suffix = $item)   =~ s/([^.]*)\.([^.]*)\.([^.]*)\.([^.]*).*/$4/;
		if($suffix eq "gif") {
			if($filedate eq $qdate) {
				print STDOUT " <A HREF=\"$webdir\/$item\"><IMG SRC=\"${webdir}/$item\" WIDTH=$gwid HEIGHT=$ghgt></A> ";
			}
		}
	}
	print STDOUT "</H4>";
	print STDOUT " <P> <HR> <P> ";
	
	$flagp = 1;
	$flagn = 1;
	foreach $item (@dates) {
		if($prev eq $item) { $flagp = 0; }
		if($next eq $item) { $flagn = 0; }
	}
	if($flagp) { print STDOUT " Previous Two Hours || "; }
	else {	
		$inq = "2_"."$prev"."_0";
		print STDOUT " <A HREF=\"/cgi-bin/$self\?$inq\">Previous Two Hours</A> || ";
	}
	
	if($flagn) { print STDOUT " Next Two Hours || "; }
	else {	
		$inq = "2_"."$next"."_0";
		print STDOUT " <A HREF=\"/cgi-bin/$self\?$inq\">Next Two Hours</A> || ";
	}
	print STDOUT " <A HREF=\"/cgi-bin/$self\">Index</A> || ";

return;

}

########################################################################
sub hr_increment {

# Take a time array of form ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
#  and increments by a given number of hours.
# Returns the new time array.

local (@t1) = splice (@_,0,9);
local ($delhr) = @_;

# print "t1 = @t1 \n";
# print "delhr = $delhr\n";

require "timelocal.pl";

my $t1seconds = &timegm(@t1);
my $t2seconds = $t1seconds + $delhr*3600.0;

@t2 = gmtime($t2seconds);

# print "t2 = @t2 \n";

return @t2;

}

