#!/webd1/scweb/Tools/perl -w -I/opt/PUBperl/lib/perl

$rootdir = "/webd1/ehzweb";
$webdir = "/waveforms/sgram";
$gifdir = "$rootdir"."$webdir";
$self  = "sgram.pl";
$gwid  = "150";
$ghgt  = "300";
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
	$group = "$query_string";
	($querytype,$qvalue) = split ('_',$query_string);
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
Northern California Seismic Network - Spectrograms (Jim Luetgert)
</TITLE></HEAD>
<BODY BGCOLOR=#EEEEEE TEXT=#333333 vlink=purple>
<A NAME="top"></A>

<H3><IMG SRC="/lv/Logos/USGS348sm.gif" ALT="USGS Logo" ALIGN="bottom" HSPACE=30>
<EM>U.S. Geological Survey - Internal Page</EM></H3>

<H3> <FONT color="green"> <CENTER>
Northern California Seismic Network - Spectrograms (Jim Luetgert)
</CENTER> </FONT> </H3>
<P>
 The Spectrograms displayed here are a selection of channels currently
 being recorded and served by the Earthworm waveservers in Menlo Park (and elsewhere).  
 These are each updated every five minutes to provide a (nearly) current record.
 Each panel represents 24 hours of data, sampled and processed once a minute.
 For detailed views in the Long Valley area, see the 
 <A HREF="/cgi-bin/spectra.pl-5">Bi-Hourly Spectrogram</A> site.

EOT

	$list = `ls $gifdir *.gif`;
	@list = split (' ',$list);
	@reverselist = reverse(@list);
# Get the lists of available sites and datetimes
	@site = ();
	@dates = ();
	foreach $item (@list) {
		($sitename = $item) =~ s/([^.]*)\.([^.]*).*/$2/;
		($filedate = $item) =~ s/([^.]*)\.([^.]*)\.(\d{8})(\d{2}).*\..*/$3/;
		($suffix = $item) =~ s/([^.]*)\.([^.]*)\.([^.]*)\.([^.]*).*/$4/;
		if($suffix eq "gif") {
			$flag = 1;
			foreach $name (@site) {
				if($sitename eq $name) { $flag = 0; }
			}
		    if($flag) { @site = (@site,$sitename); }
			$flag = 1;
			foreach $name (@dates) {
				if($filedate eq $name) { $flag = 0; }
			}
		    if($flag) { @dates = (@dates,$filedate); }
		}
	}
	@dates = sort(@dates);
	
	$i = 0;
	foreach $name (@site) {
		$i = $i + 1;
		$lname = $longname{$name};
		$inq = "1_"."$i";
		print STDOUT "<P><HR><P><H4>     ";
		print STDOUT "   [<A HREF=\"/cgi-bin/$self\?$inq\"> Thumbnails</A>] for $name \n";
		if($lname) {print STDOUT " ( $lname )  ";}
		print STDOUT "</H4><P>\n";
		foreach $item (@reverselist) {
			($sitename = $item) =~ s/([^.]*)\.([^.]*).*/$2/;
			if($sitename eq $name) {
				($datetime = $item) =~ s/([^.]*)\.([^.]*)\.(\d{8})(\d{2}).*\..*/$3_$4/;
				($date,$hr) = split ('_',$datetime);
			
				($datepart = $date) =~ s/(\d{4})(\d{2})(\d{2})/$1_$2_$3/;
				($year,$mon,$day) = split ('_',$datepart);
				print STDOUT "   | <A HREF=\"$webdir/$item\"> <B>$day/$mon/$year</B> </A> | \n";
			}
		}
	}
	
	print STDOUT "<P><HR>Thumbnails are available for the following dates:<BR>\n";
	@reversedates = reverse(@dates);
	foreach $item (@reversedates) {
		($datepart = $item) =~ s/(\d{4})(\d{2})(\d{2})/$1_$2_$3/;
		($year,$mon,$day) = split ('_',$datepart);
		$inq = "2_"."$item";
		print STDOUT " | <A HREF=\"/cgi-bin/$self\?$inq\"> <B>$day/$mon/$year</B></A> | \n";
	}
	print STDOUT "<P><HR><P><A HREF=\"/cgi-bin/$self\"><B>remake index</B></A> \n";

