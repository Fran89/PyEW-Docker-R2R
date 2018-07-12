<?php
/**
 * File: index.php
 * Description:
 * -------------
 *
 * This is an entry file for this Dataface Application.  To use your application
 * simply point your web browser to this file.
 */

if ( !isset($_REQUEST['-table'])){
            $_REQUEST['-table'] = $_GET['-table'] = 'ew_hypocenters_summary';
}
if ( !isset($_REQUEST['-sort']) and @$_REQUEST['-table'] == 'ew_hypocenters_summary' ){
            $_REQUEST['-sort'] = $_GET['-sort'] = 'ot_dt desc';
}
if ( !isset($_REQUEST['-sort']) and @$_REQUEST['-table'] == 'ew_pick_scnl' ){
            $_REQUEST['-sort'] = $_GET['-sort'] = 'tpick_dt desc';
}
if ( !isset($_REQUEST['-sort']) and @$_REQUEST['-table'] == 'v_ew_params_pick_sta_last_revision' ){
            $_REQUEST['-sort'] = $_GET['-sort'] = 'sta';
}
if ( !isset($_REQUEST['-sort']) and @$_REQUEST['-table'] == 'ew_module' ){
            $_REQUEST['-sort'] = $_GET['-sort'] = 'fk_instance';
}
if ( !isset($_REQUEST['-sort']) and @$_REQUEST['-table'] == 'ew_scnl' ){
            $_REQUEST['-sort'] = $_GET['-sort'] = 'sta';
}


/* $dataface_version_for_moleface = "1.3.3"; */
$dataface_version_for_moleface = "2.0.1";

// use the timer to time how long it takes to generate a page
$time = microtime(true);

/** Parametrizza il path di Xataface*/ 
$SCRIPT_DIR = dirname($_SERVER['SCRIPT_FILENAME']);
$XATAFACE_DIR = preg_replace('/moleface/', 'xataface-'.$dataface_version_for_moleface, $SCRIPT_DIR);

// include the initialization file
require_once $XATAFACE_DIR.'/dataface-public-api.php';
// initialize the site
df_init(__FILE__, '../xataface-'.$dataface_version_for_moleface);

// get an application instance and perform initialization
$app =& Dataface_Application::getInstance();

// display the application
$app->display();


$time = microtime(true) - $time;
//echo "<p>Execution Time: $time</p>";

?>
