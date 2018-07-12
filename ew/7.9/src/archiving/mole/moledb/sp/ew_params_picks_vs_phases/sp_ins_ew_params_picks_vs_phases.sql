
-- CALLS('hew1_mole', '2010-12-28', '2010-12-29', 'FUORN', 'HHZ', 'CH', '--', 'MOD_PICK_EW', 11, NULL, 0, 11);
-- CALLS('hew1_mole', '2010-12-28', '2010-12-29', 'FUSIO', 'HHZ', 'CH', '--', 'MOD_PICK_EW', 4, 'MOD_EQASSEMBLE', 1, 3);

DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_params_picks_vs_phases`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_params_picks_vs_phases`(
    in_ewname              VARCHAR(80),
    in_starttime           DATETIME,
    in_endtime             DATETIME,
    in_Station             CHAR(5),
    in_Comp                CHAR(3),
    in_Net                 CHAR(2),
    in_Loc                 CHAR(2),
    in_modnamep            VARCHAR(32),
    in_npicks              BIGINT,
    in_modnamel            VARCHAR(32),
    in_nphases             BIGINT,
    in_ndiff               BIGINT
)
uscita: BEGIN
DECLARE l_fk_modulep              BIGINT DEFAULT NULL;
DECLARE l_fk_modulel              BIGINT DEFAULT NULL;
DECLARE l_fk_scnl                BIGINT DEFAULT NULL;

DECLARE l_fk_instance                 BIGINT;
DECLARE l_ewname               VARCHAR(80) DEFAULT NULL;
DECLARE l_starttime            DATETIME DEFAULT NULL;
DECLARE l_endtime              DATETIME DEFAULT NULL;
DECLARE l_modnamep             VARCHAR(32) DEFAULT NULL;
DECLARE l_modnamel             VARCHAR(32) DEFAULT NULL;
DECLARE l_npicks               BIGINT DEFAULT NULL;
DECLARE l_nphases              BIGINT DEFAULT NULL;
DECLARE l_ndiff                BIGINT DEFAULT NULL;

IF TRIM(in_ewname)='' THEN 
        SELECT 'ERROR: Ew instance name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_ewname=in_ewname;

IF TRIM(in_starttime)='' THEN 
        SELECT 'ERROR: start time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_starttime=in_starttime;

IF TRIM(in_endtime)='' THEN 
	SELECT 'ERROR: end time name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_endtime=in_endtime;

IF TRIM(in_modnamep)='' THEN 
        SELECT 'ERROR: modnamep can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_modnamep=in_modnamep;

SET l_modnamel=in_modnamel;

SET l_npicks = in_npicks;
SET l_nphases = in_nphases;
SET l_ndiff = in_ndiff;

CALL `sp_get_ew_instance_out`(l_ewname, l_fk_instance);
CALL `sp_get_ew_scnl_out`(in_Station, in_Comp, in_Net, in_Loc, l_fk_scnl);
CALL `sp_get_ew_module_out`(in_ewname, in_modnamep, l_fk_modulep);
IF in_modnamel IS NOT NULL THEN
    CALL `sp_get_ew_module_out`(in_ewname, in_modnamel, l_fk_modulel);
END IF;

INSERT INTO `ew_params_picks_vs_phases`(fk_instance, starttime, endtime, fk_scnl, fk_modulepick, npicks, fk_moduleloc, nphases, ndiff)
    VALUES(l_fk_instance, l_starttime, l_endtime, l_fk_scnl, l_fk_modulep, l_npicks, l_fk_modulel, l_nphases, l_ndiff);

END$$
DELIMITER ;

