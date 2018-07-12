
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_arc_summary`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_arc_summary`(
in_ewname       VARCHAR(80)     /*Host running the EW Module*/,
in_modname      VARCHAR(32)     /*Module name*/,
in_qid          BIGINT       /*event id from binder */,
in_ot           VARCHAR(30)  /*Origin Time - datetime string */,
in_lat          DOUBLE       /*latitude (North=positive) */,
in_lon          DOUBLE       /*longitude(East=positive) */,
in_z            DOUBLE       /*depth (down=positive) */,
in_nph          INT          /*# phases (P&S) w/ weight >0.1 */,
in_nphS         INT          /*# S phases w/ weight >0.1 */,
in_nphtot       INT          /*# phases (P&S) w/ weight >0.0 */,
in_nPfm         INT          /*# P first motions */,
in_gap          INT          /*maximum azimuthal gap */,
in_dmin         INT          /*distance (km) to nearest station */,
in_rms          DOUBLE       /*RMS travel time residual */,
in_e0az         INT          /*azimuth of largest principal error */,
in_e0dp         INT          /*dip of largest principal error */,
in_e0           float        /*magnitude (km) of largest principal error */,
in_e1az         INT          /*azimuth of intermediate principal error */,
in_e1dp         INT          /*dip of intermediate principal error */,
in_e1           DOUBLE       /*magnitude (km) of intermed principal error */,
in_e2           DOUBLE       /*magnitude (km) of smallest principal error */,
in_erh          DOUBLE       /*horizontal error (km) */,
in_erz          DOUBLE       /*vertical error (km) */,
in_Md           DOUBLE       /*duration magnitude */,
in_reg          VARCHAR(4)   /*location region */,
in_labelpref    CHAR         /*character describing preferred magnitude */,
in_Mpref        DOUBLE       /*preferred magnitude */,
in_wtpref       DOUBLE       /*weight (~ # readings) of preferred Mag */,
in_mdtype       CHAR         /*Coda duration magnitude type code */,
in_mdmad        DOUBLE       /*Median-absolute-difference of duration mags */,
in_mdwt         DOUBLE       /*weight (~ # readings) of Md */,
in_version      BIGINT       /*version number of the origin */,
in_quality      CHAR(2)      /*INGV quality code of the location */
)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname      VARCHAR(32);
DECLARE l_qid          BIGINT;
DECLARE l_ot_dt        DATETIME;
DECLARE l_ot_usec      BIGINT;
DECLARE l_lat          DOUBLE;
DECLARE l_lon          DOUBLE;
DECLARE l_z            DOUBLE;
DECLARE l_nph          INT;
DECLARE l_nphS         INT;
DECLARE l_nphtot       INT;
DECLARE l_nPfm         INT;
DECLARE l_gap          INT;
DECLARE l_dmin         INT;
DECLARE l_rms          DOUBLE;
DECLARE l_e0az         INT;
DECLARE l_e0dp         INT;
DECLARE l_e0           float;
DECLARE l_e1az         INT;
DECLARE l_e1dp         INT;
DECLARE l_e1           DOUBLE;
DECLARE l_e2           DOUBLE;
DECLARE l_erh          DOUBLE;
DECLARE l_erz          DOUBLE;
DECLARE l_Md           DOUBLE;
DECLARE l_reg          VARCHAR(4);
DECLARE l_labelpref    CHAR;
DECLARE l_Mpref        DOUBLE;
DECLARE l_wtpref       DOUBLE;
DECLARE l_mdtype       CHAR;
DECLARE l_mdmad        DOUBLE;
DECLARE l_mdwt         DOUBLE;
DECLARE l_version      BIGINT;
DECLARE l_quality      CHAR(2);

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

IF TRIM(in_qid)='' THEN 
        SELECT 'ERROR: quake id can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_qid=in_qid;

IF TRIM(in_ot)='' THEN 
        SELECT 'ERROR: origin time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;

SET l_ot_dt=SUBSTR(in_ot, 1, 19);
SELECT MICROSECOND(in_ot) INTO l_ot_usec ;

SET l_lat=in_lat;
SET l_lon=in_lon;
SET l_z=in_z;
SET l_nph=in_nph;
SET l_nphS=in_nphS;
SET l_nphtot=in_nphtot;
SET l_nPfm=in_nPfm;
SET l_gap=in_gap;
SET l_dmin=in_dmin;
SET l_rms=in_rms;
SET l_e0az=in_e0az;
SET l_e0dp=in_e0dp;
SET l_e0=in_e0;
SET l_e1az=in_e1az;
SET l_e1dp=in_e1dp;
SET l_e1=in_e1;
SET l_e2=in_e2;
SET l_erh=in_erh;
SET l_erz=in_erz;
SET l_Md=in_Md;
SET l_reg=in_reg;
SET l_labelpref=in_labelpref;
SET l_Mpref=in_Mpref;
SET l_wtpref=in_wtpref;
SET l_mdtype=in_mdtype;
SET l_mdmad=in_mdmad;
SET l_mdwt=in_mdwt;
SET l_version=in_version;
SET l_quality=in_quality;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qid, l_sqkseq);

INSERT INTO `ew_arc_summary`( fk_module,fk_sqkseq,ot_dt,ot_usec,lat,lon,z,nph,nphS,nphtot,nPfm,
                                    gap,dmin,rms,e0az,e0dp,e0,e1az,e1dp,e1,e2,erh,erz,
                                    Md,reg,labelpref,Mpref,wtpref,mdtype,mdmad,mdwt,version,quality)
VALUES(l_fk_module,l_sqkseq,l_ot_dt,l_ot_usec,l_lat,l_lon,l_z,l_nph,l_nphS,l_nphtot,l_nPfm,
       l_gap,l_dmin,l_rms,l_e0az,l_e0dp,l_e0,l_e1az,l_e1dp,l_e1,l_e2,l_erh,l_erz,
       l_Md,l_reg,l_labelpref,l_Mpref,l_wtpref,l_mdtype,l_mdmad,l_mdwt,l_version,l_quality);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

