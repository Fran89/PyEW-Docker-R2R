
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_arc_phase`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_arc_phase`(
in_ewname      VARCHAR(80)     /*Host running the EW Module*/,
in_modname     VARCHAR(32)     /*Module name*/,
in_qid         BIGINT      /*event id from binder*/,    
in_site        VARCHAR(5)  /*site code, null terminated*/,
in_net         VARCHAR(3)  /*seismic network code, null terminated*/,
in_comp        VARCHAR(4)  /*component code, null terminated*/,
in_loc         VARCHAR(2)  /*location code, null terminated*/,
in_Plabel      CHAR        /*P phase label*/,
in_Slabel      CHAR        /*S phase label*/,
in_Ponset      CHAR        /*P phase onset*/,
in_Sonset      CHAR        /*S phase onset*/,
in_Pat         VARCHAR(30) /*P-arrival-time - datetime part*/,
in_Sat         VARCHAR(30) /*S-arrival-time - datetime part*/,
in_Pres        DOUBLE      /*P travel time residual*/,
in_Sres        DOUBLE      /*S travel time residual*/,
in_Pqual       INT         /*Assigned P weight code*/,
in_Squal       INT         /*Assigned S weight code*/,
in_codalen     INT         /*Coda duration time*/,
in_codawt      INT         /*Coda weight*/,
in_Pfm         CHAR        /*P first motion*/,
in_Sfm         CHAR        /*S first motion*/,
in_datasrc     CHAR        /*Data source code.*/,
in_Md          DOUBLE      /*Station duration magnitude*/,
in_azm         INT         /*azimuth*/,
in_takeoff     INT         /*emergence angle at source*/,
in_dist        DOUBLE      /*epicentral distance (km)*/,
in_Pwt         DOUBLE      /*P weight actually used.*/,
in_Swt         DOUBLE      /*S weight actually used.*/,
in_pamp        INT         /*peak P-wave half amplitude*/,
in_codalenObs  INT         /*Coda duration time (Measured)*/,
in_ccntr_0     INT         /*Window center from P time*/,
in_ccntr_1     INT         /*Window center from P time*/,
in_ccntr_2     INT         /*Window center from P time*/,
in_ccntr_3     INT         /*Window center from P time*/,
in_ccntr_4     INT         /*Window center from P time*/,
in_ccntr_5     INT         /*Window center from P time*/,
in_caav_0      INT         /*Average Amplitude for ccntr[x]*/,
in_caav_1      INT         /*Average Amplitude for ccntr[x]*/,
in_caav_2      INT         /*Average Amplitude for ccntr[x]*/,
in_caav_3      INT         /*Average Amplitude for ccntr[x]*/,
in_caav_4      INT         /*Average Amplitude for ccntr[x]*/,
in_caav_5      INT         /*Average Amplitude for ccntr[x]*/,
in_version     BIGINT      /*version number of the origin'*/
)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname     VARCHAR(32);
DECLARE l_qid         BIGINT;
DECLARE l_site        VARCHAR(5);
DECLARE l_net         VARCHAR(3);
DECLARE l_comp        VARCHAR(4);
DECLARE l_loc         VARCHAR(2);
DECLARE l_Plabel      CHAR;
DECLARE l_Slabel      CHAR;
DECLARE l_Ponset      CHAR;
DECLARE l_Sonset      CHAR;
DECLARE l_Pat_dt      DATETIME;
DECLARE l_Pat_usec    BIGINT;
DECLARE l_Sat_dt      DATETIME;
DECLARE l_Sat_usec    BIGINT;
DECLARE l_Pres        DOUBLE;
DECLARE l_Sres        DOUBLE;
DECLARE l_Pqual       INT;
DECLARE l_Squal       INT;
DECLARE l_codalen     INT;
DECLARE l_codawt      INT;
DECLARE l_Pfm         CHAR;
DECLARE l_Sfm         CHAR;
DECLARE l_datasrc     CHAR;
DECLARE l_Md          DOUBLE;
DECLARE l_azm         INT;
DECLARE l_takeoff     INT;
DECLARE l_dist        DOUBLE;
DECLARE l_Pwt         DOUBLE;
DECLARE l_Swt         DOUBLE;
DECLARE l_pamp        INT;
DECLARE l_codalenObs  INT;
DECLARE l_ccntr_0     INT;
DECLARE l_ccntr_1     INT;
DECLARE l_ccntr_2     INT;
DECLARE l_ccntr_3     INT;
DECLARE l_ccntr_4     INT;
DECLARE l_ccntr_5     INT;
DECLARE l_caav_0      INT;
DECLARE l_caav_1      INT;
DECLARE l_caav_2      INT;
DECLARE l_caav_3      INT;
DECLARE l_caav_4      INT;
DECLARE l_caav_5      INT;
DECLARE l_version     BIGINT;

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
SET l_qid=in_qid;


