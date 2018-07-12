/* A colletion of functions to plot quakeML data */

var Debug = false;


/* Plot a google map, similarly to ewhtmlreport */
function QMLGMap( opt )
{
	var self = this;


	/* Check options */
	if( typeof opt.placeholderID === "undefined" )
	{
		console.error( "Invalid options." )
		return null;
	}

	/* Default options */
	self.gmapopt =
	{
		zoom: 3,
		center: new google.maps.LatLng( 0, 0 ),
		mapTypeId: google.maps.MapTypeId.HYBRID,
		streetViewControl: false
	};
	self.iconopt = {
		scaleopt: [1, 2],
		path: google.maps.SymbolPath.CIRCLE,
		fillOpacity: 0.5,
		strokeOpacity: 0.5,
		strokeColor: "#FF0000",
		fillColor: "#FF0000",
		strokeWeight: 1
	};


	/* Merge options with defaults */
	$.extend( true, self, opt );

	/* Create shared infowindow */
	self.infoWindow = new google.maps.InfoWindow()

	/* Start google map */
	self.gmap = new google.maps.Map(
		document.getElementById( self.placeholderID ),
		self.gmapopt );

	/* Update google map with first events */
	self.markers = [];
	if( typeof opt.xmldata !== "undefined" &&
		opt.xmldata !== null &&
		$( opt.xmldata ).length > 0 )
		self.update( opt.xmldata, true );
}



QMLGMap.prototype.update = function( xmldata, updateBounds, changedIcon )
{
	var self = this;
	
	/* Add new markers */
	var finalmarkers = [];
	$( xmldata ).find( "event" ).each( function( index, newevent ) {
		
		var evt = new QMLEvent( newevent );
		var location = new google.maps.LatLng( evt.lat, evt.lon );
		
		/* Marker scale */
		var scale = self.iconopt.scaleopt[0];
		if( $( evt.pmag ).length > 0 )
		{
			if( Debug )
				console.log( "Found preferred magnitude." );
			scale = self.getMarkerScale( evt.mag );
		}

		/* Infowindow contents */
		/* TODO: Add classes and ids to elements for css */
		var infoContent = '<table class="QML_MapMarkerTable" style="color:#8888aa;font-family: sans-serif;font-size: 12px;">' +
		'<tr><th>EW Event ID: ' + evt.eventId + '</th></tr>' +
		'<tr style="background-color:#eeeeff"><td>Origin Time</td><td>' + evt.ot + '</td></tr>' +
		'<tr><td>Latitude</td><td>' + evt.lat + '</td></tr>' +
		'<tr style="background-color:#eeeeff"><td>Longitude</td><td>' + evt.lon + '</td></tr>' +
		'<tr><td>Depth</td><td>' + evt.z + '</td></tr>' +
		'<tr style="background-color:#eeeeff"><td>Magnitude</td><td>' + evt.mag + " " + evt.magtype + '</td></tr>' + '</table>'

		/* Check if this is an old marker */
		var oldmarker = $.grep( self.markers, function( marker, index ) {
			if( marker.Id === evt.eventId )
				return true;
			return false;
		} );

		var marker;
		if( oldmarker.length > 0 )
		{
			/* This is a marker to be updated */
			/* Location */
			oldmarker[0].setPosition( location );
			/* Icon - Only updates magnitude */
			var oldicon = oldmarker[0].getIcon();
			$.extend( true, oldicon, {
				scale: scale
			} );
			oldmarker[0].setIcon( oldicon );
		}
		else
		{
			/* This is a new marker */
			/* Create google marker */
			var newicon = {
				scale: scale
			};
			if( changedIcon )
				$.extend( newicon, changedIcon );
			else
				$.extend( newicon, self.iconopt );
			marker = new google.maps.Marker(
			{
				position: location,
				map: self.gmap,
				flat: true,
				icon: newicon,
				zIndex: 1000,
				/* Custom parameters */
				Id: evt.eventId,
				version: evt.version,
				InfoContent: infoContent
			} );
			/* Set event for infobox */
			google.maps.event.addListener( marker, 'click', function()
			{
				//var event = events[marker.eventindex];
				self.infoWindow.setContent( marker.InfoContent );
				self.infoWindow.open( self.gmap, marker );
			} );
		}
		/* Add marker to array of markers */
		self.markers.push( marker );
	} );


	/* Check marker bounds */
	if( updateBounds )
	{
		var bounds = new google.maps.LatLngBounds();
		$( self.markers ).each( function( index, marker ) {
			bounds.extend( marker.getPosition() );
		} );
		self.gmap.fitBounds( bounds );
		return bounds;
	}
};

