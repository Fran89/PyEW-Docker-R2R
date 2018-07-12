
DELIMITER $$
DROP TRIGGER IF EXISTS `tr_ew_strongmotionII_a_i`$$
CREATE TRIGGER `tr_ew_strongmotionII_a_i`
AFTER INSERT ON `ew_strongmotionII`
    FOR EACH ROW
    BEGIN
-- INGV definition
    DECLARE l_qkseq                 BIGINT    DEFAULT NULL;
    DECLARE l_ewname                CHAR(80)  DEFAULT NULL;
    DECLARE l_writer_amp            VARCHAR(255)    DEFAULT NULL;
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

	SELECT CONCAT(IF(ISNULL(l_ewname), 'null', l_ewname), '#', IF(ISNULL(modname), 'null', modname)) INTO l_writer_amp
	FROM ew_module WHERE ew_module.id = NEW.fk_module;

	SELECT CONCAT(l_ewname, '#', m.modname) INTO l_writer_ev
	FROM ew_module m 
	JOIN ew_arc_summary e ON e.fk_module = m.id
	WHERE e.fk_sqkseq = NEW.fk_sqkseq 
	ORDER BY e.version DESC LIMIT 1;

	SELECT net, sta, cha, loc INTO l_net, l_sta, l_cha, l_loc
	FROM `ew_scnl`
	WHERE id = NEW.fk_scnl;

	--         INSERT INTO `err_sp` (sp_name, error, gtime)
	--         VALUES('sp_ins_stm_ew', 
	    --         CONCAT(l_qkseq, l_writer_amp, l_writer_ev, NEW.version, l_net, l_sta, l_cha, l_loc, NEW.t_dt, NEW.t_usec, NEW.t_alt_dt, NEW.t_alt_usec, NEW.altcode, NEW.pga, NEW.tpga_dt, NEW.tpga_usec, NEW.pgv, NEW.tpgv_dt, NEW.tpgv_usec, NEW.pgd, NEW.tpgd_dt, NEW.tpgd_usec, NEW.rsa), 
	    --         1);

	CALL seisev.`sp_ins_stm_ew`(l_qkseq, l_writer_amp, l_writer_ev, NEW.version, l_net, l_sta, l_cha, l_loc, NEW.t_dt, NEW.t_usec, NEW.t_alt_dt, NEW.t_alt_usec, NEW.altcode, NEW.pga, NEW.tpga_dt, NEW.tpga_usec, NEW.pgv, NEW.tpgv_dt, NEW.tpgv_usec, NEW.pgd, NEW.tpgd_dt, NEW.tpgd_usec, NEW.rsa
	);

    END IF;
--  END INGV CODE

END;
$$
DELIMITER ;