# Finish up the html file:
print STDOUT <<EOT;

<P><HR><font color=red></font>
<P><A HREF="#top">Top of this page</A>
 | <A HREF="../waveforms/wavesall/index.html">EQ waves (all)</A>
 | <A HREF="../waveforms/wavesbig/index.html">EQ waves (big)</A>
 | <A HREF="../waveforms/faultplane/index.html">Focal Mechanisms</A>
 <P>
 | <A HREF="../cgi-bin/sgram.pl">Daily Spectrograms</A>
 | <A HREF="../cgi-bin/spectra.pl-5">Bi-Hourly Spectrograms</A>
 | <A HREF="../cgi-bin/helirecord.pl">NCSN Helicorders</A>
 | <A HREF="../cgi-bin/helicorder.pl">Selected Helicorders</A>
 <P>
 | <A HREF="http://quake.wr.usgs.gov/recenteqs/Quakes/quakes.big.html">big earthquake list</A>
 | <A HREF="http://quake.wr.usgs.gov/recenteqs/Quakes/quakes0.html">all earthquakes list</A>
 | <A HREF="http://quake.wr.usgs.gov/">top page</A>
 | <A HREF="http://quake.wr.usgs.gov/credits.html">Credits</A>

<P>Click here for more info on the 
<A HREF="http://quake.wr.usgs.gov/VOLCANOES/LongValley/">Long Valley volcano monitoring effort</A>. 

<P>Click here for the 
<A HREF="../lv/">Long Valley internal page</A>. 

<P><HR><P>
For more information contact Jim Luetgert at
<I>
<a href="mailto:luetgert\@andreas.wr.usgs.gov"> luetgert\@andreas.wr.usgs.gov</a>
</I> <P>
EOT

return;

}

