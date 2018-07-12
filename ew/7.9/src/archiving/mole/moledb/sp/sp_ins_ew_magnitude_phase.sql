
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_magnitude_phase`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_magnitude_phase`(
in_ewname                 VARCHAR(80)     /*Host running the EW Module*/,
in_modname                VARCHAR(32)     /*Module name*/,
in_qid                    BIGINT      /*quake id*/,
in_version                BIGINT      /*version number of the origin*/,
in_site                   VARCHAR(5)  /*site code, null terminated*/,
in_comp                   VARCHAR(4)  /*component code, null terminated*/,
in_net                    VARCHAR(3)  /*seismic network code, null terminated*/,
in_loc                    VARCHAR(2)  /*location code, null terminated*/,
in_mag                    DOUBLE      /*local magnitude for this channel*/,
in_dist                   DOUBLE      /*station-event distance used for local magnitude*/,
in_mag_correction         DOUBLE      /*mag_correction that was added to get this local mag*/,
in_Time1                  VARCHAR(30) /*time of the first pick - a string in datetime format*/,
in_Amp1                   DOUBLE      /*amplitude of the first pick*/,
in_Period1                DOUBLE      /*period associated with the first pick*/,
in_Time2                  VARCHAR(30) /*time of the second pick - a string in datetime format*/,
in_Amp2                   DOUBLE      /*amplitude of the second pick (if used)*/,
in_Period2                DOUBLE      /*period of the second pick (if used)*/
)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname     VARCHAR(32);
DECLARE l_qid         BIGINT     ;
DECLARE l_version     BIGINT     ;
DECLARE l_site        VARCHAR(5) ;
DECLARE l_net         VARCHAR(3) ;
DECLARE l_comp        VARCHAR(4) ;
DECLARE l_loc         VARCHAR(2) ;
DECLARE l_mag         DOUBLE     ;
DECLARE l_dist        DOUBLE     ;
DECLARE l_mag_correction  DOUBLE     ;
DECLARE l_Time1_dt   DATETIME   ;
DECLARE l_Time1_usec  BIGINT     ;
DECLARE l_Amp1        DOUBLE     ;
DECLARE l_Period1     DOUBLE     ;
DECLARE l_Time2_dt   DATETIME   ;
DECLARE l_Time2_usec  BIGINT     ;
DECLARE l_Amp2        DOUBLE     ;
DECLARE l_Period2     DOUBLE     ;

DECLARE l_fk_module               BIGINT;
DECLARE l_sqkseq                BIGINT;
DECLARE l_fk_scnl               BIGINT;

IF TRIM(in_ewname)='' THEN 
        SELECT 'ERROR: EW instance name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_ewname=in_ewname;

IF TRIM(in_modname)='' THEN 
        SELECT 'ERROR: modname can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_modname=in_modname;


IF TRIM(in_qid)='' THEN 
        SELECT 'ERROR: quake id can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_qid = in_qid;

SET l_version = in_version;
SET l_site = in_site;
SET l_net = in_net;
SET l_comp = in_comp;
SET l_loc = in_loc;

SET l_mag = in_mag;
SET l_dist = in_dist;
SET l_mag_correction = in_mag_correction;

SET l_Time1_dt = SUBSTR(in_Time1, 1, 19);
SELECT MICROSECOND(in_Time1) INTO l_Time1_usec;

SET l_Amp1 = in_Amp1;
SET l_Period1 = in_Period1;

SET l_Time2_dt = SUBSTR(in_Time2, 1, 19);
SELECT MICROSECOND(in_Time2) INTO l_Time2_usec;

SET l_Amp2 = in_Amp2;
SET l_Period2 = in_Period2;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qid, l_sqkseq);
CALL `sp_get_ew_scnl_out`(l_site, l_comp, l_net, l_loc, l_fk_scnl);

INSERT INTO `ew_magnitude_phase` (fk_module, fk_sqkseq, version, fk_scnl, mag, dist, mag_correction, Time1_dt, Time1_usec, Amp1, Period1, Time2_dt, Time2_usec, Amp2, Period2)
VALUES (l_fk_module,l_sqkseq, l_version, l_fk_scnl, l_mag, l_dist, l_mag_correction, l_Time1_dt, l_Time1_usec, l_Amp1, l_Period1, l_Time2_dt, l_Time2_usec, l_Amp2, l_Period2);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

