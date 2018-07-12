
DELIMITER $$
DROP TRIGGER IF EXISTS `tr_ew_magnitude_summary_a_i`$$
CREATE TRIGGER `tr_ew_magnitude_summary_a_i`
AFTER INSERT ON `ew_magnitude_summary`
    FOR EACH ROW
    BEGIN
	DECLARE l_mag_fk_module           BIGINT    DEFAULT NULL;
	DECLARE l_fk_sqkseq               BIGINT    DEFAULT NULL;
	DECLARE l_qkseq                 BIGINT    DEFAULT NULL;
	DECLARE l_version               BIGINT    DEFAULT NULL;

	DECLARE l_arc_fk_module           BIGINT    DEFAULT NULL;
	DECLARE l_ot_dt                 DATETIME  DEFAULT NULL;
	DECLARE l_ot_usec               INT       DEFAULT NULL;
	DECLARE l_lat                   DOUBLE    DEFAULT NULL;
	DECLARE l_lon                   DOUBLE    DEFAULT NULL;
	DECLARE l_z                     DOUBLE    DEFAULT NULL;
	DECLARE l_arc_quality           CHAR(2)   DEFAULT NULL;
	DECLARE l_erh                   DOUBLE    DEFAULT NULL;
	DECLARE l_erz                   DOUBLE    DEFAULT NULL;
	DECLARE l_nphtot                INT       DEFAULT NULL;
	DECLARE l_gap                   INT       DEFAULT NULL;
	DECLARE l_dmin                  INT       DEFAULT NULL;
	DECLARE l_rms                   DOUBLE    DEFAULT NULL;
	DECLARE l_arc_modified          DATETIME  DEFAULT NULL;
	DECLARE l_ewname                CHAR(80)  DEFAULT NULL;
	DECLARE l_region                VARCHAR(80)  DEFAULT NULL;

	/* start definition INGV */
	DECLARE l_writer_mag            VARCHAR(255)    DEFAULT NULL;
	DECLARE l_writer_ev             VARCHAR(255)    DEFAULT NULL;
	DECLARE l_moleslave             TINYINT         DEFAULT 0;
	/* end definition INGV */

	-- Set l_ewname and l_qkseq from table ew_sqkseq
	SELECT ewname,qkseq INTO l_ewname,l_qkseq FROM ew_instance, ew_sqkseq
	WHERE ew_sqkseq.fk_instance = ew_instance.id 
	AND ew_sqkseq.id = NEW.fk_sqkseq;

	-- Set l_mag_fk_module, l_fk_sqkseq, l_version
	SELECT DISTINCT mag_fk_module, fk_sqkseq,version  INTO l_mag_fk_module, l_fk_sqkseq, l_version
	FROM `ew_hypocenters_summary`
	WHERE
	    mag_fk_module = NEW.fk_module
	    AND fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version
	;

	-- Verifica che gia' esiste in ew_hypocenters_summary
	-- Se gia' e' presente esegue UPDATE altrimenti INSERT
	IF l_mag_fk_module = NEW.fk_module AND l_fk_sqkseq = NEW.fk_sqkseq AND l_version = NEW.version THEN
	UPDATE `ew_hypocenters_summary` 
	    SET mag = NEW.mag,
	    mag_error = NEW.error,
	    mag_quality = NEW.quality,
	    mag_type = NEW.szmagtype,
	    nstations = NEW.nstations,
	    nchannels = NEW.nchannels,
	    ewname = l_ewname,
	    mag_modified = NEW.modified,
	    mag_fk_module = NEW.fk_module
	WHERE
	    fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version
	;
	ELSE
	-- Verifica l'esitenza di mag_fk_module (ovvero se 'e uguale o meno a NULL)
	SELECT DISTINCT mag_fk_module, fk_sqkseq,version  INTO l_mag_fk_module, l_fk_sqkseq, l_version
	FROM `ew_hypocenters_summary`
	WHERE
	    fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version
	;
	-- Se l_mag_fk_module e' NULL, vuol dire che ho gia' inserito le info dall'arc_summary
	-- allora aggiorno solo i campi relativi alla magnitudo
	IF l_mag_fk_module IS NULL THEN
	    UPDATE `ew_hypocenters_summary` 
	    SET mag = NEW.mag,
	    mag_error = NEW.error,
	    mag_quality = NEW.quality,
	    mag_type = NEW.szmagtype,
	    nstations = NEW.nstations,
	    nchannels = NEW.nchannels,
	    ewname = l_ewname,
	    mag_modified = NEW.modified,
	    mag_fk_module = NEW.fk_module
	    WHERE
	    fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version;
	ELSE
	-- Se l_mag_fk_module e' diverso da NULL, vuol dire che 

	SELECT arc_fk_module, ot_dt, ot_usec, lat, lon, z, arc_quality, erh, erz, nphtot, gap, dmin, rms, region, arc_modified
	    INTO l_arc_fk_module, l_ot_dt, l_ot_usec, l_lat, l_lon, l_z, l_arc_quality, l_erh, l_erz, l_nphtot, l_gap, l_dmin, l_rms, l_region, l_arc_modified
	FROM `ew_hypocenters_summary`
	WHERE
	    fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version;

	INSERT INTO `ew_hypocenters_summary` 
	    SET mag = NEW.mag,
	    mag_error = NEW.error,
	    mag_quality = NEW.quality,
	    mag_type = NEW.szmagtype,
	    nstations = NEW.nstations,
	    nchannels = NEW.nchannels,
	    ewname = l_ewname,
	    mag_modified = NEW.modified,
	    mag_fk_module = NEW.fk_module,
	    fk_sqkseq = NEW.fk_sqkseq,
	    qkseq = l_qkseq,
	    version = NEW.version,
	    arc_fk_module = l_arc_fk_module ,
	    ot_dt = l_ot_dt,
	    ot_usec = l_ot_usec,
	    lat = l_lat,
	    lon = l_lon,
	    z = l_z,
	    arc_quality = l_arc_quality,
	    erh = l_erh,
	    erz = l_erz,
	    nphtot = l_nphtot,
	    gap = l_gap,
	    dmin = l_dmin,
	    rms = l_rms,
	    region = l_region,
	    arc_modified = l_arc_modified
	;
	END IF;
	END IF;

