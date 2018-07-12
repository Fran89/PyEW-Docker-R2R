
DELIMITER //

DROP PROCEDURE IF EXISTS sp_ew_arcsummary//

Create  DEFINER = CURRENT_USER  PROCEDURE sp_ew_arcsummary(IN event_id bigint(20), IN loc_version bigint(20), IN ew_instance varchar(80))
BEGIN
	SELECT  ew_arc_summary.* 
	FROM ew_arc_summary 
		join ew_sqkseq on (ew_arc_summary.fk_sqkseq = ew_sqkseq.id) 
	WHERE ew_sqkseq.fk_instance = (SELECT id 
					FROM ew_instance 
					WHERE ewname =ew_instance) 
		AND  ew_sqkseq.qkseq = event_id
		AND ew_arc_summary.version = loc_version;
END//

DELIMITER ;

