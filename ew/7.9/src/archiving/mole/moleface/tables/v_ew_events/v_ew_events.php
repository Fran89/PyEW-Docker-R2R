<?php

class tables_v_ew_events {

    function getTitle(&$record){
	$title_content = 'Magnitude: '.$record->val('mag').' ('.$record->val('quality').') - Time: '.$record->val('ot_time').' - Region: Italia';
	return $title_content;
    } 

    function titleColumn(){
	return "CONCAT(ot_time, ' ', mag, ' ', quality)";
	// return "CONCAT(depth, ' ', lat, ' ', lon)";
    }

    function section__summary_info(&$record){

	$width=425;
	$height=350;
	$zoom=11;
	$lat = $record->val('lat');
	$lon = $record->val('lon');

	$map_content = '<iframe width="'.$width.'" height="'.$height.'" frameborder="0" scrolling="no" marginheight="0" marginwidth="0" src="http://maps.google.it/maps?f=q&amp;source=s_q&amp;hl=it&amp;geocode=&amp;q='.$lat.'+'.$lon.'&amp;sll='.$lat.','.$lon.'&amp;sspn=0.009729,0.021951&amp;ie=UTF8&amp;ll='.$lat.','.$lon.'&amp;spn=0.009729,0.021951&amp;t=h&amp;z='.$zoom.'&amp;output=embed"></iframe>';

	$map_content .= '<br />';

	$map_content .= '<small><a href="http://maps.google.it/maps?f=q&amp;source=embed&amp;hl=it&amp;geocode=&amp;q='.$lat.'+'.$lon.'&amp;sll='.$lat.','.$lon.'&amp;sspn=0.009729,0.021951&amp;ie=UTF8&amp;ll='.$lat.','.$lon.'&amp;spn=0.009729,0.021951&amp;t=h&amp;z='.$zoom.'" style="color:#0000FF;text-align:left">Visualizzazione ingrandita della mappa</a></small>';


	$summary_content = '<font size=+2>';
	$summary_content .= 'Quality: '.$record->val('quality').' - Magnitude: '.$record->val('mag').' - Region: ??????????';
	$summary_content .= '</font>';
	$summary_content .= '<br />';
       	$summary_content .= 'Depth: '.$record->val('depth').' - Latitude: '.$record->val('lat').' - Longitude: '.$record->val('lon');

	$summary_content .= '<br />';
	$summary_content .= '<hr align="left" width='.$width.'/>';
	$summary_content .= '<br />';
	$summary_content .= $map_content;

	return array(
	    'content' => $summary_content,
	    'class' => 'main',
	    'label' => 'Map',
	    'order' => -10000
	);
    }


};

?>

