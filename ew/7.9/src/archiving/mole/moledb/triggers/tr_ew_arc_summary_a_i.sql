
DELIMITER $$
DROP TRIGGER IF EXISTS `tr_ew_arc_summary_a_i`$$
CREATE TRIGGER `tr_ew_arc_summary_a_i`
AFTER INSERT ON `ew_arc_summary`
    FOR EACH ROW
    BEGIN
	DECLARE l_arc_fk_module           BIGINT    DEFAULT NULL;
	DECLARE l_fk_sqkseq               BIGINT    DEFAULT NULL;
	DECLARE l_version               BIGINT    DEFAULT NULL;
	DECLARE l_ewname                CHAR(80)  DEFAULT NULL;
	DECLARE l_qkseq                 BIGINT    DEFAULT NULL;
	/* start definition INGV */
	DECLARE l_writer                VARCHAR(255)    DEFAULT NULL;
	/* end definition INGV */
    
	-- Set l_ewname and l_qkseq from table ew_sqkseq
	SELECT ewname,qkseq INTO l_ewname,l_qkseq FROM ew_instance, ew_sqkseq
	WHERE ew_sqkseq.fk_instance = ew_instance.id 
	AND ew_sqkseq.id = NEW.fk_sqkseq;

	-- Set l_arc_fk_module, l_fk_sqkseq, l_version
	SELECT DISTINCT arc_fk_module, fk_sqkseq,version  INTO l_arc_fk_module, l_fk_sqkseq, l_version
	FROM `ew_hypocenters_summary`
	WHERE
	    arc_fk_module = NEW.fk_module
	    AND fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version
	;

	-- Verifica che gia' esiste in ew_hypocenters_summary
	-- Se gia' e' presente esegue UPDATE altrimenti INSERT
	IF l_arc_fk_module = NEW.fk_module AND l_fk_sqkseq = NEW.fk_sqkseq AND l_version = NEW.version THEN
	UPDATE `ew_hypocenters_summary` 
	    SET ot_dt = NEW.ot_dt,
	    ot_usec = NEW.ot_usec,
	    lat = NEW.lat,
	    lon = NEW.lon,
	    z = NEW.z,
	    arc_quality = NEW.quality,
	    erh = NEW.erh,
	    erz = NEW.erz,
	    nphtot = NEW.nphtot,
	    gap = NEW.gap,
	    dmin = NEW.dmin,
	    rms = NEW.rms,
	    ewname = l_ewname,
	    arc_modified = NEW.modified
	WHERE
	    arc_fk_module = NEW.fk_module
	    AND fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version
	;
	ELSE
	INSERT INTO `ew_hypocenters_summary` 
	    SET ot_dt = NEW.ot_dt,
	    ot_usec = NEW.ot_usec,
	    lat = NEW.lat,
	    lon = NEW.lon,
	    z = NEW.z,
	    arc_quality = NEW.quality,
	    erh = NEW.erh,
	    erz = NEW.erz,
	    nphtot = NEW.nphtot,
	    gap = NEW.gap,
	    dmin = NEW.dmin,
	    rms = NEW.rms,
	    ewname = l_ewname,
	    arc_modified = NEW.modified,
	    arc_fk_module = NEW.fk_module,
	    fk_sqkseq = NEW.fk_sqkseq,
	    qkseq = l_qkseq,           
	    region = `fn_find_region_name`(NEW.lat, NEW.lon),
	    version = NEW.version
	;
	END IF;

--  START INGV CODE
	IF fn_check_if_host_fill_db() = 1 THEN

	    SELECT CONCAT(IF(ISNULL(l_ewname), 'null', l_ewname), '#', IF(ISNULL(modname), 'null', modname)) INTO l_writer
	    FROM ew_module WHERE id = NEW.fk_module;

	    -- INSERT INTO `err_sp` (sp_name, error, gtime)
	    -- VALUES('sp_ins_ev_ew', CONCAT(l_qkseq, ' ', l_writer, ' ', NEW.ot_dt, ' ', NEW.ot_usec, ' ', NEW.lat, ' ', NEW.lon, ' ',  NEW.z, ' ', NEW.nph, ' ', NEW.nphs, ' ', NEW.nphtot, ' ', NEW.nPfm, ' ', NEW.gap, ' ', NEW.dmin, ' ', NEW.rms, ' ', NEW.e0az, ' ', NEW.e0dp, ' ', NEW.e0, ' ', NEW.e1az, ' ', NEW.e1dp, ' ', NEW.e1, ' ', NEW.e2, ' ', NEW.erh, ' ', NEW.erz, ' ', NEW.version, ' ', NEW.quality), 1);

	    CALL seisev.`sp_ins_ev_ew`(l_qkseq, l_writer, NEW.ot_dt, NEW.ot_usec, NEW.lat, NEW.lon,  NEW.z, NEW.nph, NEW.nphs, NEW.nphtot, NEW.nPfm, NEW.gap, NEW.dmin, NEW.rms, NEW.e0az, NEW.e0dp, NEW.e0, NEW.e1az, NEW.e1dp, NEW.e1, NEW.e2, NEW.erh, NEW.erz, NEW.version, NEW.quality);

	END IF;
--  END INGV CODE

END;
$$
DELIMITER ;