--  START INGV CODE
	IF fn_check_if_host_fill_db() = 1 THEN

	    SELECT CONCAT(IF(ISNULL(l_ewname), 'null', l_ewname), '#', IF(ISNULL(modname), 'null', modname)) INTO l_writer_mag
	    FROM ew_module WHERE id = NEW.fk_module;

	    SELECT CONCAT(l_ewname, '#', m.modname) INTO l_writer_ev
	    FROM ew_module m 
	    JOIN ew_arc_summary e ON e.fk_module = m.id
	    WHERE e.fk_sqkseq = NEW.fk_sqkseq LIMIT 1;

	    --         INSERT INTO `err_sp` (sp_name, error, gtime)
	    --         VALUES('sp_ins_mag_ew', 
		--         CONCAT(
		    --         IF(ISNULL(l_qkseq), 'null', l_qkseq), ' ', 
		    --         IF(ISNULL(l_writer_mag), 'null', l_writer_mag), ' ', 
		    --         IF(ISNULL(l_writer_ev), 'null', l_writer_ev), ' ', 
		    --         IF(ISNULL(NEW.version), 'null', NEW.version), ' ', 
		    --         IF(ISNULL(NEW.mag), 'null', NEW.mag), ' ', 
		    --         IF(ISNULL(NEW.error), 'null', NEW.error), ' ', 
		    --         IF(ISNULL(NEW.quality), 'null', NEW.quality), ' ', 
		    --         IF(ISNULL(NEW.mindist), 'null', NEW.mindist), ' ', 
		    --         IF(ISNULL(NEW.azimuth), 'null', NEW.azimuth), ' ', 
		    --         IF(ISNULL(NEW.nstations), 'null', NEW.nstations), ' ', 
		    --         IF(ISNULL(NEW.nchannels), 'null', NEW.nchannels), ' ', 
		    --         IF(ISNULL(CONCAT(NEW.szmagtype, ' ', NEW.algorithm)), 'null', CONCAT(NEW.szmagtype, ' ', NEW.algorithm))), 
		--         1);

	    CALL seisev.`sp_ins_mag_ew`(l_qkseq, l_writer_mag, l_writer_ev, NEW.version, NEW.mag, NEW.error, NEW.quality, NEW.mindist, NEW.azimuth, NEW.nstations, NEW.nchannels, NEW.mag_quality, CONCAT(NEW.szmagtype, '-', NEW.algorithm));

	END IF;
--  END INGV CODE

END;
$$
DELIMITER ;

