
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_eb_picktwc`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_eb_picktwc`(
in_ewname       VARCHAR(80)  /* Host running the EW Module*/,
in_modname      VARCHAR(32)  /* Module name */,

in_site           VARCHAR(5)  /*site code, null terminated*/,
in_net            VARCHAR(3)  /*seismic network code, null terminated*/,
in_comp           VARCHAR(4)  /*component code, null terminated*/,
in_loc            VARCHAR(2)  /*location code, null terminated*/,

in_lPickIndex     BIGINT       /* Pick index; 0-10000->pick_wcatwc, 10000-20000
			     ->develo; 20000-30000->hypo_display source*/,
in_iUseMe         BIGINT       /* 2->This pick will not be removed by FindBadPs,
			     1->Use this P in location; 0->Don t (auto KO)
			     -1->Don t (manually knocked out)*/,
in_dPTime         VARCHAR(30) /* P-time - datetime string (microseconds) */,
in_cFirstMotion   CHAR(1)     /* ?=unknown, U=up, D=down*/,
in_szPhase        VARCHAR(10) /* Pick Phase*/,
in_dMbAmpGM       DOUBLE      /* Mb amplitude (ground motion in nm)*/,
in_dMbPer         DOUBLE      /* Mb Per data, per of lMbAmp (sec)*/,
in_dMbTime        VARCHAR(30) /* time at end of Mb T/A - datetime string (microseconds) */,
in_dMlAmpGM       DOUBLE      /* Ml amplitude (ground motion in nm)*/,
in_dMlPer         DOUBLE      /* Ml Per data, per of lMlAmp (sec)*/,
in_dMlTime        VARCHAR(30) /* time at end of Ml T/A - datetime string (microseconds) */,
in_dMSAmpGM       DOUBLE      /* MS amplitude (ground motion in um)*/,
in_dMSPer         DOUBLE      /* MS Per data, per of lMSAmp (sec)*/,
in_dMSTime        VARCHAR(30) /* time at end of MS T/A - datetime string (microseconds) */,
in_dMwpIntDisp    DOUBLE      /* Maximum integrated disp. peak-to-peak amp*/,
in_dMwpTime       DOUBLE      /* Mwp window time in seconds*/,
in_szHypoID       VARCHAR(33) /* If pick made in hypo_display, this is the,
			     associated hypocenter ID*/,
in_dPStrength     DOUBLE      /* Ratio of P motion to background*/,
in_dFreq          DOUBLE      /* Dominant frequency of this P-pick*/

)
uscita: BEGIN
DECLARE l_ewname           VARCHAR(80);
DECLARE l_modname          VARCHAR(32);
DECLARE l_fk_module        BIGINT;
DECLARE l_site             VARCHAR(5);
DECLARE l_net              VARCHAR(3);
DECLARE l_comp             VARCHAR(4);
DECLARE l_loc              VARCHAR(2);
DECLARE l_fk_scnl          BIGINT;

DECLARE l_lPickIndex       BIGINT;
DECLARE l_iUseMe           BIGINT;
DECLARE l_dPTime_dt        DATETIME;
DECLARE l_dPTime_usec      INT;
DECLARE l_cFirstMotion     CHAR(1);
DECLARE l_szPhase          VARCHAR(10);
DECLARE l_dMbAmpGM         DOUBLE;
DECLARE l_dMbPer           DOUBLE;
DECLARE l_dMbTime_dt       DATETIME;
DECLARE l_dMbTime_usec     INT;
DECLARE l_dMlAmpGM         DOUBLE;
DECLARE l_dMlPer           DOUBLE;
DECLARE l_dMlTime_dt       DATETIME;
DECLARE l_dMlTime_usec     INT;
DECLARE l_dMSAmpGM         DOUBLE;
DECLARE l_dMSPer           DOUBLE;
DECLARE l_dMSTime_dt       DATETIME;
DECLARE l_dMSTime_usec     INT;
DECLARE l_dMwpIntDisp      DOUBLE;
DECLARE l_dMwpTime         DOUBLE;
DECLARE l_szHypoID         VARCHAR(33);
DECLARE l_dPStrength       DOUBLE;
DECLARE l_dFreq            DOUBLE;

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

SET l_site       = in_site;
SET l_net        = in_net;
SET l_comp       = in_comp;
SET l_loc        = in_loc;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);
CALL `sp_get_ew_scnl_out`(l_site, l_comp, l_net, l_loc, l_fk_scnl);

SET l_lPickIndex=in_lPickIndex;
SET l_iUseMe=in_iUseMe;
SET l_dPTime_dt=in_dPTime;
SELECT MICROSECOND(in_dPTime) INTO l_dPTime_usec;
SET l_cFirstMotion=in_cFirstMotion;
SET l_szPhase=in_szPhase;
SET l_dMbAmpGM=in_dMbAmpGM;
SET l_dMbPer=in_dMbPer;
SET l_dMbTime_dt=in_dMbTime;
SELECT MICROSECOND(in_dMbTime) INTO l_dMbTime_usec;
SET l_dMlAmpGM=in_dMlAmpGM;
SET l_dMlPer=in_dMlPer;
SET l_dMlTime_dt=in_dMlTime;
SELECT MICROSECOND(in_dMlTime) INTO l_dMlTime_usec;
SET l_dMSAmpGM=in_dMSAmpGM;
SET l_dMSPer=in_dMSPer;
SET l_dMSTime_dt=in_dMSTime;
SELECT MICROSECOND(in_dMSTime) INTO l_dMSTime_usec;
SET l_dMwpIntDisp=in_dMwpIntDisp;
SET l_dMwpTime=in_dMwpTime;
SET l_szHypoID=in_szHypoID;
SET l_dPStrength=in_dPStrength;
SET l_dFreq=in_dFreq;

INSERT INTO `eb_picktwc`(
  fk_module, fk_scnl, lPickIndex, iUseMe, dPTime_dt, dPTime_usec, cFirstMotion, szPhase,
  dMbAmpGM, dMbPer, dMbTime_dt, dMbTime_usec, dMlAmpGM, dMlPer, dMlTime_dt, dMlTime_usec,
  dMSAmpGM, dMSPer, dMSTime_dt, dMSTime_usec,
  dMwpIntDisp, dMwpTime,
  szHypoID, dPStrength, dFreq)
VALUES(
  l_fk_module, l_fk_scnl, l_lPickIndex, l_iUseMe, l_dPTime_dt, l_dPTime_usec, l_cFirstMotion, l_szPhase,
  l_dMbAmpGM, l_dMbPer, l_dMbTime_dt, l_dMbTime_usec, l_dMlAmpGM, l_dMlPer, l_dMlTime_dt, l_dMlTime_usec,
  l_dMSAmpGM, l_dMSPer, l_dMSTime_dt, l_dMSTime_usec,
  l_dMwpIntDisp, l_dMwpTime,
  l_szHypoID, l_dPStrength, l_dFreq);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