/* Determine the scale of a marker from the event magnitude */
QMLGMap.prototype.getMarkerScale = function( mag )
{
	var self = this;
	return self.iconopt.scaleopt[0] + mag * self.iconopt.scaleopt[1];
};

QMLGMap.prototype.focus = function( eventID, version )
{
	var i;
	var found = false;
	var self = this;
	for( i = 0; i < self.markers.length; i++ )
	{
		//console.log( self.markers[i].Id + ' - ' + eventID );
		if( self.markers[i].Id === eventID )
		{
			if( typeof version !== "undefined" && version.length > 0 )
			{
				if( self.markers[i].version === version )
				{
					found = true;
					break;
				}
			}
			else
			{
				found = true;
				break;
			}
		}
	}
	if( found )
	{
		/* Pan to marker */
		this.gmap.panTo(self.markers[i].getPosition());
		
		/* Zoom into marker */
		this.gmap.setZoom( 8 );
		
		/* Open infowindow */
		self.infoWindow.setContent( self.markers[i].InfoContent );
		self.infoWindow.open( self.gmap, self.markers[i] );
	}
	else //if( Debug )
		console.error( "Event " + eventID + " not found... " );
};

/* Automatically escape a jquery selector */
/*
QMLGMap.prototype.jQuerySelectorEscape = function( expression ) {
	return expression.replace( /[!"#$%&'()*+,.\/:;<=>?@\[\\\]^`{|}~]/g, '\\$&' );
};
*/


