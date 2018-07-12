-- Created Fri Apr 26 14:41:13 CEST 2013


-- TRIGGERS
DROP TRIGGER IF EXISTS tr_ew_arc_phase_a_i;
DROP TRIGGER IF EXISTS tr_ew_events_summary_from_arc_sum;
DROP TRIGGER IF EXISTS tr_ew_magnitude_phase_a_i;
DROP TRIGGER IF EXISTS tr_ew_events_summary_from_mag_sum;
DROP TRIGGER IF EXISTS tr_ew_strongmotionII_a_i;

-- PROCEDURES
DROP PROCEDURE IF EXISTS sp_ew_arcsummary;
DROP PROCEDURE IF EXISTS sp_ew_event;
DROP PROCEDURE IF EXISTS sp_ew_eventphases_list;
DROP PROCEDURE IF EXISTS sp_ew_event_instance;
DROP PROCEDURE IF EXISTS sp_ew_event_list;
DROP PROCEDURE IF EXISTS sp_ew_instance;
DROP PROCEDURE IF EXISTS sp_ew_last_event_id;
DROP PROCEDURE IF EXISTS sp_ew_magnitudepicks;
DROP PROCEDURE IF EXISTS sp_ew_show_hosts;
DROP PROCEDURE IF EXISTS sp_get_ew_instance_out;
DROP PROCEDURE IF EXISTS sp_get_ew_module_out;
DROP PROCEDURE IF EXISTS sp_get_ew_scnl_out;
DROP PROCEDURE IF EXISTS sp_get_ew_spkseq_out;
DROP PROCEDURE IF EXISTS sp_get_ew_sqkseq_out;
DROP PROCEDURE IF EXISTS sp_ins_ew_arc_phase;
DROP PROCEDURE IF EXISTS sp_ins_ew_arc_summary;
DROP PROCEDURE IF EXISTS sp_ins_ew_coda_scnl;
DROP PROCEDURE IF EXISTS sp_ins_ew_error;
DROP PROCEDURE IF EXISTS sp_ins_ew_link;
DROP PROCEDURE IF EXISTS sp_ins_ew_magnitude_phase;
DROP PROCEDURE IF EXISTS sp_ins_ew_magnitude_summary;
DROP PROCEDURE IF EXISTS sp_ins_ew_params_picks_vs_phases;
DROP PROCEDURE IF EXISTS sp_ins_ew_params_pick_sta;
DROP PROCEDURE IF EXISTS sp_ins_ew_pick_scnl;
DROP PROCEDURE IF EXISTS sp_ins_ew_quake2k;
DROP PROCEDURE IF EXISTS sp_ins_ew_scnl;
DROP PROCEDURE IF EXISTS sp_ins_ew_strongmotionII;
DROP PROCEDURE IF EXISTS sp_ins_ew_tracebuf;
DROP PROCEDURE IF EXISTS sp_sel_ew_event_phases;

-- FUNCTIONS
DROP FUNCTION IF EXISTS fn_delta;
DROP FUNCTION IF EXISTS fn_find_region_name;
DROP FUNCTION IF EXISTS fn_get_str_from_dt_usec;
DROP FUNCTION IF EXISTS fn_get_suffix_from_datetime;
DROP FUNCTION IF EXISTS fn_get_tbname;
DROP FUNCTION IF EXISTS fn_isdate;
DROP FUNCTION IF EXISTS fn_pnpoly;

-- VIEWS
DROP VIEW IF EXISTS v_ew_error_message;
DROP VIEW IF EXISTS v_ew_params_pick_id_last_revision;
DROP VIEW IF EXISTS v_ew_params_pick_sta_last_revision;
DROP VIEW IF EXISTS v_ew_params_picks_vs_phases;
DROP VIEW IF EXISTS v_ew_picks;
DROP VIEW IF EXISTS v_ew_quake_evolution;
DROP VIEW IF EXISTS vw_cmplocations;

-- TABLES
RENAME TABLE err_sp TO old_myisam_err_sp;
RENAME TABLE ew_arc_phase TO old_myisam_ew_arc_phase;
RENAME TABLE ew_arc_summary TO old_myisam_ew_arc_summary;
RENAME TABLE ew_error TO old_myisam_ew_error;
RENAME TABLE ew_events_summary TO old_myisam_ew_events_summary;
RENAME TABLE ew_instance TO old_myisam_ew_instance;
RENAME TABLE ew_link TO old_myisam_ew_link;
RENAME TABLE ew_magnitude_phase TO old_myisam_ew_magnitude_phase;
RENAME TABLE ew_magnitude_summary TO old_myisam_ew_magnitude_summary;
RENAME TABLE ew_module TO old_myisam_ew_module;
RENAME TABLE ew_params_pick_sta TO old_myisam_ew_params_pick_sta;
RENAME TABLE ew_params_picks_vs_phases TO old_myisam_ew_params_picks_vs_phases;
RENAME TABLE ew_pick_scnl TO old_myisam_ew_pick_scnl;
RENAME TABLE ew_quake2k TO old_myisam_ew_quake2k;
RENAME TABLE ew_region_geom TO old_myisam_ew_region_geom;
RENAME TABLE ew_region_kind TO old_myisam_ew_region_kind;
RENAME TABLE ew_scnl TO old_myisam_ew_scnl;
RENAME TABLE ew_spkseq TO old_myisam_ew_spkseq;
RENAME TABLE ew_sqkseq TO old_myisam_ew_sqkseq;
RENAME TABLE ew_strongmotionII TO old_myisam_ew_strongmotionII;
RENAME TABLE ew_tracebuf TO old_myisam_ew_tracebuf;
