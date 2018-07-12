
DELIMITER //

DROP PROCEDURE IF EXISTS sp_ew_magnitudepicks//

CREATE  DEFINER = CURRENT_USER  PROCEDURE sp_ew_magnitudepicks(IN event_id bigint(20), IN l_version bigint(20), IN ew_instance varchar(80))
BEGIN
SELECT ew_instance.ewname, ew_scnl.sta, ew_scnl.cha, ew_scnl.net, ew_scnl.loc,ew_sqkseq.qkseq, ew_magnitude_phase.version, ew_module.modname, ew_magnitude_phase.modified,ew_magnitude_phase.*
	FROM ew_magnitude_phase
	JOIN ew_module ON (ew_magnitude_phase.fk_module=ew_module.id )
	JOIN ew_instance ON (ew_module.fk_instance=ew_instance.id)
	JOIN ew_sqkseq ON (ew_magnitude_phase.fk_sqkseq=ew_sqkseq.id)
	JOIN ew_scnl ON (ew_scnl.id=ew_magnitude_phase.fk_scnl)
	WHERE ew_sqkseq.qkseq = event_id AND ew_magnitude_phase.version = l_version AND ew_instance.ewname= ew_instance;
END//

DELIMITER ;

