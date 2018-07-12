<?php

class tables_ew_hypocenters_summary {

    function getTitle(&$record){
	// $region_name = $record->val('region');
	// $title_content = $record->val('mag_type').' '.$record->val('mag').' ('.$record->val('arc_quality').') '.Dataface_Table::datetime_to_string($record->val('ot_dt')).$region_name;
	$title_content = $record->val('mag_type').' '.$record->val('mag').' ('.$record->val('Q1').$record->val('Q2').$record->val('Q3').') '.$record->val('region').' '.Dataface_Table::datetime_to_string($record->val('ot_dt')).$region_name;
	return $title_content;
    } 

    function titleColumn(){
	return "CONCAT(IF(ot_dt IS NULL, 'XX', ot_dt), ' ', IF(mag IS NULL, 'XX', mag_type), ' ', IF(mag IS NULL, 'XX', mag), ' ', IF(arc_quality IS NULL, 'XX', arc_quality))";
    }

    /* Source from http://www.php.net/manual/en/function.mysql-fetch-array.php */
    /* Put the full result in one array */
    private function mysql_fetch_full_result_array($result)
    {
	$table_result=array();
	$r=0;
	while($row = mysql_fetch_assoc($result)){
	    $arr_row=array();
	    $c=0;
	    while ($c < mysql_num_fields($result)) {
		$col = mysql_fetch_field($result, $c);
		$arr_row[$col -> name] = $row[$col -> name];
		$c++;
	    }
	    $table_result[$r] = $arr_row;
	    $r++;
	}
	return $table_result;
    }

    private function mysql_exec_query_get_table($q)
    {
	$app =& Dataface_Application::getInstance();
	$query =& $app->getQuery();
	$result = mysql_query($q,$app->db());
	$ret = $this->mysql_fetch_full_result_array($result);
	mysql_free_result($result);
	return $ret;
    }


    private function get_station_info_from_arc($fk_sqkseq, $version){
	// $q="SELECT p.dist, s.sta, s.cha, s.net, s.loc, s.lat, s.lon, s.ele, s.name FROM ew_arc_phase p JOIN ew_scnl s ON (p.fk_scnl = s.id) WHERE p.fk_sqkseq = ".$fk_sqkseq." AND p.version = ".$version.";";
	$q="SELECT p.dist, s.sta, SUBSTR(s.cha, 1, 2) as cha, s.net, s.loc, AVG(s.lat) lat, AVG(s.lon) lon, AVG(s.ele) ele, s.name FROM ew_arc_phase p JOIN ew_scnl s ON (p.fk_scnl = s.id) WHERE p.fk_sqkseq = ".$fk_sqkseq." AND p.version = ".$version." GROUP BY s.sta, s.cha;";
	return $this->mysql_exec_query_get_table($q);
    }

    private function get_station_info_from_mag($fk_sqkseq, $version){
	// $q="SELECT p.dist, s.sta, s.cha, s.net, s.loc, s.lat, s.lon, s.ele, s.name FROM ew_magnitude_phase p JOIN ew_scnl s ON (p.fk_scnl = s.id) WHERE p.fk_sqkseq = ".$fk_sqkseq." AND p.version = ".$version.";";
	$q="SELECT p.dist, s.sta, SUBSTR(s.cha, 1, 2) as cha, s.net, s.loc, AVG(s.lat) lat, AVG(s.lon) lon, AVG(s.ele) ele, s.name FROM ew_magnitude_phase p JOIN ew_scnl s ON (p.fk_scnl = s.id) WHERE p.fk_sqkseq = ".$fk_sqkseq." AND p.version = ".$version." GROUP BY s.sta, s.cha;";
	return $this->mysql_exec_query_get_table($q);
    }

