
DELIMITER $$
DROP TRIGGER IF EXISTS `tr_ew_arc_phase_a_i`$$
CREATE TRIGGER `tr_ew_arc_phase_a_i`
AFTER INSERT ON `ew_arc_phase`
    FOR EACH ROW
    BEGIN

-- INGV definition
    DECLARE l_qkseq                 BIGINT    DEFAULT NULL;
    DECLARE l_ewname                CHAR(80)  DEFAULT NULL;
    DECLARE l_writer_ph             VARCHAR(255)    DEFAULT NULL;
    DECLARE l_writer_ev             VARCHAR(255)    DEFAULT NULL;
    DECLARE l_net                   CHAR(2)         DEFAULT NULL;
    DECLARE l_sta                   VARCHAR(5)      DEFAULT NULL;
    DECLARE l_cha                   CHAR(3)         DEFAULT NULL;
    DECLARE l_loc                   CHAR(2)         DEFAULT NULL;
-- end INGV definition

--  START INGV CODE
    IF fn_check_if_host_fill_db() = 1 THEN

	SELECT ewname, qkseq INTO l_ewname,l_qkseq 
	FROM ew_instance, ew_sqkseq
	WHERE ew_sqkseq.fk_instance = ew_instance.id 
	AND ew_sqkseq.id = NEW.fk_sqkseq;

	SELECT CONCAT(IF(ISNULL(l_ewname), 'null', l_ewname), '#', IF(ISNULL(modname), 'null', modname)) INTO l_writer_ph
	FROM ew_module WHERE ew_module.id = NEW.fk_module;

	SELECT CONCAT(l_ewname, '#', m.modname) INTO l_writer_ev
	FROM ew_module m 
	JOIN ew_arc_summary e ON e.fk_module = m.id
	WHERE e.fk_sqkseq = NEW.fk_sqkseq 
	ORDER BY e.version DESC LIMIT 1;

        SELECT net, sta, cha, loc INTO l_net, l_sta, l_cha, l_loc
        FROM `ew_scnl`
        WHERE id = NEW.fk_scnl;
        
	-- INSERT INTO `err_sp` (sp_name, error, gtime)
	-- VALUES('sp_ins_ph_ew', CONCAT(l_qkseq, ', ', l_writer_ph, ', ', l_writer_ph, ', ', NEW.version, ', ', l_net, ', ', l_sta, ', ', l_cha, ', ', l_loc, ', ', NEW.Plabel, ', ', NEW.Slabel, ', ', NEW.Ponset, ', ', NEW.Sonset, ', ', NEW.Pat_dt, ', ', NEW.Pat_usec, ', ', NEW.Sat_dt, ', ', NEW.Sat_usec, ', ', NEW.Pres, ', ', NEW.Sres, ', ', NEW.Pqual, ', ', NEW.Squal, ', ', NEW.codalen, ', ', NEW.codawt, ', ', NEW.Pfm, ', ', NEW.Sfm, ', ', NEW.datasrc, ', ', NEW.Md, ', ', NEW.azm, ', ', NEW.takeoff, ', ', NEW.dist, ', ', NEW.Pwt, ', ', NEW.Swt, ', ', NEW.pamp, ', ', NEW.codalenObs), 1);

        CALL seisev.`sp_ins_ph_ew`(l_qkseq, l_writer_ph, l_writer_ev, NEW.version, l_net, l_sta, l_cha, l_loc, NEW.Plabel, NEW.Slabel, NEW.Ponset, NEW.Sonset, NEW.Pat_dt, NEW.Pat_usec, NEW.Sat_dt, NEW.Sat_usec, NEW.Pres, NEW.Sres, NEW.Pqual, NEW.Squal, NEW.codalen, NEW.codawt, NEW.Pfm, NEW.Sfm, NEW.datasrc, NEW.Md, NEW.azm, NEW.takeoff, NEW.dist, NEW.Pwt, NEW.Swt, NEW.pamp, NEW.codalenObs, NEW.ccntr_0, NEW.ccntr_1, NEW.ccntr_2, NEW.ccntr_3, NEW.ccntr_4, NEW.ccntr_5, NEW.caav_0, NEW.caav_1, NEW.caav_2, NEW.caav_3, NEW.caav_4, NEW.caav_5
        );

    END IF;
--  END INGV CODE

END;
$$
DELIMITER ;

