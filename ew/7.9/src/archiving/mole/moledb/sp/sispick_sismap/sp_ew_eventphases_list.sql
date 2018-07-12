
DELIMITER //

DROP PROCEDURE IF EXISTS sp_ew_eventphases_list//

CREATE  DEFINER = CURRENT_USER  PROCEDURE sp_ew_eventphases_list(IN event_id bigint(20), IN loc_version bigint(20), IN ew_instance varchar(80))
BEGIN
SELECT ew_instance.ewname, ew_scnl.sta, ew_scnl.cha, ew_scnl.net, ew_scnl.loc,
ROUND(ew_arc_phase.dist, 2) as dist, 
IF(ew_arc_phase.Ponset='P', ew_arc_phase.Pres, '')  AS Pres, 
IF(ew_arc_phase.Ponset='P', ew_arc_phase.Pwt, '')  AS Pwt, 
IF(ew_arc_phase.Sonset='S', ew_arc_phase.Sres, '') AS Sres, 
IF(ew_arc_phase.Sonset='S', ew_arc_phase.Swt, '') AS Swt, 
IF(ew_arc_phase.Ponset='P', CONCAT(ew_arc_phase.Ponset, ew_arc_phase.Plabel), '') AS PLab, 
IF(ew_arc_phase.Ponset='P', CONCAT(CAST(ew_arc_phase.Pat_dt AS CHAR),SUBSTR(ROUND(ew_arc_phase.Pat_usec/1000000.0, 2), 2) ), '') AS 'P_phase_time',
IF(ew_arc_phase.Ponset='P', ew_arc_phase.Pqual, '') AS Pqual,
IF(ew_arc_phase.Sonset='S', CONCAT(ew_arc_phase.Sonset, ew_arc_phase.Slabel), '') AS SLab,
IF(ew_arc_phase.Sonset='S', CONCAT(CAST(ew_arc_phase.Sat_dt AS CHAR),  SUBSTR(ROUND(ew_arc_phase.Sat_usec/1000000.0, 2), 2) ), '') AS 'S_phase_time',
IF(ew_arc_phase.Sonset='S', ew_arc_phase.Squal, '') AS Squal,
ew_arc_phase.codalen, ew_arc_phase.codawt, ew_arc_phase.Pfm, ew_arc_phase.Sfm, ew_arc_phase.datasrc, ew_arc_phase.Md, ew_arc_phase.azm, ew_arc_phase.takeoff, ew_arc_phase.id, ew_sqkseq.qkseq, ew_arc_phase.version, ew_module.modname, ew_arc_phase.modified 
FROM ew_arc_phase 
JOIN ew_module ON (ew_arc_phase.fk_module=ew_module.id) 
JOIN ew_instance ON (ew_module.fk_instance=ew_instance.id) 
JOIN ew_sqkseq ON (ew_arc_phase.fk_sqkseq=ew_sqkseq.id) 
JOIN ew_scnl ON (ew_scnl.id=ew_arc_phase.fk_scnl) 
WHERE ew_sqkseq.qkseq = event_id AND ew_arc_phase.version = loc_version AND ew_instance.ewname= ew_instance;
END//

DELIMITER ;