    function section__summary_info(&$record){

	$width="500";
	$height=500;
	$zoom=11;
	$lat = $record->val('lat');
	$lon = $record->val('lon');
	$fk_sqkseq = $record->val('fk_sqkseq');
	$version = $record->val('version');

	$station_arc = $this->get_station_info_from_arc($fk_sqkseq, $version);
	$station_mag = $this->get_station_info_from_mag($fk_sqkseq, $version);

	$station_arc_content = "";
	foreach($station_arc as $value) {
	    $station_arc_content .= $value["sta"].".".$value["cha"];
	    $station_arc_content .= ",";
	}
	$station_mag_content = "";
	foreach($station_mag as $value) {
	    $station_mag_content .= $value["sta"].".".$value["cha"];
	    $station_mag_content .= ",";
	}

	/* red triangle */
	$arc_link_icon_station_clean = "http://maps.google.com/mapfiles/kml/pal4/icon52.png";
	$arc_link_icon_station = str_replace("/", "%2F", $arc_link_icon_station_clean);
	/* green triangle  */
	$both_link_icon_station_clean = "http://maps.google.com/mapfiles/kml/pal4/icon60.png";
	$both_link_icon_station = str_replace("/", "%2F", $both_link_icon_station_clean);
	/* square and white flag  */
	$mag_link_icon_station_clean = "http://maps.google.com/mapfiles/kml/pal4/icon61.png";
	$mag_link_icon_station = str_replace("/", "%2F", $mag_link_icon_station_clean);

	/*   */
	// $undef_link_icon_station_clean = "http://maps.google.com/mapfiles/kml/pal3/icon59.png";
	// $undef_link_icon_station_clean = " http://google.com/mapfiles/ms/micons/orange-dot.png";
	$undef_link_icon_station_clean=str_replace("/", "%2F","http://google.com/mapfiles/ms/micons/caution.png");
	$undef_link_icon_station = str_replace("/", "%2F", $undef_link_icon_station_clean);
	/* red dot marker */
	// $link_icon_location=str_replace("/", "%2F","http://maps.google.com/mapfiles/ms/micons/red-dot.png");
	$link_icon_location=str_replace("/", "%2F","http://google.com/mapfiles/ms/micons/earthquake.png");
	$color_path_map = "0xFC6456";
	$weight_path_map = "2";

	$arc_station_coord_str = "&markers=icon:".$arc_link_icon_station."|shadow:false|";
	$mag_station_coord_str = "&markers=icon:".$mag_link_icon_station."|shadow:false|";
	$both_station_coord_str = "&markers=icon:".$both_link_icon_station."|shadow:false|";
	$paths_h_station .= "&path=color:".$color_path_map."|weight:".$weight_path_map;

	$station_main = array();
	foreach($station_arc as $value) {
	    $station_main[$value["sta"].$value["cha"]]["lat"] = $value["lat"];
	    $station_main[$value["sta"].$value["cha"]]["lon"] = $value["lon"];
	    $station_main[$value["sta"].$value["cha"]]["arc"] = 1;
	    $station_main[$value["sta"].$value["cha"]]["mag"] = 0;
	}
	foreach($station_mag as $value) {
	    $station_main[$value["sta"].$value["cha"]]["lat"] = $value["lat"];
	    $station_main[$value["sta"].$value["cha"]]["lon"] = $value["lon"];
	    if(!$station_main[$value["sta"].$value["cha"]]["arc"]) {
		$station_main[$value["sta"].$value["cha"]]["arc"] = 0;
	    }
	    $station_main[$value["sta"].$value["cha"]]["mag"] = 1;
	}
	// print_r($station_main);
	$arc_i = 0;
	$mag_i = 0;
	$both_i = 0;
	$flag_warning_station_no_coordinates = false;
	foreach($station_main as $key => $value) {
	    if(is_numeric($value["lat"]) && is_numeric($value["lon"]) ) {
		$paths_h_station .= "|".round($lat, 2).",".round($lon, 2)."|".round($value["lat"], 2).",".round($value["lon"], 2)."|".round($lat, 2).",".round($lon, 2);
		if(       $value["arc"] == 1  &&  $value["mag"] == 1) {
		    $both_i++;
		    if($both_i > 1) {
			$both_station_coord_str .= "|";
		    }
		    $both_station_coord_str .= round($value["lat"], 2).",".round($value["lon"], 2);
		} else if($value["arc"] == 1  &&  $value["mag"] == 0) {
		    $arc_i++;
		    if($arc_i > 1) {
			$arc_station_coord_str .= "|";
		    }
		    $arc_station_coord_str .= round($value["lat"], 2).",".round($value["lon"], 2);
		} else if($value["arc"] == 0  &&  $value["mag"] == 1) {
		    $mag_i++;
		    if($mag_i > 1) {
			$mag_station_coord_str .= "|";
		    }
		    $mag_station_coord_str .= round($value["lat"], 2).",".round($value["lon"], 2);
		} else {
		    /* This case should never occur */
		    echo "Error......";
		}
	    } else {
		$flag_warning_station_no_coordinates = true;
	    }
	}

	if($flag_warning_station_no_coordinates) {
	    $link_icon_location = $undef_link_icon_station;
	}
	$link_static_map = "http://maps.google.com/maps/api/staticmap?size=".$width."x".$height."&format=png8&maptype=hybrid&sensor=false&markers=icon:".$link_icon_location."|shadow:true|".$lat.",".$lon;
	if($arc_i > 0) {
	    if(strlen($link_static_map) + strlen($arc_station_coord_str) <= 1800) {
		$link_static_map .= $arc_station_coord_str;
	    }
	}
	if($mag_i > 0) {
	    if(strlen($link_static_map) + strlen($mag_station_coord_str) <= 1800) {
		$link_static_map .= $mag_station_coord_str;
	    }
	}
	if($both_i > 0) {
	    if(strlen($link_static_map) + strlen($both_station_coord_str) <= 1800) {
		$link_static_map .= $both_station_coord_str;
	    }
	}
	if(strlen($link_static_map) + strlen($paths_h_station) <= 1800) {
	    $link_static_map .= $paths_h_station;
	}

	$img_static_map = "<img class=\"MapClass\" alt=\"\" src=\"".$link_static_map."\"/>";

	$map_content = "";
	$map_content .= "<table border='0'>";
	$map_content .= "<tr>";

	$map_content .= "<td>";
	$map_content .= $img_static_map;
	$map_content .= "</td>";

	$map_content .= "<td>";
	$map_content .= '<iframe width="'.$width.'" height="'.$height.'" frameborder="0" scrolling="no" marginheight="0" marginwidth="0" src="http://maps.google.it/maps?f=q&amp;source=s_q&amp;hl=it&amp;geocode=&amp;q='.$lat.'+'.$lon.'&amp;sll='.$lat.','.$lon.'&amp;sspn=0.009729,0.021951&amp;ie=UTF8&amp;ll='.$lat.','.$lon.'&amp;spn=0.009729,0.021951&amp;t=h&amp;z='.$zoom.'&amp;output=embed"></iframe>';

	$map_content .= "</td>";
	$map_content .= "</tr>";

	$map_content .= "<tr>";

	$map_content .= "<td>";
	$map_content .= "<img src='".$both_link_icon_station_clean."' border='0'>Phases and magnitude";
	$map_content .= "&nbsp;";
	$map_content .= "&nbsp;";
	$map_content .= "&nbsp;";
	$map_content .= "<img src='".$arc_link_icon_station_clean."' border='0'>Only phases";
	$map_content .= "&nbsp;";
	$map_content .= "&nbsp;";
	$map_content .= "&nbsp;";
	$map_content .= "<img src='".$mag_link_icon_station_clean."' border='0'>Only magnitude";
	$map_content .= "&nbsp;";
	$map_content .= "</td>";

	$map_content .= "<td>";
	$map_content .= '<small><a href="http://maps.google.it/maps?f=q&amp;source=embed&amp;hl=it&amp;geocode=&amp;q='.$lat.'+'.$lon.'&amp;sll='.$lat.','.$lon.'&amp;sspn=0.009729,0.021951&amp;ie=UTF8&amp;ll='.$lat.','.$lon.'&amp;spn=0.009729,0.021951&amp;t=h&amp;z='.$zoom.'" style="color:#0000FF;text-align:left">Display on google map</a></small>';
	$map_content .= "</td>";

	$map_content .= "</tr>";
	$map_content .= "</table>";


	$summary_content .= $record->val('mag_type').' '.$record->val('mag');
	$summary_content .= ' ('.$record->val('arc_quality').')';
	$summary_content .= ' '.Dataface_Table::datetime_to_string($record->val('ot_dt')).' GMT';
	$summary_content .= '<br />';
	$summary_content .= $record->val('region');
	$summary_content .= '<br />';
       	$summary_content .= 'Latitude: '.$record->val('lat').' - Longitude: '.$record->val('lon').' - Depth: '.$record->val('z').' km';

	$summary_content .= '<br />';
	$summary_content .= '<hr align="left" width='.$width.'/>';
	$summary_content .= '<br />';
	$summary_content .= $map_content;
	// $summary_content .= '<br />';
	// $summary_content .= $station_arc_content;
	// $summary_content .= '<br />';
	// $summary_content .= $station_mag_content;
	// $summary_content .= '<br />';
	// $summary_content .= $img_static_map;

	return array(
	    'content' => $summary_content,
	    'class' => 'main',
	    'label' => 'Map',
	    'order' => -1000000
	);
    }


};

?>