########################################################################
# Display thumbnails of all spectrograms for one site
# querytype = 1
# $qvalue is index to array of site names
########################################################################
sub show_site_thumbs {

print STDOUT <<EOT;
Content-type: text/html

<HTML>
<HEAD><TITLE>
NCSN  - Internal USGS Page - Spectrograms (Jim Luetgert)
</TITLE></HEAD>
<BODY BGCOLOR=#EEEEEE TEXT=#333333 vlink=purple>
<A NAME="top"></A>

<H3><IMG SRC="/lv/Logos/USGS348sm.gif" ALT="USGS Logo" ALIGN="bottom" HSPACE=30>
<EM>U.S. Geological Survey - Internal Page</EM></H3>

<H3> <FONT color="green"> <CENTER>
NCSN - Spectrograms (Jim Luetgert)
</CENTER> </FONT> </H3>

<P>
EOT

	$list = `ls $gifdir *.gif`;
	@list = split (' ',$list);
	@reverselist = reverse(@list);
	@site = ();
	$max = 0;
	foreach $item (@list) {
		($sitename = $item) =~ s/([^.]*)\.([^.]*).*/$2/;
		($suffix = $item) =~ s/([^.]*)\.([^.]*)\.([^.]*)\.([^.]*).*/$4/;
		if($suffix eq "gif") {
			$flag = 1;
			foreach $name (@site) {
				if($sitename eq $name) { $flag = 0; }
			}
		    if($flag) {
				@site = (@site,$sitename);
				$max = $max + 1;
			}
		}
	}
	
	$i = 0;
	foreach $name (@site) {
		$lname = $longname{$name};
		$i = $i + 1;
		if($i eq $qvalue) { 
			print STDOUT "<P><HR><P><H2>$name";
			if($lname) {print STDOUT " ( $lname )  ";}
			print STDOUT "</H2><P><BR>\n";
			foreach $item (@reverselist) {
				($sitename = $item) =~ s/([^.]*)\.([^.]*).*/$2/;
				if($sitename eq $name) {
					print STDOUT " <A HREF=\"$webdir\/$item\"><IMG SRC=\"${webdir}/$item\" WIDTH=$gwid HEIGHT=$ghgt></A> ";
				}
			}
			last; 
		}
	}
	
	print STDOUT " <P> <HR> <P> ";

	$psite = $qvalue - 1;
	if($psite < 1) { print STDOUT " Previous Site || "; }
	else {	
		$name = $site[$psite-1];
		$inq = "1_"."$psite";
		print STDOUT " <A HREF=\"/cgi-bin/$self\?$inq\">Previous Site ($name)</A> || ";
	}
	
	$nsite = $qvalue + 1;
	if($nsite > $max) { print STDOUT " Next Site || "; }
	else {	
		$name = $site[$nsite-1];
		$inq = "1_"."$nsite";
		print STDOUT " <A HREF=\"/cgi-bin/$self\?$inq\">Next Site ($name)</A> || ";
	}
	print STDOUT " <A HREF=\"/cgi-bin/$self\">Index</A> || ";

# Finish up the html file:
print STDOUT <<EOT;

<P><HR><font color=red></font>
<P><A HREF="#top">Top of this page</A>
 | <A HREF="../waveforms/wavesall/index.html">EQ waves (all)</A>
 | <A HREF="../waveforms/wavesbig/index.html">EQ waves (big)</A>
 <P>
 | <A HREF="../cgi-bin/sgram.pl">Daily Spectrograms</A>
 | <A HREF="../cgi-bin/spectra.pl-5">Bi-Hourly Spectrograms</A>
 | <A HREF="../cgi-bin/helirecord.pl">NCSN Helicorders</A>
 | <A HREF="../cgi-bin/helicorder.pl">Selected Helicorders</A>
 <P>
 | <A HREF="http://quake.wr.usgs.gov/recenteqs/Quakes/quakes.big.html">big earthquake list</A>
 | <A HREF="http://quake.wr.usgs.gov/recenteqs/Quakes/quakes0.html">all earthquakes list</A>
 | <A HREF="http://quake.wr.usgs.gov/">top page</A>
 | <A HREF="http://quake.wr.usgs.gov/credits.html">Credits</A>

<P>Click here for the 
<A HREF="../lv/">Long Valley internal page</A>. 

<P> <HR> <P>
For more information contact Jim Luetgert at
<I>
<a href="mailto:luetgert\@andreas.wr.usgs.gov"> luetgert\@andreas.wr.usgs.gov</a>
</I>
<P>
EOT
	
return;

}

