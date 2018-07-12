
DROP VIEW IF EXISTS `vw_cmplocations`;
CREATE VIEW `vw_cmplocations` AS 
SELECT e.fk_sqkseq, e.arc_quality, e.ot_dt, e.lat, e.lon, e.z, e.mag_type, e.mag, e.region, e.version, m.modname modarc, k.modname modmag, e.ewname, e.qkseq, 
sec_to_time(DATEDIFF(DATE(e.arc_modified), DATE(e.ot_dt))  + TIME_TO_SEC(TIME(e.arc_modified)) - TIME_TO_SEC(TIME(e.ot_dt))) as diff_ot_loc ,
sec_to_time(DATEDIFF(DATE(e.mag_modified), DATE(e.ot_dt))  + TIME_TO_SEC(TIME(e.mag_modified)) - TIME_TO_SEC(TIME(e.ot_dt))) as diff_ot_mag 

FROM ew_hypocenters_summary e
JOIN ew_module m ON e.arc_fk_module = m.id
JOIN ew_module k ON e.mag_fk_module = k.id
;


