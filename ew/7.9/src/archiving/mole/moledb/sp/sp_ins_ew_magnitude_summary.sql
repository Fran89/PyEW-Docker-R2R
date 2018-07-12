
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_magnitude_summary`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_magnitude_summary`(

in_ewname          VARCHAR(80)     /*Host running the EW Module*/,
in_modname         VARCHAR(32)     /*Module name*/,
in_qid             BIGINT       /*event id from binder */,
in_origin_version  INT,
in_mag             DOUBLE,
in_error           DOUBLE,
in_quality         DOUBLE,
in_mindist         DOUBLE,
in_azimuth         INT,
in_nstations       INT,
in_nchannels       INT,
in_qauthor         VARCHAR(32),
in_qdds_version    INT,
in_imagtype        INT,
in_szmagtype       VARCHAR(32),
in_algorithm       VARCHAR(32),
in_mag_quality        CHAR(2)
)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname       VARCHAR(32);
DECLARE l_qid          BIGINT;
DECLARE l_origin_version INT;
DECLARE l_mag       DOUBLE;
DECLARE l_error     DOUBLE;
DECLARE l_quality   DOUBLE;
DECLARE l_mindist   DOUBLE;
DECLARE l_azimuth   INT;
DECLARE l_nstations INT;
DECLARE l_nchannels INT;
DECLARE l_qauthor   VARCHAR(32);
DECLARE l_qdds_version   INT;
DECLARE l_imagtype  INT;
DECLARE l_szmagtype VARCHAR(32);
DECLARE l_algorithm VARCHAR(32);
DECLARE l_mag_quality  CHAR(2);

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

SET l_qid=in_qid;
SET l_origin_version=in_origin_version;
SET l_mag=in_mag;
SET l_error=in_error;
SET l_quality=in_quality;
SET l_mindist=in_mindist;
SET l_azimuth=in_azimuth;
SET l_nstations=in_nstations;
SET l_nchannels=in_nchannels;
SET l_qauthor=in_qauthor;
SET l_qdds_version=in_qdds_version;
SET l_imagtype=in_imagtype;
SET l_szmagtype=in_szmagtype;
SET l_algorithm=in_algorithm;
SET l_mag_quality=in_mag_quality;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qid, l_sqkseq);

INSERT INTO `ew_magnitude_summary`(fk_module,fk_sqkseq,version,mag,error,quality,mindist,azimuth,nstations,nchannels,qauthor,qdds_version,imagtype,szmagtype,algorithm,mag_quality)
    VALUES (l_fk_module,l_sqkseq,l_origin_version,l_mag,l_error,l_quality,l_mindist,l_azimuth,l_nstations,l_nchannels,l_qauthor,l_qdds_version,l_imagtype,l_szmagtype,l_algorithm,l_mag_quality);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