########################################################################
# Display thumbnails of spectrograms for all sites for one day
# querytype = 2
# $qvalue of form YYYYMMDD - same as in file name
########################################################################
sub show_date_thumbs {

	$date = $qvalue;
	$sec0 = 0;
	$min0 = 0;
	$hour0 = 0;
	$mday0 = substr ($date,6,2);
	$mon0 = substr ($date,4,2) - 1;
	$year0 = substr ($date,2,2);
	$cent0 = substr ($date,0,2);
	$wday0 = 0;
	$yday0 = 0;
	$isdst0 = 0;
	@t0 = ($sec0,$min0,$hour0,$mday0,$mon0,$year0,$wday0,$yday0,$isdst0);

	$prev = $qvalue;
	$delhr = -24;
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = hr_increment (@t0,$delhr);
	$monplus = $mon + 1;
	$prev = sprintf "%2.2d%2.2d%2.2d%2.2d", $cent0,$year,$monplus,$mday;

	$next = $qvalue;
	$delhr = 24;
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = hr_increment (@t0,$delhr);
	$monplus = $mon + 1;
	$next = sprintf "%2.2d%2.2d%2.2d%2.2d", $cent0,$year,$monplus,$mday;

print STDOUT <<EOT;
Content-type: text/html

<HTML>
<HEAD><TITLE>
NCSN  - Internal USGS Page - Spectrograms (Jim Luetgert)
</TITLE></HEAD>
<BODY BGCOLOR=#EEEEEE TEXT=#333333 vlink=purple>
<A NAME="top"></A>

<H3><IMG SRC="/lv/Logos/USGS348sm.gif" ALT="USGS Logo" ALIGN="bottom" HSPACE=30>
<EM>U.S. Geological Survey - Internal Page</EM></H3>

<H3> <FONT color="green"> <CENTER>
NCSN - Spectrograms (Jim Luetgert)
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
		($filedate = $item) =~ s/([^.]*)\.([^.]*)\.(\d{8})(\d{2}).*\..*/$3/;
		($suffix = $item)   =~ s/([^.]*)\.([^.]*)\.([^.]*)\.([^.]*).*/$4/;
		if($suffix eq "gif") {
			if($filedate eq $qvalue) {
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
	print STDOUT "<P><H4>$day/$mon/$year</H4><P>\n";
	foreach $name (@gsite) { print STDOUT " | $name |"; }
	print STDOUT "<HR>";
	
	print STDOUT " <H4> ";
	foreach $item (@list) {
		($sitename = $item) =~ s/([^.]*)\.([^.]*).*/$2/;
		($filedate = $item) =~ s/([^.]*)\.([^.]*)\.(\d{8})(\d{2}).*\..*/$3/;
		($suffix = $item)   =~ s/([^.]*)\.([^.]*)\.([^.]*)\.([^.]*).*/$4/;
		if($suffix eq "gif") {
			if($filedate eq $qvalue) {
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
	if($flagp) { print STDOUT " Previous Day || "; }
	else {	
		$inq = "2_"."$prev";
		print STDOUT " <A HREF=\"/cgi-bin/$self\?$inq\">Previous Day</A> || ";
	}
	
	if($flagn) { print STDOUT " Next Day || "; }
	else {	
		$inq = "2_"."$next";
		print STDOUT " <A HREF=\"/cgi-bin/$self\?$inq\">Next Day</A> || ";
	}
	print STDOUT " <A HREF=\"/cgi-bin/$self\">Index</A> || ";

# Finish up the html file:
print STDOUT <<EOT;

<P><HR><font color=red></font>
<P><A HREF="#top">Top of this page</A>
 | <A HREF="../waveforms/wavesall/index.html">EQ waves (all)</A>
 | <A HREF="../waveforms/wavesbig/index.html">EQ waves (big)</A>
 <P>
 | <A HREF="../cgi-bin/sgram.pl">Daily Spectrograms</A>
 | <A HREF="../cgi-bin/spectra.pl-5">Bi-Hourly Spectrograms</A>
 | <A HREF="../cgi-bin/helirecord.pl">NCSN Helicorders</A>
 | <A HREF="../cgi-bin/helicorder.pl">Selected Helicorders</A>
 <P>
 | <A HREF="http://quake.wr.usgs.gov/recenteqs/Quakes/quakes.big.html">big earthquake list</A>
 | <A HREF="http://quake.wr.usgs.gov/recenteqs/Quakes/quakes0.html">all earthquakes list</A>
 | <A HREF="http://quake.wr.usgs.gov/">top page</A>
 | <A HREF="http://quake.wr.usgs.gov/credits.html">Credits</A>

<P>Click here for more info on the 
<A HREF="http://quake.wr.usgs.gov/VOLCANOES/LongValley/">Long Valley volcano monitoring effort</A>. 

<P>Click here for the 
<A HREF="../lv/">Long Valley internal page</A>. 

<P> <HR> <P>
For more information contact Jim Luetgert at
<I>
<a href="mailto:luetgert\@andreas.wr.usgs.gov"> luetgert\@andreas.wr.usgs.gov</a>
</I>
<P>
EOT

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

########################################################################
