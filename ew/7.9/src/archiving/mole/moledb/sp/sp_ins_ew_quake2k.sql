
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_quake2k`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_quake2k`(
in_ewname                VARCHAR(80)     /*Host running the EW Module*/,
in_modname               VARCHAR(32)     /*Module name*/,
in_qkseq                 INT             /*Sequence number*/,
in_quake_ot              VARCHAR(30)      /*Quake Origin Time*/,
in_lat                   DOUBLE      /*Latitude*/,
in_lon                   DOUBLE      /*Longitude*/,
in_depth                 DOUBLE      /*Depht => QUAKE2K->z*/,
in_rms                   DOUBLE      /*Rms Residual*/,
in_dmin                  DOUBLE      /*Distance to closest station*/,
in_ravg                  DOUBLE      /*Average or median station distance*/,
in_gap                   DOUBLE      /*Largest azimuth without picks*/,
in_nph                   INT         /*Linked phases*/
)
uscita: BEGIN
DECLARE l_ewname                VARCHAR(80);
DECLARE l_modname               VARCHAR(32);
DECLARE l_qkseq                 INT;
DECLARE l_quake_ot_dt           DATETIME;
DECLARE l_quake_ot_usec         BIGINT;
DECLARE l_lat                   DOUBLE;
DECLARE l_lon                   DOUBLE;
DECLARE l_depth                 DOUBLE;
DECLARE l_rms                   DOUBLE;
DECLARE l_dmin                  DOUBLE;
DECLARE l_ravg                  DOUBLE;
DECLARE l_gap                   DOUBLE;
DECLARE l_nph                   INT;

DECLARE l_fk_module               BIGINT;
DECLARE l_sqkseq                BIGINT;

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


IF TRIM(in_qkseq)='' THEN 
        SELECT 'ERROR: sequence number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_qkseq=in_qkseq;


IF TRIM(in_quake_ot)='' THEN 
        SELECT 'ERROR: origin time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;

SET l_quake_ot_dt=SUBSTR(in_quake_ot, 1, 19);
SELECT MICROSECOND(in_quake_ot) INTO l_quake_ot_usec ;


IF TRIM(in_lat)='' THEN 
        SELECT 'ERROR: latitude can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_lat=in_lat;

IF TRIM(in_lon)='' THEN 
        SELECT 'ERROR: longitude can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_lon=in_lon;

IF TRIM(in_depth)='' THEN 
        SELECT 'ERROR: depth can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_depth=in_depth;

IF TRIM(in_rms)='' THEN 
        SELECT 'ERROR: rms can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_rms=in_rms;

IF TRIM(in_dmin)='' THEN 
        SELECT 'ERROR: dmin can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_dmin=in_dmin;


IF TRIM(in_ravg)='' THEN 
        SELECT 'ERROR: ravg can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_ravg=in_ravg;

IF TRIM(in_gap)='' THEN 
        SELECT 'ERROR: gap can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_gap=in_gap;

IF TRIM(in_nph)='' THEN 
        SELECT 'ERROR: nph can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_nph=in_nph;


CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qkseq, l_sqkseq);

INSERT INTO `ew_quake2k` (fk_module,fk_sqkseq,quake_ot_dt,quake_ot_usec,lat,lon,depth,rms,dmin,ravg,gap,nph)
VALUES (l_fk_module,l_sqkseq,l_quake_ot_dt,l_quake_ot_usec,l_lat,l_lon,l_depth,l_rms,l_dmin,l_ravg,l_gap,l_nph);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