IF TRIM(in_Pat)='' THEN 
        SELECT 'ERROR: origin time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;

SET l_Pat_dt=SUBSTR(in_Pat, 1, 19);
SELECT MICROSECOND(in_Pat) INTO l_Pat_usec ;


IF TRIM(in_Sat)='' THEN 
        SELECT 'ERROR: origin time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;

SET l_Sat_dt=SUBSTR(in_Sat, 1, 19);
SELECT MICROSECOND(in_Sat) INTO l_Sat_usec ;


SET l_qid        = in_qid;
SET l_site       = in_site;
SET l_net        = in_net;
SET l_comp       = in_comp;
SET l_loc        = in_loc;
SET l_Plabel     = in_Plabel;
SET l_Slabel     = in_Slabel;
SET l_Ponset     = in_Ponset;
SET l_Sonset     = in_Sonset;
SET l_Pres       = in_Pres;
SET l_Sres       = in_Sres;
SET l_Pqual      = in_Pqual;
SET l_Squal      = in_Squal;
SET l_codalen    = in_codalen;
SET l_codawt     = in_codawt;
SET l_Pfm        = in_Pfm;
SET l_Sfm        = in_Sfm;
SET l_datasrc    = in_datasrc;
SET l_Md         = in_Md;
SET l_azm        = in_azm;
SET l_takeoff    = in_takeoff;
SET l_dist       = in_dist;
SET l_Pwt        = in_Pwt;
SET l_Swt        = in_Swt;
SET l_pamp       = in_pamp;
SET l_codalenObs = in_codalenObs;
SET l_ccntr_0    = in_ccntr_0;
SET l_ccntr_1    = in_ccntr_1;
SET l_ccntr_2    = in_ccntr_2;
SET l_ccntr_3    = in_ccntr_3;
SET l_ccntr_4    = in_ccntr_4;
SET l_ccntr_5    = in_ccntr_5;
SET l_caav_0     = in_caav_0;
SET l_caav_1     = in_caav_1;
SET l_caav_2     = in_caav_2;
SET l_caav_3     = in_caav_3;
SET l_caav_4     = in_caav_4;
SET l_caav_5     = in_caav_5;
SET l_version    = in_version;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qid, l_sqkseq);
CALL `sp_get_ew_scnl_out`(l_site, l_comp, l_net, l_loc, l_fk_scnl);

INSERT INTO `ew_arc_phase` (fk_module,fk_sqkseq,fk_scnl,Plabel,Slabel,Ponset,Sonset,Pat_dt,Pat_usec,Sat_dt,Sat_usec,Pres,Sres,Pqual,Squal,
                                  codalen,codawt,Pfm,Sfm,datasrc,Md,azm,takeoff,dist,Pwt,Swt,pamp,codalenObs,
                                  ccntr_0,ccntr_1,ccntr_2,ccntr_3,ccntr_4,ccntr_5,caav_0,caav_1,caav_2,caav_3,caav_4,caav_5,version)
    VALUES ( l_fk_module,l_sqkseq,l_fk_scnl,l_Plabel,l_Slabel,l_Ponset,l_Sonset,l_Pat_dt,l_Pat_usec,l_Sat_dt,l_Sat_usec,l_Pres,l_Sres,l_Pqual,l_Squal,
	l_codalen,l_codawt,l_Pfm,l_Sfm,l_datasrc,l_Md,l_azm,l_takeoff,l_dist,l_Pwt,l_Swt,l_pamp,l_codalenObs,
	l_ccntr_0,l_ccntr_1,l_ccntr_2,l_ccntr_3,l_ccntr_4,l_ccntr_5,l_caav_0,l_caav_1,l_caav_2,l_caav_3,l_caav_4,l_caav_5,l_version);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

