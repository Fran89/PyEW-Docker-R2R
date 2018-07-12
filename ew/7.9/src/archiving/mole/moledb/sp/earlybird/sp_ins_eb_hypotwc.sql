
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_eb_hypotwc`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_eb_hypotwc`(
in_ewname       VARCHAR(80)  /* Host running the EW Module*/,
in_modname      VARCHAR(32)  /* Module name */,

in_dLat             DOUBLE   /* Geocentric epicentral latitude */,
in_dLon             DOUBLE   /* Geocentric epicentral longitude */,

in_dOriginTime      VARCHAR(30) /* Origin time - datetime string (microseconds) */,

in_dDepth           DOUBLE   /* Hypocenter depth (in km) */,
in_dPreferredMag    DOUBLE   /* Magnitude to use for this event */,
in_szPMagType      CHAR(3)  /* Magnitude type of dPreferredMag (l, b, etc.) */,

in_iNumPMags        BIGINT   /* Number of stations used in dPreferredMag */,
in_szQuakeID        CHAR(33) /* Event ID */,
in_iVersion         BIGINT   /* Version of location (1, 2, ...) */,
in_iNumPs           BIGINT   /* Number of P s used in quake solution */,
in_dAvgRes          DOUBLE   /* Residual average */,
in_iAzm             BIGINT   /* Azimuthal coverage in degrees */,
in_iGoodSoln        BIGINT   /* 0-bad, 1-soso, 2-good, 3-very good */,
in_dMbAvg           DOUBLE   /* Modified average Mb */,
in_iNumMb           BIGINT   /* Number of Mb s in modified average */,
in_dMlAvg           DOUBLE   /* Modified average Ml */,
in_iNumMl           BIGINT   /* Number of Ml s in modified average */,
in_dMSAvg           DOUBLE   /* Modified average MS */,
in_iNumMS           BIGINT   /* Number of MS s in modified average */,
in_dMwpAvg          DOUBLE   /* Modified average Mwp */,
in_iNumMwp          BIGINT   /* Number of Mwp s in modified average */,
in_dMwAvg           DOUBLE   /* Modified average Mw */,
in_iNumMw           BIGINT   /* Number of Mw s in modified average */,
in_dTheta           DOUBLE   /* Theta (energy/moment) ratio */,
in_iNumTheta        BIGINT   /* Number of stations used in theta computation */,
in_iMagOnly         BIGINT   /* 0->New location, 1->updated magnitudes only */

)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname      VARCHAR(32);
DECLARE l_fk_module               BIGINT;

DECLARE l_dLat             DOUBLE;
DECLARE l_dLon             DOUBLE;

DECLARE l_dOriginTime_dt   DATETIME;
DECLARE l_dOriginTime_usec int;

DECLARE l_dDepth           DOUBLE;
DECLARE l_dPreferredMag    DOUBLE;
DECLARE l_szPMagType      CHAR(3);

DECLARE l_iNumPMags        BIGINT;
DECLARE l_szQuakeID        CHAR(33);
DECLARE l_iVersion         BIGINT;
DECLARE l_iNumPs           BIGINT;
DECLARE l_dAvgRes          DOUBLE;
DECLARE l_iAzm             BIGINT;
DECLARE l_iGoodSoln        BIGINT;
DECLARE l_dMbAvg           DOUBLE;
DECLARE l_iNumMb           BIGINT;
DECLARE l_dMlAvg           DOUBLE;
DECLARE l_iNumMl           BIGINT;
DECLARE l_dMSAvg           DOUBLE;
DECLARE l_iNumMS           BIGINT;
DECLARE l_dMwpAvg          DOUBLE;
DECLARE l_iNumMwp          BIGINT;
DECLARE l_dMwAvg           DOUBLE;
DECLARE l_iNumMw           BIGINT;
DECLARE l_dTheta           DOUBLE;
DECLARE l_iNumTheta        BIGINT;
DECLARE l_iMagOnly         BIGINT;

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

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);

SET l_dLat=in_dLat;
SET l_dLon=in_dLon;

SET l_dOriginTime_dt=in_dOriginTime;
SELECT MICROSECOND(in_dOriginTime) INTO l_dOriginTime_usec;

SET l_dDepth=in_dDepth;
SET l_dPreferredMag=in_dPreferredMag;
SET l_szPMagType=in_szPMagType;

SET l_iNumPMags=in_iNumPMags;
SET l_szQuakeID=in_szQuakeID;
SET l_iVersion=in_iVersion;
SET l_iNumPs=in_iNumPs;
SET l_dAvgRes=in_dAvgRes;
SET l_iAzm=in_iAzm;
SET l_iGoodSoln=in_iGoodSoln;
SET l_dMbAvg=in_dMbAvg;
SET l_iNumMb=in_iNumMb;
SET l_dMlAvg=in_dMlAvg;
SET l_iNumMl=in_iNumMl;
SET l_dMSAvg=in_dMSAvg;
SET l_iNumMS=in_iNumMS;
SET l_dMwpAvg=in_dMwpAvg;
SET l_iNumMwp=in_iNumMwp;
SET l_dMwAvg=in_dMwAvg;
SET l_iNumMw=in_iNumMw;
SET l_dTheta=in_dTheta;
SET l_iNumTheta=in_iNumTheta;
SET l_iMagOnly=in_iMagOnly;

INSERT INTO `eb_hypotwc`( fk_module,
  dLat, dLon,
  dOriginTime_dt, dOriginTime_usec,
  dDepth, dPreferredMag, szPMagType,
  iNumPMags, szQuakeID, iVersion,
  iNumPs, dAvgRes, iAzm, iGoodSoln,
  dMbAvg, iNumMb, dMlAvg, iNumMl,
  dMSAvg, iNumMS, dMwpAvg, iNumMwp,
  dMwAvg, iNumMw, dTheta, iNumTheta,
  iMagOnly)
VALUES (
  l_fk_module,
  l_dLat, l_dLon,
  l_dOriginTime_dt, l_dOriginTime_usec,
  l_dDepth, l_dPreferredMag, l_szPMagType,
  l_iNumPMags, l_szQuakeID, l_iVersion,
  l_iNumPs, l_dAvgRes, l_iAzm, l_iGoodSoln,
  l_dMbAvg, l_iNumMb, l_dMlAvg, l_iNumMl,
  l_dMSAvg, l_iNumMS, l_dMwpAvg, l_iNumMwp,
  l_dMwAvg, l_iNumMw, l_dTheta, l_iNumTheta,
  l_iMagOnly);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