function QMLEvent( xmlevent )
{
	/* Extract marker parameters */
	var rawId = $( xmlevent ).attr( "publicID" );
	/* Finds last numeric element in event ID */
	var i;
	for( i = rawId.length; i >= 0; i--)
		if( isNaN( rawId.substr( i, 1 ) ) ) break;
	this.eventId = rawId.substr( i + 1 );
	
	/* Preferred Origin */
	this.preferredOriginID = $( xmlevent ).find( "preferredOriginID" ).text();
	if( Debug )
		console.log( "Pref. Origin: " + this.preferredOriginID );
	this.porigin = $( xmlevent ).find( "[publicID|=" +
		jQuerySelectorEscape( this.preferredOriginID ) + "]" );
	
	if( this.porigin.length === 0 )
	{
		if( Debug )
			console.log( "Did not find preferred origin." );
		return true;
	}
	if( Debug )
		console.log( "Found preferred origin." );

	/* Location */
	this.ot = $( this.porigin ).find( "time" ).find( "value" ).text();
	this.lat = parseFloat( $( this.porigin ).find( "latitude" ).text() );
	this.lon = parseFloat( $( this.porigin ).find( "longitude" ).text() );
	this.z = parseFloat( $( this.porigin ).find( "depth" ).text() ) / 1000;
	
	/* version */
	this.version = $(this.porigin).find("creationInfo").find("version").text();

	/* Preferred Magnitude */
	this.preferredMagnitudeID = $( xmlevent ).find( "preferredMagnitudeID" ).text();
	if( Debug )
		console.log( "Pref. Mag: " + this.preferredMagnitudeID );
	this.pmag = $( xmlevent ).find( "[publicID|=" +
		jQuerySelectorEscape( this.preferredMagnitudeID ) + "]" );

	/* Marker scale */
	this.mag = "";
	this.magtype = "";
	if( $( this.pmag ).length > 0 )
	{
		if( Debug )
			console.log( "Found preferred magnitude." );
		this.mag = parseFloat( $( this.pmag ).find( "mag" ).find( "value" ).text() );
		this.magtype = $( this.pmag ).find( "type" ).text();
	}
	else
	{
		if( Debug )
			console.log( "Did not find preferred magnitude." );
	}
	
	return null;
	
	
	function jQuerySelectorEscape( expression ) 
	{
		return expression.replace( /[!"#$%&'()*+,.\/:;<=>?@\[\\\]^`{|}~]/g, '\\$&' );
	}
	
}




/* Event table object */
function QMLEvtTable( opt )
{
	var self = this;


	/* Check options */
	if( typeof opt.placeholderID === "undefined" )
	{
		console.error( "Invalid options." )
		return null;
	}
	
	/* Merge options with defaults */
	$.extend( true, self, opt );
	
	/* Generate unique ID reference */
	self.uniq = 'id' + (new Date()).getTime();
	
	/* Define base elements of the table */
	self.placeholder = $( '#' + self.placeholderID );
	self.placeholder.css({
		overflow : 'hidden'
	});
	self.placeholder.append( '<div id="QML_EventsTableHeaderDiv_'+self.uniq+'">'+
		'<table id="QML_EventsTableHeader_'+self.uniq+'" class="QML_eventsTable">'+
		'<tr>'+
		'<th>ID</td>'+
		'<th>Date Time</th>'+
		'<th>Lat.</th>'+
		'<th>Lon.</th>'+
		'<th>Depth</th>'+
		'<th>Mag.</th>'+
		'<th>Mag. Type</th>'+
		//'<th></th>'+ // An additional cell for overflow
		'</tr>'+
		'</table></div>' +
		'<div id="QML_EventsTableBodyDiv_'+self.uniq+'"><table id="QML_EventsTable_'+self.uniq+'" class="QML_eventsTable">'+
		'</table></div>');
	self.headerDiv = $('#'+'QML_EventsTableHeaderDiv_'+self.uniq);
	self.headerTable = $('#'+'QML_EventsTableHeader_'+self.uniq);
	self.bodyDiv = $('#'+'QML_EventsTableBodyDiv_'+self.uniq);
	self.bodyTable = $('#'+'QML_EventsTable_'+self.uniq);
	
	self.bodyDiv.css(
	{
		//width : placeholder.width() + 'px',
		height : (self.placeholder.height() - self.headerDiv.height()) + 'px',
		'overflow-y' : 'scroll',
		'overflow-x' : 'hidden'
	} );
	
	/* Update table with first events */
	if( typeof opt.xmldata !== "undefined" &&
		opt.xmldata !== null &&
		$( opt.xmldata ).length > 0 )
		self.update( opt.xmldata, true );
}

QMLEvtTable.prototype.update = function( xmldata, clearAll, opts )
{
	var self = this;
	
	if( typeof clearAll === "undefined" )
		clearAll = false;
	
	if( clearAll )
		self.bodyTable.empty();
	
	/* Data header with source */
	if( typeof opts !== "undefined" )
	{
		if( typeof opts.source !== "undefined" )
		{
			var newrow = self.bodyTable[0].insertRow( self.bodyTable.find('tr').length );
			$(newrow).addClass("QML_EVT_Table_sourceRow")
			newrow.insertCell(0).innerHTML = opts.source;
			newrow.insertCell(1).innerHTML = "";
			newrow.insertCell(2).innerHTML = "";
			newrow.insertCell(3).innerHTML = "";
			newrow.insertCell(4).innerHTML = "";
			newrow.insertCell(5).innerHTML = "";
			newrow.insertCell(6).innerHTML = "";
		}
	}
	
	/* Process data */
	$( xmldata ).find( "event" ).each( function( i, newevent ) {
		/* Convert xml to javascript object */
		var event = new QMLEvent( newevent );
		
		/* Place data in the table body */
		var newrow = self.bodyTable[0].insertRow( self.bodyTable.find('tr').length );
		
		if ( i % 2 == 1) $( newrow ).addClass( 'QML_EVT_Table_oddRow' );
		else $( newrow ).addClass( 'QML_EVT_Table_evenRow' );
		
		/* Set id of class */
		$(newrow).attr('id',self.uniq + '_' + event.eventId);
		
		/* Insert data in new row */
		var newcell = newrow.insertCell(0);
		var span;
		if( event.version.length > 0 )
		{
			$(newcell).append('<span qid="'+event.eventId+
				'" style="cursor:pointer;text-decoration:underline">'+
				event.eventId+' ('+event.version+')'+'</span>');
			span = $(newcell).children()[0];
			$(span).click( function(obj)
			{
				self.clickFcn( event.eventId, event.version );
			} );
		}
		else
		{
			$(newcell).append('<span qid="'+event.eventId+
				'" style="cursor:pointer;text-decoration:underline">'+
				event.eventId+'</span>');
			span = $(newcell).children()[0];
			$(span).click( function(obj)
			{
				self.clickFcn( event.eventId );
			} );
		}
		newrow.insertCell(1).innerHTML = event.ot.substr( 0, 21 );
		newrow.insertCell(2).innerHTML = event.lat;
		newrow.insertCell(3).innerHTML = event.lon;
		newrow.insertCell(4).innerHTML = event.z;
		newrow.insertCell(5).innerHTML = event.mag;
		newrow.insertCell(6).innerHTML = event.magtype;
		
		
		/* Fix width of tables */
		self.headerTable.width( self.bodyTable.width() + 'px' );
			
		/* Fix width of cells in header table */
		var row = self.bodyTable.find('tr:first');//$( '#EventsTable tr:first' );
		var hrow = self.headerTable.find('th');// $('#EventsTableHeader th' );
		
		// 4 iterations
		$.each( row.children(), function( index, cell )
		{
			$( hrow[index] ).width( $( cell ).width() + 'px' );
		} );
	
	});
}