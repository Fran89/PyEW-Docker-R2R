-- Created Mon Apr 22 19:29:19 CEST 2013


-- PROCEDURES
DROP PROCEDURE IF EXISTS sp_ew_arcsummary;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ew_arcsummary`(IN event_id bigint(20), IN loc_version bigint(20), IN ew_instance varchar(80))
BEGIN
	SELECT  ew_arc_summary.* 
	FROM ew_arc_summary 
		join ew_sqkseq on (ew_arc_summary.fk_sqkseq = ew_sqkseq.id) 
	WHERE ew_sqkseq.fk_instance = (SELECT id 
					FROM ew_instance 
					WHERE ewname =ew_instance) 
		AND  ew_sqkseq.qkseq = event_id
		AND ew_arc_summary.version = loc_version;
END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ew_event;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ew_event`(
in_start_time       VARCHAR(50),
in_end_time         VARCHAR(50)
)
uscita: BEGIN
    
    IF fn_isdate(in_start_time) THEN
        SELECT'ERROR: start_time not datatime format yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;
    IF fn_isdate(in_end_time) THEN
        SELECT'ERROR: end_time not datatime format  yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;

    SELECT CAST(CONCAT(`e`.`fk_sqkseq`, '_', `e`.`version`) AS CHAR(50)) AS `sqkid_version`,
    `e`.`fk_sqkseq` AS `sqkid`,
    `e`.`qkseq` AS `qkseq`,
    `e`.`ewname` AS `instance`,
    CAST(CONCAT(`e`.`ot_dt`, '.', LEFT(`e`.`ot_usec`,4)) AS CHAR(50)) AS `ot_time`,
    `e`.`mag` AS `mag`,
    `e`.`mag_type` AS `mag_type`,
    `e`.`mag_error` AS `mag_error`,
    `e`.`arc_quality` AS `quality`,
    ROUND(`e`.`lat`,3) AS `lat`,
    ROUND(`e`.`lon`,3) AS `lon`,
    ROUND(`e`.`z`,3) AS `depth`,
    `e`.`erh` AS `error_h`,
    `e`.`erz` AS `error_z`,
    `e`.`nphtot` AS `nphtot`,
    `e`.`gap` AS `gap`,
    `e`.`dmin` AS `dmin`,
    `e`.`rms` AS `rms`,
    `e`.`ewname` AS `ewname`,
    `e`.`mag_ewmodid` AS `modmag`,
    `e`.`region` AS `region_name`,
    `e`.`version` AS `version`,
    `e`.`modified` AS `modified`,
    `s`.`net`,
    `s`.`sta`,
    `p`.`dist`,
    `p`.`Pwt`
    FROM `ew_events_summary` e
    JOIN `ew_arc_phase` p ON `p`.`fk_sqkseq` = `e`.`fk_sqkseq` AND `p`.`version` = `e`.`version`
    JOIN `ew_scnl` s ON `p`.`fk_scnl` = `s`.`id`
    WHERE `e`.`ot_dt` >= in_start_time 
        AND `e`.`ot_dt` <= in_end_time
    ORDER BY `e`.`fk_sqkseq` DESC, `e`.`version` DESC, `p`.`dist` ASC;
END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ew_eventphases_list;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ew_eventphases_list`(IN event_id bigint(20), IN loc_version bigint(20), IN ew_instance varchar(80))
BEGIN
SELECT ew_instance.ewname, ew_scnl.sta, ew_scnl.cha, ew_scnl.net, ew_scnl.loc,
ROUND(ew_arc_phase.dist, 2) as dist, 
IF(ew_arc_phase.Ponset='P', ew_arc_phase.Pres, '')  AS Pres, 
IF(ew_arc_phase.Ponset='P', ew_arc_phase.Pwt, '')  AS Pwt, 
IF(ew_arc_phase.Sonset='S', ew_arc_phase.Sres, '') AS Sres, 
IF(ew_arc_phase.Sonset='S', ew_arc_phase.Swt, '') AS Swt, 
IF(ew_arc_phase.Ponset='P', CONCAT(ew_arc_phase.Ponset, ew_arc_phase.Plabel), '') AS PLab, 
IF(ew_arc_phase.Ponset='P', CONCAT(CAST(ew_arc_phase.Pat_dt AS CHAR),SUBSTR(ROUND(ew_arc_phase.Pat_usec/1000000.0, 2), 2) ), '') AS 'P_phase_time',
IF(ew_arc_phase.Ponset='P', ew_arc_phase.Pqual, '') AS Pqual,
IF(ew_arc_phase.Sonset='S', CONCAT(ew_arc_phase.Sonset, ew_arc_phase.Slabel), '') AS SLab,
IF(ew_arc_phase.Sonset='S', CONCAT(CAST(ew_arc_phase.Sat_dt AS CHAR),  SUBSTR(ROUND(ew_arc_phase.Sat_usec/1000000.0, 2), 2) ), '') AS 'S_phase_time',
IF(ew_arc_phase.Sonset='S', ew_arc_phase.Squal, '') AS Squal,
ew_arc_phase.codalen, ew_arc_phase.codawt, ew_arc_phase.Pfm, ew_arc_phase.Sfm, ew_arc_phase.datasrc, ew_arc_phase.Md, ew_arc_phase.azm, ew_arc_phase.takeoff, ew_arc_phase.id, ew_sqkseq.qkseq, ew_arc_phase.version, ew_module.modname, ew_arc_phase.modified 
FROM ew_arc_phase 
JOIN ew_module ON (ew_arc_phase.fk_module=ew_module.id) 
JOIN ew_instance ON (ew_module.fk_instance=ew_instance.id) 
JOIN ew_sqkseq ON (ew_arc_phase.fk_sqkseq=ew_sqkseq.id) 
JOIN ew_scnl ON (ew_scnl.id=ew_arc_phase.fk_scnl) 
WHERE ew_sqkseq.qkseq = event_id AND ew_arc_phase.version = loc_version AND ew_instance.ewname= ew_instance;
END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ew_event_instance;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ew_event_instance`(
in_instance         VARCHAR(19),
in_start_time       VARCHAR(19),
in_end_time         VARCHAR(19)
)
uscita: BEGIN
    DECLARE l_id_instance        BIGINT DEFAULT 0;
    
    SELECT id INTO l_id_instance
    FROM `ew_instance` 
    WHERE ewname = in_instance;
    IF l_id_instance = 0 THEN
        SELECT'ERROR: wrong istance name';
        LEAVE uscita;
    END IF;
    
    IF fn_isdate(in_start_time) THEN
        SELECT'ERROR: start_time not datatime format yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;
    IF fn_isdate(in_end_time) THEN
        SELECT'ERROR: end_time not datatime format  yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;
    SELECT CAST(CONCAT(`e`.`fk_sqkseq`, '_', `e`.`version`) AS CHAR(50)) AS `sqkid_version`,
    `e`.`fk_sqkseq` AS `sqkid`,
    `e`.`qkseq` AS `qkseq`,
    CAST(CONCAT(`e`.`ot_dt`, '.', LEFT(`e`.`ot_usec`,4)) AS CHAR(50)) AS `ot_time`,
    `e`.`mag` AS `mag`,
    `e`.`mag_type` AS `mag_type`,
    `e`.`mag_error` AS `mag_error`,
    `e`.`arc_quality` AS `quality`,
    ROUND(`e`.`lat`,3) AS `lat`,
    ROUND(`e`.`lon`,3) AS `lon`,
    ROUND(`e`.`z`,3) AS `depth`,
    `e`.`erh` AS `error_h`,
    `e`.`erz` AS `error_z`,
    `e`.`nphtot` AS `nphtot`,
    `e`.`gap` AS `gap`,
    `e`.`dmin` AS `dmin`,
    `e`.`rms` AS `rms`,
    `e`.`ewname` AS `ewname`,
    `e`.`mag_ewmodid` AS `modmag`,
    `e`.`region` AS `region_name`,
    `e`.`version` AS `version`,
    `e`.`modified` AS `modified`,
    `s`.`net`,
    `s`.`sta`,
    `p`.`dist`,
    `p`.`Pwt`
    FROM `ew_events_summary` e
    JOIN `ew_arc_phase` p ON `p`.`fk_sqkseq` = `e`.`fk_sqkseq` AND `p`.`version` = `e`.`version`
    JOIN `ew_scnl` s ON `p`.`fk_scnl` = `s`.`id`
    WHERE `e`.`ot_dt` >= in_start_time 
        AND `e`.`ot_dt` <= in_end_time
        AND `e`.`ewname` = in_instance
    ORDER BY `e`.`fk_sqkseq` DESC, `e`.`version` DESC; 
END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ew_event_list;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ew_event_list`(
in_start_time  VARCHAR(19),
in_end_time    VARCHAR(19)
)
uscita: BEGIN
    IF fn_isdate(in_start_time) THEN
        SELECT'ERROR: start_time not datatime format yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;
    IF fn_isdate(in_end_time) THEN
        SELECT'ERROR: end_time not datatime format  yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;
    SELECT `ew_arc_summary`.`id` AS `db_id`,
    `ew_sqkseq`.`qkseq` AS `ew_id`,
     CONCAT(`ew_arc_summary`.`ot_dt`, '.', LEFT(`ew_arc_summary`.`ot_usec`,4)) AS `ot_time`,
    `ew_magnitude_summary`.`mag` AS `mag`,
    `ew_magnitude_summary`.`szmagtype` AS `mag_type`,
    `ew_magnitude_summary`.`error` AS `mag_error`,
    `ew_arc_summary`.`quality` AS `quality`,
    ROUND(`ew_arc_summary`.`lat`,3) AS `lat`,
    ROUND(`ew_arc_summary`.`lon`,3) AS `lon`,
    ROUND(`ew_arc_summary`.`z`,3) AS `depth`,
    `ew_arc_summary`.`erh` AS `error_h`,
    `ew_arc_summary`.`erz` AS `error_z`,
    `ew_arc_summary`.`nphtot` AS `nphtot`,
    `ew_arc_summary`.`gap` AS `gap`,
    `ew_arc_summary`.`dmin` AS `dmin`,
    `ew_arc_summary`.`rms` AS `rms`,
    `ew_instance`.`ewname` AS `ewname`,
    `ew_instance`.`hostname` AS `hostname`,
    `modmag`.`modname` AS `modmag`,


    `ew_arc_summary`.`version` AS `version`,
    `ew_arc_summary`.`modified` AS `modified`,
    GROUP_CONCAT(DISTINCT CONCAT(`ew_scnl`.`sta`, '_', `ew_arc_phase`.`dist`, '_', `ew_arc_phase`.`Pwt`) SEPARATOR ';') AS `station_list`
    FROM `ew_arc_summary`
    JOIN `ew_module` ON `ew_arc_summary`.`fk_module` = `ew_module`.`id`
    JOIN `ew_instance` ON `ew_module`.`fk_instance` = `ew_instance`.`id`
    JOIN `ew_sqkseq` ON `ew_arc_summary`.`fk_sqkseq` = `ew_sqkseq`.`id`
    LEFT JOIN `ew_magnitude_summary` ON `ew_arc_summary`.`fk_sqkseq` = `ew_magnitude_summary`.`fk_sqkseq`
        AND `ew_arc_summary`.`version` = `ew_magnitude_summary`.`version`
    LEFT JOIN `ew_module` `modmag` ON IF(ISNULL(`ew_magnitude_summary`.`mag`),0,`ew_magnitude_summary`.`fk_module` = `modmag`.`id`)
    LEFT JOIN `ew_arc_phase` ON `ew_arc_phase`.`fk_sqkseq` = `ew_arc_summary`.`fk_sqkseq`
    JOIN `ew_scnl` ON `ew_arc_phase`.`fk_scnl` = `ew_scnl`.`id`
    WHERE `ew_arc_summary`.`ot_dt` >= in_start_time 
        AND `ew_arc_summary`.`ot_dt` <= in_end_time
    GROUP BY `ew_instance`.`ewname`,`ew_module`.`modname`,`ew_sqkseq`.`qkseq`,`ew_arc_summary`.`version`
    ORDER BY `ew_arc_summary`.`ot_dt`,`ew_arc_summary`.`ot_usec`;
END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ew_instance;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ew_instance`(
)
BEGIN
    SELECT * FROM ew_instance;
END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ew_last_event_id;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ew_last_event_id`(
)
BEGIN
SELECT MAX(id) FROM `ew_events_summary`;

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ew_magnitudepicks;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ew_magnitudepicks`(IN event_id bigint(20), IN l_version bigint(20), IN ew_instance varchar(80))
BEGIN
SELECT ew_instance.ewname, ew_scnl.sta, ew_scnl.cha, ew_scnl.net, ew_scnl.loc,ew_sqkseq.qkseq, ew_magnitude_phase.version, ew_module.modname, ew_magnitude_phase.modified,ew_magnitude_phase.*
	FROM ew_magnitude_phase
	JOIN ew_module ON (ew_magnitude_phase.fk_module=ew_module.id )
	JOIN ew_instance ON (ew_module.fk_instance=ew_instance.id)
	JOIN ew_sqkseq ON (ew_magnitude_phase.fk_sqkseq=ew_sqkseq.id)
	JOIN ew_scnl ON (ew_scnl.id=ew_magnitude_phase.fk_scnl)
	WHERE ew_sqkseq.qkseq = event_id AND ew_magnitude_phase.version = l_version AND ew_instance.ewname= ew_instance;
END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ew_show_hosts;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ew_show_hosts`(
)
BEGIN
Select distinct hostname From `ew_instance` Where hostname is not  NULL;

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_get_ew_instance_out;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_get_ew_instance_out`(
in_ewname                VARCHAR(80)   ,
OUT l_id                 BIGINT
)
uscita: BEGIN
DECLARE l_ewname         VARCHAR(80);

SET l_id = -1;

IF TRIM(in_ewname)='' THEN 
        SELECT 'ERROR: Ew instance name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_ewname=in_ewname;

SELECT id INTO l_id FROM `ew_instance` WHERE ewname=l_ewname;

IF l_id = -1 THEN
    INSERT INTO `ew_instance`(ewname) VALUES (l_ewname);
    SELECT LAST_INSERT_ID() INTO l_id;
END IF;



END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_get_ew_module_out;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_get_ew_module_out`(
in_ewname                VARCHAR(80)   ,
in_modname               VARCHAR(32)   ,
OUT l_id                 BIGINT
)
uscita: BEGIN
DECLARE l_ewname         VARCHAR(80);
DECLARE l_modname        VARCHAR(32);
DECLARE l_ewid           BIGINT;

SET l_id = -1;

IF TRIM(in_ewname)='' THEN 
        SELECT 'ERROR: Ew instance name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_ewname=in_ewname;

IF TRIM(in_modname)='' THEN 
        SELECT 'ERROR: Ew module name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_modname=in_modname;

CALL `sp_get_ew_instance_out`(l_ewname, l_ewid);

SELECT id INTO l_id FROM `ew_module` WHERE fk_instance=l_ewid AND modname=l_modname;

IF l_id = -1 THEN
    INSERT INTO `ew_module`(fk_instance,modname) VALUES (l_ewid,l_modname);
    SELECT LAST_INSERT_ID() INTO l_id;
END IF;



END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_get_ew_scnl_out;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_get_ew_scnl_out`(
in_sta                   VARCHAR(5)   ,
in_cha                   VARCHAR(3)   ,
in_net                   VARCHAR(2)   ,
in_loc                   VARCHAR(2)   ,
OUT l_id                 BIGINT
)
uscita: BEGIN
DECLARE l_sta                   VARCHAR(5);
DECLARE l_cha                   VARCHAR(3);
DECLARE l_net                   VARCHAR(2);
DECLARE l_loc                   VARCHAR(2);

SET l_id = -1;

IF TRIM(in_sta)='' THEN 
        SELECT 'ERROR: station (site) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_sta=in_sta;

IF TRIM(in_cha)='' THEN 
        SELECT 'ERROR: channel (component) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_cha=in_cha;

IF TRIM(in_net)='' THEN 
        SELECT 'ERROR: net name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_net=in_net;

IF in_loc IS NULL OR TRIM(in_loc)='' OR TRIM(in_loc)='--' THEN 
    SET in_loc='--';
END IF;
SET l_loc=in_loc;

SELECT id INTO l_id FROM `ew_scnl` WHERE sta=l_sta AND cha=l_cha AND net=l_net AND loc=l_loc;

IF l_id = -1 THEN
    INSERT INTO `ew_scnl`(sta,cha,net,loc) VALUES (l_sta,l_cha,l_net,l_loc);
    SELECT LAST_INSERT_ID() INTO l_id;
END IF;



END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_get_ew_spkseq_out;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_get_ew_spkseq_out`(
in_ewname                VARCHAR(80)   ,
in_pkseq                 BIGINT   ,
OUT l_id                 BIGINT
)
uscita: BEGIN
DECLARE l_ewname         VARCHAR(80);
DECLARE l_pkseq          BIGINT;
DECLARE l_ewid           BIGINT;

SET l_id = -1;

IF TRIM(in_ewname)='' THEN 
        SELECT 'ERROR: Ew instance name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_ewname=in_ewname;

IF TRIM(in_pkseq)='' THEN 
        SELECT 'ERROR: Pick sequence number from picker can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_pkseq=in_pkseq;

CALL `sp_get_ew_instance_out`(l_ewname, l_ewid);

SELECT id INTO l_id FROM `ew_spkseq` WHERE fk_instance=l_ewid AND pkseq=l_pkseq;

IF l_id = -1 THEN
    INSERT INTO `ew_spkseq`(fk_instance,pkseq) VALUES (l_ewid,l_pkseq);
    SELECT LAST_INSERT_ID() INTO l_id;
END IF;



END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_get_ew_sqkseq_out;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_get_ew_sqkseq_out`(
in_ewname                VARCHAR(80)   ,
in_qkseq                 BIGINT   ,
OUT l_id                 BIGINT
)
uscita: BEGIN
DECLARE l_ewname         VARCHAR(80);
DECLARE l_qkseq          BIGINT;
DECLARE l_ewid           BIGINT;

SET l_id = -1;

IF TRIM(in_ewname)='' THEN 
        SELECT 'ERROR: Ew instance name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_ewname=in_ewname;

IF TRIM(in_qkseq)='' THEN 
        SELECT 'ERROR: Quake sequence number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_qkseq=in_qkseq;

CALL `sp_get_ew_instance_out`(l_ewname, l_ewid);

SELECT id INTO l_id FROM `ew_sqkseq` WHERE fk_instance=l_ewid AND qkseq=l_qkseq;

IF l_id = -1 THEN
    INSERT INTO `ew_sqkseq`(fk_instance,qkseq) VALUES (l_ewid,l_qkseq);
    SELECT LAST_INSERT_ID() INTO l_id;
END IF;



END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_arc_phase;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_arc_phase`(
in_ewname      VARCHAR(80)      ,
in_modname     VARCHAR(32)      ,
in_qid         BIGINT       ,    
in_site        VARCHAR(5)   ,
in_net         VARCHAR(3)   ,
in_comp        VARCHAR(4)   ,
in_loc         VARCHAR(2)   ,
in_Plabel      CHAR         ,
in_Slabel      CHAR         ,
in_Ponset      CHAR         ,
in_Sonset      CHAR         ,
in_Pat         VARCHAR(30)  ,
in_Sat         VARCHAR(30)  ,
in_Pres        DOUBLE       ,
in_Sres        DOUBLE       ,
in_Pqual       INT          ,
in_Squal       INT          ,
in_codalen     INT          ,
in_codawt      INT          ,
in_Pfm         CHAR         ,
in_Sfm         CHAR         ,
in_datasrc     CHAR         ,
in_Md          DOUBLE       ,
in_azm         INT          ,
in_takeoff     INT          ,
in_dist        DOUBLE       ,
in_Pwt         DOUBLE       ,
in_Swt         DOUBLE       ,
in_pamp        INT          ,
in_codalenObs  INT          ,
in_ccntr_0     INT          ,
in_ccntr_1     INT          ,
in_ccntr_2     INT          ,
in_ccntr_3     INT          ,
in_ccntr_4     INT          ,
in_ccntr_5     INT          ,
in_caav_0      INT          ,
in_caav_1      INT          ,
in_caav_2      INT          ,
in_caav_3      INT          ,
in_caav_4      INT          ,
in_caav_5      INT          ,
in_version     BIGINT      
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

DECLARE l_ewmodid               BIGINT;
DECLARE l_sqkseq                BIGINT;
DECLARE l_scnlid               BIGINT;

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

SET l_Pat_dt=in_Pat;
SELECT MICROSECOND(in_Pat) INTO l_Pat_usec ;


IF TRIM(in_Sat)='' THEN 
        SELECT 'ERROR: origin time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;

SET l_Sat_dt=in_Sat;
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

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_ewmodid);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qid, l_sqkseq);
CALL `sp_get_ew_scnl_out`(l_site, l_comp, l_net, l_loc, l_scnlid);

INSERT INTO `ew_arc_phase` (fk_module,fk_sqkseq,fk_scnl,Plabel,Slabel,Ponset,Sonset,Pat_dt,Pat_usec,Sat_dt,Sat_usec,Pres,Sres,Pqual,Squal,
                                  codalen,codawt,Pfm,Sfm,datasrc,Md,azm,takeoff,dist,Pwt,Swt,pamp,codalenObs,
                                  ccntr_0,ccntr_1,ccntr_2,ccntr_3,ccntr_4,ccntr_5,caav_0,caav_1,caav_2,caav_3,caav_4,caav_5,version)
    VALUES ( l_ewmodid,l_sqkseq,l_scnlid,l_Plabel,l_Slabel,l_Ponset,l_Sonset,l_Pat_dt,l_Pat_usec,l_Sat_dt,l_Sat_usec,l_Pres,l_Sres,l_Pqual,l_Squal,
	l_codalen,l_codawt,l_Pfm,l_Sfm,l_datasrc,l_Md,l_azm,l_takeoff,l_dist,l_Pwt,l_Swt,l_pamp,l_codalenObs,
	l_ccntr_0,l_ccntr_1,l_ccntr_2,l_ccntr_3,l_ccntr_4,l_ccntr_5,l_caav_0,l_caav_1,l_caav_2,l_caav_3,l_caav_4,l_caav_5,l_version);

SELECT LAST_INSERT_ID();

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_arc_summary;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_arc_summary`(
in_ewname       VARCHAR(80)      ,
in_modname      VARCHAR(32)      ,
in_qid          BIGINT        ,
in_ot           VARCHAR(30)   ,
in_lat          DOUBLE        ,
in_lon          DOUBLE        ,
in_z            DOUBLE        ,
in_nph          INT           ,
in_nphS         INT           ,
in_nphtot       INT           ,
in_nPfm         INT           ,
in_gap          INT           ,
in_dmin         INT           ,
in_rms          DOUBLE        ,
in_e0az         INT           ,
in_e0dp         INT           ,
in_e0           float         ,
in_e1az         INT           ,
in_e1dp         INT           ,
in_e1           DOUBLE        ,
in_e2           DOUBLE        ,
in_erh          DOUBLE        ,
in_erz          DOUBLE        ,
in_Md           DOUBLE        ,
in_reg          VARCHAR(4)    ,
in_labelpref    CHAR          ,
in_Mpref        DOUBLE        ,
in_wtpref       DOUBLE        ,
in_mdtype       CHAR          ,
in_mdmad        DOUBLE        ,
in_mdwt         DOUBLE        ,
in_version      BIGINT        ,
in_quality      CHAR(2)      
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

DECLARE l_ewmodid               BIGINT;
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

SET l_ot_dt= in_ot;
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

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_ewmodid);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qid, l_sqkseq);

INSERT INTO `ew_arc_summary`( fk_module,fk_sqkseq,ot_dt,ot_usec,lat,lon,z,nph,nphS,nphtot,nPfm,
                                    gap,dmin,rms,e0az,e0dp,e0,e1az,e1dp,e1,e2,erh,erz,
                                    Md,reg,labelpref,Mpref,wtpref,mdtype,mdmad,mdwt,version,quality)
VALUES(l_ewmodid,l_sqkseq,l_ot_dt,l_ot_usec,l_lat,l_lon,l_z,l_nph,l_nphS,l_nphtot,l_nPfm,
       l_gap,l_dmin,l_rms,l_e0az,l_e0dp,l_e0,l_e1az,l_e1dp,l_e1,l_e2,l_erh,l_erz,
       l_Md,l_reg,l_labelpref,l_Mpref,l_wtpref,l_mdtype,l_mdmad,l_mdwt,l_version,l_quality);

SELECT LAST_INSERT_ID();

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_coda_scnl;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_coda_scnl`(

in_ewname        VARCHAR(80)      ,
in_modname       VARCHAR(32)      ,
in_seq           INT              ,
in_net           VARCHAR(2)       ,
in_sta           VARCHAR(5)       ,
in_cha           VARCHAR(3)       ,
in_loc           VARCHAR(2)       ,
in_caav_0        BIGINT           ,
in_caav_1        BIGINT           ,
in_caav_2        BIGINT           ,
in_caav_3        BIGINT           ,
in_caav_4        BIGINT           ,
in_caav_5        BIGINT           ,
in_dur           BIGINT          
)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname     VARCHAR(32);
DECLARE l_seq         INT;
DECLARE l_net         VARCHAR(2);
DECLARE l_sta         VARCHAR(5);
DECLARE l_cha         VARCHAR(3);
DECLARE l_loc         VARCHAR(2);
DECLARE l_caav_0      BIGINT;
DECLARE l_caav_1      BIGINT;
DECLARE l_caav_2      BIGINT;
DECLARE l_caav_3      BIGINT;
DECLARE l_caav_4      BIGINT;
DECLARE l_caav_5      BIGINT;
DECLARE l_dur         BIGINT;

DECLARE l_ewmodid               BIGINT;
DECLARE l_scnlid               BIGINT;
DECLARE l_spkseq                BIGINT;

IF TRIM(in_ewname)='' THEN 
        SELECT 'ERROR: EW instance can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_ewname=in_ewname;

IF TRIM(in_modname)='' THEN 
        SELECT 'ERROR: modname can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_modname=in_modname;


IF TRIM(in_seq)='' THEN 
        SELECT 'ERROR: sequence number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_seq=in_seq;

IF TRIM(in_net)='' THEN 
        SELECT 'ERROR: net name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_net=in_net;

IF TRIM(in_sta)='' THEN 
        SELECT 'ERROR: station (site) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_sta=in_sta;

IF TRIM(in_cha)='' THEN 
        SELECT 'ERROR: channel (component) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_cha=in_cha;

IF TRIM(in_loc)='' THEN 
    SET in_loc=null;
END IF;
SET l_loc=in_loc;

SET l_caav_0=in_caav_0;
SET l_caav_1=in_caav_1;
SET l_caav_2=in_caav_2;
SET l_caav_3=in_caav_3;
SET l_caav_4=in_caav_4;
SET l_caav_5=in_caav_5;
SET l_dur=in_dur;

CALL `sp_get_ew_scnl_out`(l_sta, l_cha, l_net, l_loc, l_scnlid);
CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_ewmodid);
CALL `sp_get_ew_spkseq_out`(l_ewname, l_seq, l_spkseq);





UPDATE `ew_pick_scnl`
SET caav_0=l_caav_0,caav_1=l_caav_1,caav_2=l_caav_2,caav_3=l_caav_3,caav_4=l_caav_4,caav_5=l_caav_5,dur=l_dur
WHERE fk_module=l_ewmodid AND fk_spkseq=l_spkseq AND fk_scnl=l_scnlid;

SELECT LAST_INSERT_ID();

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_error;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_error`(
in_ewname                VARCHAR(80)  ,
in_modname               VARCHAR(32)  ,
in_time_dt               VARCHAR(30)  ,
in_message               VARCHAR(256) 
)
uscita: BEGIN
DECLARE l_ewname                VARCHAR(80);
DECLARE l_modname               VARCHAR(32);
DECLARE l_time_dt               DATETIME;
DECLARE l_message               VARCHAR(256);

DECLARE l_ewmodid               BIGINT;
DECLARE l_scnlid               BIGINT;

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

IF TRIM(in_time_dt)='' THEN 
        SELECT 'ERROR: time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_time_dt=in_time_dt;

SET l_message=in_message;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_ewmodid);

INSERT INTO `ew_error` (fk_module,time_dt,message)
VALUES (l_ewmodid, l_time_dt, l_message);

SELECT LAST_INSERT_ID();

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_link;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_link`(
in_ewname        VARCHAR(80)      ,
in_modname       VARCHAR(32)      ,
in_qkseq         BIGINT           ,
in_pkseq         BIGINT           ,
in_iphs          INT
)
uscita: BEGIN
DECLARE l_ewname    VARCHAR(80);
DECLARE l_modname     VARCHAR(32);
DECLARE l_qkseq       INT;
DECLARE l_pkseq       BIGINT;
DECLARE l_iphs        INT;

DECLARE l_ewmodid               BIGINT;
DECLARE l_sqkseq                BIGINT;
DECLARE l_spkseq                BIGINT;

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
        SELECT 'ERROR: link sequence number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_qkseq=in_qkseq;

IF TRIM(in_pkseq)='' THEN 
        SELECT 'ERROR: phase sequence number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_pkseq=in_pkseq;

IF TRIM(in_iphs)='' THEN 
        SELECT 'ERROR: phase number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_iphs=in_iphs;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_ewmodid);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qkseq, l_sqkseq);
CALL `sp_get_ew_spkseq_out`(l_ewname, l_pkseq, l_spkseq);

INSERT INTO `ew_link` (fk_module,fk_sqkseq,fk_spkseq,iphs)
    VALUES (l_ewmodid,l_sqkseq,l_spkseq,l_iphs);

SELECT LAST_INSERT_ID();

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_magnitude_phase;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_magnitude_phase`(
in_ewname                 VARCHAR(80)      ,
in_modname                VARCHAR(32)      ,
in_qid                    BIGINT       ,
in_version                BIGINT       ,
in_site                   VARCHAR(5)   ,
in_comp                   VARCHAR(4)   ,
in_net                    VARCHAR(3)   ,
in_loc                    VARCHAR(2)   ,
in_mag                    DOUBLE       ,
in_dist                   DOUBLE       ,
in_mag_correction         DOUBLE       ,
in_Time1                  VARCHAR(30)  ,
in_Amp1                   DOUBLE       ,
in_Period1                DOUBLE       ,
in_Time2                  VARCHAR(30)  ,
in_Amp2                   DOUBLE       ,
in_Period2                DOUBLE      
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

DECLARE l_ewmodid               BIGINT;
DECLARE l_sqkseq                BIGINT;
DECLARE l_scnlid               BIGINT;

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

SET l_Time1_dt = in_Time1;
SELECT MICROSECOND(in_Time1) INTO l_Time1_usec;

SET l_Amp1 = in_Amp1;
SET l_Period1 = in_Period1;

SET l_Time2_dt = in_Time2;
SELECT MICROSECOND(in_Time2) INTO l_Time2_usec;

SET l_Amp2 = in_Amp2;
SET l_Period2 = in_Period2;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_ewmodid);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qid, l_sqkseq);
CALL `sp_get_ew_scnl_out`(l_site, l_comp, l_net, l_loc, l_scnlid);

INSERT INTO `ew_magnitude_phase` (fk_module, fk_sqkseq, version, fk_scnl, mag, dist, mag_correction, Time1_dt, Time1_usec, Amp1, Period1, Time2_dt, Time2_usec, Amp2, Period2)
VALUES (l_ewmodid,l_sqkseq, l_version, l_scnlid, l_mag, l_dist, l_mag_correction, l_Time1_dt, l_Time1_usec, l_Amp1, l_Period1, l_Time2_dt, l_Time2_usec, l_Amp2, l_Period2);

SELECT LAST_INSERT_ID();

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_magnitude_summary;
DELIMITER $$
 CREATE DEFINER=`root`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_magnitude_summary`(

in_ewname          VARCHAR(80)      ,
in_modname         VARCHAR(32)      ,
in_qid             BIGINT        ,
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

DECLARE l_ewmodid               BIGINT;
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

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_ewmodid);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qid, l_sqkseq);

INSERT INTO `ew_magnitude_summary`(fk_module,fk_sqkseq,version,mag,error,quality,mindist,azimuth,nstations,nchannels,qauthor,qdds_version,imagtype,szmagtype,algorithm,mag_quality)
    VALUES (l_ewmodid,l_sqkseq,l_origin_version,l_mag,l_error,l_quality,l_mindist,l_azimuth,l_nstations,l_nchannels,l_qauthor,l_qdds_version,l_imagtype,l_szmagtype,l_algorithm,l_mag_quality);

SELECT LAST_INSERT_ID();

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_params_pick_sta;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_params_pick_sta`(
    in_ewname              VARCHAR(80),
    in_modname             VARCHAR(80),
    in_Pick_Flag           INT,
    in_Pin_Numb            BIGINT,
    in_Station             CHAR(5),
    in_Comp                CHAR(3),
    in_Net                 CHAR(2),
    in_Loc                 CHAR(2),
    in_Itr1                BIGINT,
    in_MinSmallZC          BIGINT,
    in_MinBigZC            BIGINT,
    in_MinPeakSize         BIGINT,
    in_MaxMint             BIGINT,
    in_i9                  BIGINT,
    in_RawDataFilt         DOUBLE,
    in_CharFuncFilt        DOUBLE,
    in_StaFilt             DOUBLE,
    in_LtaFilt             DOUBLE,
    in_EventThresh         DOUBLE,
    in_RmavFilt            DOUBLE,
    in_DeadSta             DOUBLE,
    in_CodaTerm            DOUBLE,
    in_AltCoda             DOUBLE,
    in_PreEvent            DOUBLE,
    in_Erefs               DOUBLE,
    in_ClipCount           BIGINT
)
BEGIN
DECLARE l_seisnet_is 		BIGINT DEFAULT 0;
DECLARE l_ext_id 		BIGINT DEFAULT NULL;
DECLARE l_scnlid_present	BIGINT DEFAULT 0;


DECLARE l_scnlid_last_revision	BIGINT DEFAULT 0;
DECLARE l_id_max_new_revision	BIGINT DEFAULT 0;
DECLARE l_id_max_revision	BIGINT DEFAULT 0;

DECLARE l_ewmodid               BIGINT;
DECLARE l_scnlid               BIGINT;

CALL `sp_get_ew_scnl_out`(in_Station, in_Comp, in_Net, in_Loc, l_scnlid);
CALL `sp_get_ew_module_out`(in_ewname, in_modname, l_ewmodid);



SELECT COUNT(*) INTO l_seisnet_is
FROM information_schema.SCHEMATA
WHERE SCHEMA_NAME = 'seisnet';

IF l_seisnet_is > 0 THEN
    
    
    
    SELECT c.id INTO l_ext_id FROM `seisnet`.`channel_52` c
	    JOIN `seisnet`.`station` s ON s.id = c.fk_station
	    JOIN `seisnet`.`network` n ON n.id = c.fk_network
    WHERE 	n.net_code = in_Net
	    AND s.id_inter = in_Station
	    AND c.code = in_Comp
	    AND c.end_time > NOW()
	    AND c.main = 1
	    AND IF (ISNULL(c.location), '--', c.location) = IF (ISNULL(in_Loc), '--', in_Loc);
    
END IF;


SELECT DISTINCT fk_scnl INTO l_scnlid_present FROM `ew_params_pick_sta`
WHERE fk_scnl = l_scnlid AND fk_module=l_ewmodid;


IF l_scnlid_present = 0 THEN
	INSERT INTO `ew_params_pick_sta` (ext_id, fk_module, fk_scnl, revision, pick_flag, itr1, minsmallzc, minbigzc, minpeaksize, maxmint, i9, rawdatafilt, charfuncfilt, stafilt, ltafilt, eventthresh, rmavfilt, deadsta, codaterm, altcoda, preevent, erefs, clipcount)
	VALUES (l_ext_id, l_ewmodid, l_scnlid, 0, in_Pick_Flag, in_Itr1, in_MinSmallZC, in_MinBigZC, in_MinPeakSize, in_MaxMint, in_i9, in_RawDataFilt, in_CharFuncFilt, in_StaFilt, in_LtaFilt, in_EventThresh, in_RmavFilt, in_DeadSta, in_CodaTerm, in_AltCoda, in_PreEvent, in_Erefs, in_ClipCount);
	
	SELECT 0, in_Station, in_Comp, in_Net, in_Loc, l_ext_id, l_scnlid, l_ewmodid, 0, 'channel inserted';
ELSE

    
    SELECT MAX(revision) INTO l_id_max_revision FROM `ew_params_pick_sta`
    WHERE fk_scnl = l_scnlid AND fk_module=l_ewmodid;

    SET l_id_max_new_revision=l_id_max_revision+1;

    
	
	
	
    
    SELECT fk_scnl INTO l_scnlid_last_revision  FROM `ew_params_pick_sta`
    WHERE
    fk_scnl = l_scnlid
    AND fk_module=l_ewmodid
    AND revision = l_id_max_revision
    AND pick_flag = in_Pick_Flag
    AND itr1 = in_Itr1
    AND minsmallzc = in_MinSmallZC
    AND minbigzc = in_MinBigZC
    AND minpeaksize = in_MinPeakSize
    AND maxmint = in_MaxMint
    AND i9 = in_i9
    AND ABS(rawdatafilt - in_RawDataFilt) <= 0.0000001
    AND ABS(charfuncfilt - in_CharFuncFilt) <= 0.0000001
    AND ABS(stafilt - in_StaFilt) <= 0.0000001
    AND ABS(ltafilt - in_LtaFilt) <= 0.0000001
    AND ABS(eventthresh - in_EventThresh) <= 0.0000001
    AND ABS(rmavfilt - in_RmavFilt) <= 0.0000001
    AND ABS(deadsta - in_DeadSta) <= 0.0000001
    AND ABS(codaterm - in_CodaTerm) <= 0.0000001
    AND ABS(altcoda - in_AltCoda) <= 0.0000001
    AND ABS(preevent - in_PreEvent) <= 0.0000001
    AND ABS(erefs - in_Erefs) <= 0.0000001
    AND clipcount = in_ClipCount
    AND ext_id = l_ext_id;

    IF l_scnlid_last_revision = 0 THEN
	
	INSERT INTO `ew_params_pick_sta` (ext_id, fk_module, fk_scnl, revision, pick_flag, itr1, minsmallzc, minbigzc, minpeaksize, maxmint, i9, rawdatafilt, charfuncfilt, stafilt, ltafilt, eventthresh, rmavfilt, deadsta, codaterm, altcoda, preevent, erefs, clipcount)
	VALUES (l_ext_id, l_ewmodid, l_scnlid, l_id_max_new_revision, in_Pick_Flag, in_Itr1, in_MinSmallZC, in_MinBigZC, in_MinPeakSize, in_MaxMint, in_i9, in_RawDataFilt, in_CharFuncFilt, in_StaFilt, in_LtaFilt, in_EventThresh, in_RmavFilt, in_DeadSta, in_CodaTerm, in_AltCoda, in_PreEvent, in_Erefs, in_ClipCount);
	
	SELECT 0, in_Station, in_Comp, in_Net, in_Loc, l_ext_id, l_scnlid, l_ewmodid, l_id_max_new_revision, 'channel inserted';
    ELSE
	
	
	SELECT -2, in_Station, in_Comp, in_Net, in_Loc, l_ext_id, l_scnlid, l_ewmodid, l_id_max_revision, 'equal to last revision';
    END IF;


    
    
    

END IF;

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_pick_scnl;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_pick_scnl`(

in_ewname        VARCHAR(80)      ,
in_modname       VARCHAR(32)      ,
in_seq           INT              ,
in_net           VARCHAR(2)       ,
in_sta           VARCHAR(5)       ,
in_cha           VARCHAR(3)       ,
in_loc           VARCHAR(2)       ,
in_dir           CHAR(1)          ,
in_wt            TINYINT          ,
in_tpick         VARCHAR(30)      ,
in_pamp_0        BIGINT           ,
in_pamp_1        BIGINT           ,    
in_pamp_2        BIGINT                  
)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname     VARCHAR(32);
DECLARE l_seq         INT;
DECLARE l_net         VARCHAR(2);
DECLARE l_sta         VARCHAR(5);
DECLARE l_cha         VARCHAR(3);
DECLARE l_loc         VARCHAR(2);
DECLARE l_dir         CHAR(1);
DECLARE l_wt          TINYINT;
DECLARE l_tpick_dt    DATETIME;
DECLARE l_tpick_usec  BIGINT;
DECLARE l_pamp_0      BIGINT;
DECLARE l_pamp_1      BIGINT;
DECLARE l_pamp_2      BIGINT;
DECLARE l_ew_pick_id  BIGINT;

DECLARE l_ewmodid               BIGINT;
DECLARE l_scnlid               BIGINT;
DECLARE l_spkseq                BIGINT;

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


IF TRIM(in_seq)='' THEN 
        SELECT 'ERROR: sequence number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_seq=in_seq;

IF TRIM(in_net)='' THEN 
        SELECT 'ERROR: net name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_net=in_net;

IF TRIM(in_sta)='' THEN 
        SELECT 'ERROR: station (site) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_sta=in_sta;

IF TRIM(in_cha)='' THEN 
        SELECT 'ERROR: channel (component) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_cha=in_cha;

IF TRIM(in_loc)='' THEN 
    SET in_loc=null;
END IF;
SET l_loc=in_loc;

IF TRIM(in_dir)='' THEN 
        SELECT 'ERROR: first motion can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_dir=in_dir;

IF TRIM(in_wt)='' THEN 
        SELECT 'ERROR: weight can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_wt=in_wt;

IF TRIM(in_tpick)='' THEN 
        SELECT 'ERROR: pick time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;

SET l_tpick_dt=in_tpick;
SELECT MICROSECOND(in_tpick) INTO l_tpick_usec ;

SET l_pamp_0=in_pamp_0;
SET l_pamp_1=in_pamp_1;
SET l_pamp_2=in_pamp_2;

CALL `sp_get_ew_scnl_out`(l_sta, l_cha, l_net, l_loc, l_scnlid);
CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_ewmodid);
CALL `sp_get_ew_spkseq_out`(l_ewname, l_seq, l_spkseq);

INSERT INTO `ew_pick_scnl` (fk_module,fk_spkseq,fk_scnl,dir,wt,tpick_dt,tpick_usec,pamp_0,pamp_1,pamp_2)
    VALUES (l_ewmodid,l_spkseq,l_scnlid,l_dir,l_wt,l_tpick_dt,l_tpick_usec,l_pamp_0,l_pamp_1,l_pamp_2);

SELECT LAST_INSERT_ID();

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_quake2k;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_quake2k`(
in_ewname                VARCHAR(80)      ,
in_modname               VARCHAR(32)      ,
in_qkseq                 INT              ,
in_quake_ot              VARCHAR(30)       ,
in_lat                   DOUBLE       ,
in_lon                   DOUBLE       ,
in_depth                 DOUBLE       ,
in_rms                   DOUBLE       ,
in_dmin                  DOUBLE       ,
in_ravg                  DOUBLE       ,
in_gap                   DOUBLE       ,
in_nph                   INT         
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

DECLARE l_ewmodid               BIGINT;
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

SET l_quake_ot_dt=in_quake_ot;
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


CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_ewmodid);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qkseq, l_sqkseq);

INSERT INTO `ew_quake2k` (fk_module,fk_sqkseq,quake_ot_dt,quake_ot_usec,lat,lon,depth,rms,dmin,ravg,gap,nph)
VALUES (l_ewmodid,l_sqkseq,l_quake_ot_dt,l_quake_ot_usec,l_lat,l_lon,l_depth,l_rms,l_dmin,l_ravg,l_gap,l_nph);

SELECT LAST_INSERT_ID();

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_scnl;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_scnl`(
in_sta                   VARCHAR(5)   ,
in_cha                   VARCHAR(3)   ,
in_net                   VARCHAR(2)   ,
in_loc                   VARCHAR(2)   ,
in_lat                   DOUBLE       ,
in_lon                   DOUBLE       ,
in_ele                   DOUBLE       ,
in_name                  VARCHAR(80)  ,
in_ext_id                BIGINT      
)
uscita: BEGIN
DECLARE l_sta                   VARCHAR(5);
DECLARE l_cha                   VARCHAR(3);
DECLARE l_net                   VARCHAR(2);
DECLARE l_loc                   VARCHAR(2);
DECLARE l_lat                   DOUBLE;
DECLARE l_lon                   DOUBLE;
DECLARE l_ele                   DOUBLE;
DECLARE l_name                  VARCHAR(80);
DECLARE l_ext_id                BIGINT;

DECLARE l_scnlid                BIGINT;

IF TRIM(in_sta)='' THEN 
        SELECT 'ERROR: station (site) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_sta=in_sta;

IF TRIM(in_cha)='' THEN 
        SELECT 'ERROR: channel (component) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_cha=in_cha;

IF TRIM(in_net)='' THEN 
        SELECT 'ERROR: net name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_net=in_net;

IF TRIM(in_loc)='' THEN 
    SET in_loc=null;
END IF;
SET l_loc=in_loc;

SET l_lat=in_lat;
SET l_lon=in_lon;
SET l_ele=in_ele;

IF TRIM(in_name)='' THEN 
    SET in_name=null;
END IF;
SET l_name=in_name;

IF TRIM(in_ext_id) < 0 THEN 
    SET in_ext_id=null;
END IF;
SET l_ext_id=in_ext_id;

CALL `sp_get_ew_scnl_out`(l_sta, l_cha, l_net, l_loc, l_scnlid);

UPDATE `ew_scnl`
    SET lat=l_lat,lon=l_lon,ele=l_ele,name=l_name,ext_id=l_ext_id
    WHERE id=l_scnlid;

SELECT l_scnlid;

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_strongmotionII;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_strongmotionII`(
in_ewname        VARCHAR(80)      ,
in_modname       VARCHAR(32)      ,
in_qid           BIGINT           ,

in_site          VARCHAR(5)       ,
in_comp          VARCHAR(3)       ,
in_net           VARCHAR(2)       ,
in_loc           VARCHAR(2)       ,
in_qauthor       VARCHAR(32),
in_t             VARCHAR(30)      ,
in_t_alt         VARCHAR(30)      ,
in_altcode       INT              ,
in_pga           DOUBLE           ,
in_tpga          VARCHAR(30)      ,
in_pgv           DOUBLE           ,
in_tpgv          VARCHAR(30)      ,
in_pgd           DOUBLE           ,
in_tpgd          VARCHAR(30)      ,
in_rsa           VARCHAR(256)    
)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname     VARCHAR(32);
DECLARE l_qid         BIGINT;

DECLARE l_site        VARCHAR(5);
DECLARE l_net         VARCHAR(3);
DECLARE l_comp        VARCHAR(4);
DECLARE l_loc         VARCHAR(2);
DECLARE l_qauthor     VARCHAR(32);
DECLARE l_t_dt        DATETIME;
DECLARE l_t_usec      DOUBLE;
DECLARE l_t_alt_dt    DATETIME;
DECLARE l_t_alt_usec  DATETIME;
DECLARE l_altcode     INT;
DECLARE l_pga         DOUBLE;
DECLARE l_tpga_dt     DATETIME;
DECLARE l_tpga_usec   DOUBLE;
DECLARE l_pgv         DOUBLE;
DECLARE l_tpgv_dt     DATETIME;
DECLARE l_tpgv_usec   DOUBLE;
DECLARE l_pgd         DOUBLE;
DECLARE l_tpgd_dt     DATETIME;
DECLARE l_tpgd_usec   DOUBLE;
DECLARE l_rsa         VARCHAR(256);

DECLARE l_ewmodid     BIGINT;
DECLARE l_sqkseq      BIGINT;
DECLARE l_scnlid      BIGINT;

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

SET l_site = in_site;
SET l_net = in_net;
SET l_comp = in_comp;

IF TRIM(in_loc)='' THEN 
     SET in_loc = null;
END IF;
SET l_loc = in_loc;

SET l_qauthor=in_qauthor;

SET l_t_dt = in_t;
SELECT MICROSECOND(in_t) INTO l_t_usec;

SET l_t_alt_dt = in_t_alt;

    SELECT MICROSECOND(in_t_alt) INTO l_t_alt_usec;


SET l_altcode = in_altcode;
SET l_pga = in_pga;

SET l_tpga_dt = in_tpga;
SELECT MICROSECOND(in_tpga) INTO l_tpga_usec;

SET l_pgv = in_pgv;

SET l_tpgv_dt = in_tpgv;
SELECT MICROSECOND(in_tpgv) INTO l_tpgv_usec;

SET l_pgd = in_pgd;

SET l_tpgd_dt = in_tpgd;
SELECT MICROSECOND(in_tpgd) INTO l_tpgd_usec;

SET l_rsa = in_rsa;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_ewmodid);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qid, l_sqkseq);
CALL `sp_get_ew_scnl_out`(l_site, l_comp, l_net, l_loc, l_scnlid);

INSERT INTO `ew_strongmotionII` (fk_module,fk_sqkseq,fk_scnl,qauthor,
        t_dt,t_usec,t_alt_dt,t_alt_usec,altcode,
        pga,tpga_dt,tpga_usec,pgv,tpgv_dt,tpgv_usec,pgd,tpgd_dt,tpgd_usec,rsa)
VALUES (l_ewmodid,l_sqkseq,l_scnlid,l_qauthor,
        l_t_dt,l_t_usec,l_t_alt_dt,l_t_alt_usec,l_altcode,
        l_pga,l_tpga_dt,l_tpga_usec,l_pgv,l_tpgv_dt,l_tpgv_usec,l_pgd,l_tpgd_dt,l_tpgd_usec,l_rsa);

SELECT LAST_INSERT_ID();

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_ins_ew_tracebuf;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_ins_ew_tracebuf`(
in_ewname                VARCHAR(80)  ,
in_modname               VARCHAR(32)  ,
in_pinno                 BIGINT       ,
in_nsamp                 BIGINT       ,
in_starttime             VARCHAR(30)  ,
in_endtime               VARCHAR(30)  ,
in_samprate              DOUBLE       ,
in_net                   VARCHAR(2)   ,
in_sta                   VARCHAR(5)   ,
in_cha                   VARCHAR(3)   ,
in_loc                   VARCHAR(2)   ,
in_version               VARCHAR(10)  ,
in_datatype              VARCHAR(3)   ,
in_quality               VARCHAR(10) 
)
uscita: BEGIN
DECLARE l_ewname                VARCHAR(80);
DECLARE l_modname               VARCHAR(32);
DECLARE l_pinno                 BIGINT;
DECLARE l_nsamp                 BIGINT;
DECLARE l_starttime_dt          DATETIME;
DECLARE l_starttime_usec        BIGINT;
DECLARE l_endtime_dt            DATETIME;
DECLARE l_endtime_usec          BIGINT;
DECLARE l_samprate              DOUBLE;
DECLARE l_net                   VARCHAR(2);
DECLARE l_sta                   VARCHAR(5);
DECLARE l_cha                   VARCHAR(3);
DECLARE l_loc                   VARCHAR(2);
DECLARE l_version               VARCHAR(10);
DECLARE l_datatype              VARCHAR(3);
DECLARE l_quality               VARCHAR(10);

DECLARE l_ewmodid               BIGINT;
DECLARE l_scnlid               BIGINT;

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


SET l_pinno=in_pinno;
SET l_nsamp=in_nsamp;

IF TRIM(in_starttime)='' THEN 
        SELECT 'ERROR: start time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_starttime_dt=in_starttime;
SELECT MICROSECOND(in_starttime) INTO l_starttime_usec ;

IF TRIM(in_endtime)='' THEN 
        SELECT 'ERROR: end time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_endtime_dt=in_endtime;
SELECT MICROSECOND(in_endtime) INTO l_endtime_usec ;

SET l_samprate=in_samprate;

IF TRIM(in_net)='' THEN 
        SELECT 'ERROR: net name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_net=in_net;

IF TRIM(in_sta)='' THEN 
        SELECT 'ERROR: station (site) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_sta=in_sta;

IF TRIM(in_cha)='' THEN 
        SELECT 'ERROR: channel (component) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_cha=in_cha;

IF TRIM(in_loc)='' THEN 
    SET in_loc=null;
END IF;
SET l_loc=in_loc;

SET l_version=in_version;
SET l_datatype=in_datatype;
SET l_quality=in_quality;

CALL `sp_get_ew_scnl_out`(l_sta, l_cha, l_net, l_loc, l_scnlid);
CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_ewmodid);

INSERT INTO `ew_tracebuf`(fk_module,pinno,nsamp,starttime_dt,starttime_usec,endtime_dt,endtime_usec,samprate,fk_scnl,version,datatype,quality)
    VALUES (l_ewmodid,l_pinno,l_nsamp,l_starttime_dt,l_starttime_usec,l_endtime_dt,l_endtime_usec,l_samprate,l_scnlid,l_version,l_datatype,l_quality);

SELECT LAST_INSERT_ID();

END
$$
DELIMITER ;
DROP PROCEDURE IF EXISTS sp_sel_ew_event_phases;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` PROCEDURE `sp_sel_ew_event_phases`(
in_start_time       VARCHAR(50),
in_end_time         VARCHAR(50)
)
uscita: BEGIN
    
    IF fn_isdate(in_start_time) THEN
        SELECT'ERROR: start_time not datatime format yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;
    IF fn_isdate(in_end_time) THEN
        SELECT'ERROR: end_time not datatime format  yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;

    SELECT CAST(CONCAT(`e`.`fk_sqkseq`, '_', `e`.`version`) AS CHAR(50)) AS `sqkid_version`,
    `e`.`fk_sqkseq` AS `sqkid`,
    `e`.`qkseq` AS `qkseq`,
    `e`.`ewname` AS `instance`,
    CAST(CONCAT(`e`.`ot_dt`, '.', LEFT(`e`.`ot_usec`,4)) AS CHAR(50)) AS `ot_time`,
    `e`.`mag` AS `mag`,
    `e`.`mag_type` AS `mag_type`,
    `e`.`mag_error` AS `mag_error`,
    `e`.`arc_quality` AS `quality`,
    ROUND(`e`.`lat`,3) AS `lat`,
    ROUND(`e`.`lon`,3) AS `lon`,
    ROUND(`e`.`z`,3) AS `depth`,
    `e`.`erh` AS `error_h`,
    `e`.`erz` AS `error_z`,
    `e`.`nphtot` AS `nphtot`,
    `e`.`gap` AS `gap`,
    `e`.`dmin` AS `dmin`,
    `e`.`rms` AS `rms`,
    `e`.`ewname` AS `ewname`,
    `e`.`mag_ewmodid` AS `modmag`,
    `e`.`region` AS `region_name`,
    `e`.`version` AS `version`,
    `e`.`modified` AS `modified`,
    `s`.`net`,
    `s`.`sta`,
    `s`.`cha`,
    `p`.`dist`,
    `p`.`azm`,
    IF(`p`.`Ponset` = 'P', CONCAT('P', `p`.`Plabel`), IF(`p`.`Sonset` = 'S', CONCAT('S', `p`.`Slabel`), NULL)) as `phasename`,
    IF(`p`.`Ponset` = 'P', CAST(fn_get_str_from_dt_usec(`p`.`Pat_dt`, `p`.`Pat_usec`) AS CHAR), IF(`p`.`Sonset` = 'S',  CAST(fn_get_str_from_dt_usec(`p`.`Sat_dt`, `p`.`Sat_usec`) AS CHAR), NULL)) as `phasetime`,
    IF(`p`.`Ponset` = 'P', `p`.`Pwt`, IF(`p`.`Sonset` = 'S', `p`.`Swt`, NULL)) as `phaseweight`,
    IF(`p`.`Ponset` = 'P', `p`.`Pres`, IF(`p`.`Sonset` = 'S', `p`.`Sres`, NULL)) as `phaseresidual`,
    IF(`p`.`Ponset` = 'P', `p`.`Pqual`, IF(`p`.`Sonset` = 'S', `p`.`Squal`, NULL)) as `phasequality`
    
    FROM `ew_events_summary` e
    JOIN `ew_arc_phase` p ON `p`.`fk_sqkseq` = `e`.`fk_sqkseq` AND `p`.`version` = `e`.`version`
    JOIN `ew_scnl` s ON `p`.`fk_scnl` = `s`.`id`
    
    WHERE `e`.`ot_dt` >= in_start_time 
        AND `e`.`ot_dt` <= in_end_time
    ORDER BY `e`.`fk_sqkseq` DESC, `e`.`version` DESC, `p`.`dist` ASC;
END
$$
DELIMITER ;

-- FUNCTIONS
DROP FUNCTION IF EXISTS fn_delta;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` FUNCTION `fn_delta`(
in_lat1 float,
in_lon1 float,
in_lat2 float,
in_lon2 float) RETURNS float
    DETERMINISTIC
BEGIN
DECLARE l_g1            FLOAT;
DECLARE l_g2            FLOAT;
DECLARE l_gr            FLOAT;
DECLARE l_a             FLOAT;
DECLARE l_b             FLOAT;
DECLARE l_rlonep        FLOAT;
DECLARE l_gamma2        FLOAT;
DECLARE l_amenb2        FLOAT;
DECLARE l_apiub2        FLOAT;
DECLARE l_tgaamenbb2    FLOAT;
DECLARE l_tgaapiubb2    FLOAT;
DECLARE l_aamenbb2      FLOAT;
DECLARE l_aapiubb2      FLOAT;
DECLARE l_tgc2s         FLOAT;
DECLARE l_cs            FLOAT;
DECLARE l_tgc2p         FLOAT;
DECLARE l_cp            FLOAT;
DECLARE l_azim          FLOAT;
DECLARE l_delta         FLOAT;
DECLARE l_rlonst        FLOAT;

SET l_g1=6.28318531;
SET l_g2=360;
SET l_gr=l_g1/l_g2;
SET l_b=(90-in_lat1)*l_gr;      
SET l_rlonep=in_lon1*l_gr;
SET l_a=(90-in_lat2)*l_gr;       
SET l_rlonst=in_lon2*l_gr;
SET l_gamma2=(l_rlonst-l_rlonep)/2;
SET l_amenb2=(l_a-l_b)/2;
SET l_apiub2=(l_a+l_b)/2;
IF (ABS(l_gamma2)<(0.000001*l_gr)) THEN
    SET l_delta=(ABS(in_lat1-in_lat2)*l_gr);
ELSE
    IF (in_lat1=in_lat2) THEN
        SET l_tgaamenbb2=(1/Tan(l_gamma2))*Sin(l_amenb2)/Sin(l_apiub2);
        SET l_tgaapiubb2=(1/Tan(l_gamma2))*Cos(l_amenb2)/Cos(l_apiub2);
        SET l_aamenbb2=Atan(l_tgaamenbb2);
        SET l_aapiubb2=Atan(l_tgaapiubb2);
        SET l_tgc2s=Tan(l_apiub2)*Cos(l_aapiubb2)/Cos(l_aamenbb2);
        SET l_cs=2*Atan(l_tgc2s);
        SET l_delta=l_cs;
    ELSE
        SET l_tgaamenbb2=(1/Tan(l_gamma2))*Sin(l_amenb2)/Sin(l_apiub2);
        SET l_tgaapiubb2=(1/Tan(l_gamma2))*Cos(l_amenb2)/Cos(l_apiub2);
        SET l_aamenbb2=Atan(l_tgaamenbb2);
        SET l_aapiubb2=Atan(l_tgaapiubb2);
        SET l_tgc2p=Tan(l_amenb2)*Sin(l_aapiubb2)/Sin(l_aamenbb2);
        SET l_cp=2*Atan(l_tgc2p);
        SET l_delta=l_cp;
    END IF;
END IF;
RETURN Abs((l_delta/l_gr)*111.195);
END
$$
DELIMITER ;
DROP FUNCTION IF EXISTS fn_exists_auth_ext_seisev_db;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` FUNCTION `fn_exists_auth_ext_seisev_db`(
in_dbname              VARCHAR(50)             
) RETURNS int(11)
    DETERMINISTIC
BEGIN

    
    

    DECLARE l_ret                   INT             DEFAULT 0;
    DECLARE l_seisev_is             TINYINT         DEFAULT 0;

    SELECT COUNT(*) INTO l_seisev_is 
    FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME='seisev';





    IF l_seisev_is > 0 THEN
	SET l_ret = `seisev`.`fn_fill_seisev`();
    END IF;

    RETURN l_ret;

END
$$
DELIMITER ;
DROP FUNCTION IF EXISTS fn_find_region_name;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` FUNCTION `fn_find_region_name`(
in_lat                   DOUBLE     ,
in_lon                   DOUBLE    
) RETURNS varchar(1024) CHARSET latin1
    READS SQL DATA
BEGIN
    DECLARE l_region    VARCHAR(1024);
    DECLARE l_sp        VARCHAR(1024);
    DECLARE l_p         GEOMETRY;

    SET l_region = NULL;
    
    
    
    
    SET l_sp = CONCAT('POINT', '(',CAST(in_lat AS CHAR) ,' ',  CAST(in_lon AS CHAR), ')' );
    SET l_p = GeomFromText(l_sp);

    SELECT
    region
    
    INTO l_region
    FROM (
	SELECT *
	FROM ew_region_geom
	WHERE Contains(g, l_p)
    ) tmp_table1
    ORDER BY fn_pnpoly(g, in_lat, in_lon)  DESC, kind, Area(g)
    LIMIT 1
    ;

    if l_region IS NULL then 
	SET l_region = 'Region not found!'; 
    END IF;

    RETURN l_region;
END
$$
DELIMITER ;
DROP FUNCTION IF EXISTS fn_get_str_from_dt_usec;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` FUNCTION `fn_get_str_from_dt_usec`(
in_dt              DATETIME              ,
in_usec            BIGINT
) RETURNS char(24) CHARSET latin1
    DETERMINISTIC
BEGIN


RETURN CONCAT(CAST(in_dt AS CHAR),  SUBSTR((in_usec/1000000.0), 2) );

END
$$
DELIMITER ;
DROP FUNCTION IF EXISTS fn_get_suffix_from_datetime;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` FUNCTION `fn_get_suffix_from_datetime`(
in_dt              DATETIME             
) RETURNS char(7) CHARSET latin1
    DETERMINISTIC
BEGIN


RETURN CONCAT(SUBSTRING(in_dt, 1, 4), '_', SUBSTRING(in_dt, 6, 2));

END
$$
DELIMITER ;
DROP FUNCTION IF EXISTS fn_get_tbname;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` FUNCTION `fn_get_tbname`(
in_dbname                VARCHAR(80)   ,
in_dt                    DATETIME      ,
in_tbname                VARCHAR(80)  
) RETURNS varchar(255) CHARSET latin1
    DETERMINISTIC
BEGIN


RETURN CONCAT('`', in_dbname, '_', fn_get_suffix_from_datetime(in_dt), '`.`', in_tbname, '`');


END
$$
DELIMITER ;
DROP FUNCTION IF EXISTS fn_isdate;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` FUNCTION `fn_isdate`(in_str VARCHAR(255)) RETURNS tinyint(4)
    DETERMINISTIC
BEGIN
IF LENGTH(DATE(in_str)) IS NOT NULL AND LENGTH(TIME(in_str)) IS NOT NULL THEN
    RETURN 0;
ELSE
    RETURN 1;
END IF;
END
$$
DELIMITER ;
DROP FUNCTION IF EXISTS fn_pnpoly;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` FUNCTION `fn_pnpoly`(
in_g    GEOMETRY,
in_X    DOUBLE,
in_Y    DOUBLE
) RETURNS int(11)
    DETERMINISTIC
BEGIN
    DECLARE l_linestring          LineString DEFAULT NULL;
    DECLARE l_num_points          BIGINT DEFAULT NULL;
    DECLARE l_Xj                  DOUBLE DEFAULT 0;
    DECLARE l_Xi                  DOUBLE DEFAULT 0;
    DECLARE l_Yj                  DOUBLE DEFAULT 0;
    DECLARE l_Yi                  DOUBLE DEFAULT 0;
    DECLARE l_in                  INT DEFAULT -1;

    DECLARE i                     INT DEFAULT 0;

    SET l_linestring=ExteriorRing(in_g);
    SET l_num_points=NumPoints(l_linestring);

    SET l_Xj = X(PointN(l_linestring, l_num_points));
    SET l_Yj = Y(PointN(l_linestring, l_num_points));
    SET i = 1;
    WHILE (i <= l_num_points) DO
	SET l_Xi = X(PointN(l_linestring, i));
	SET l_Yi = Y(PointN(l_linestring, i));
	IF( ((l_Yi>in_Y) <> (l_Yj>in_Y)) AND
	    ( in_X <  ( (l_Xj-l_Xi) * ((in_Y-l_Yi) / (l_Yj-l_Yi)) + l_Xi) ) ) THEN
		SET l_in = l_in * (-1);
	END IF;
	SET l_Xj = l_Xi;
	SET l_Yj = l_Yi;
	SET i = i + 1;
    END WHILE;

    RETURN l_in;
END
$$
DELIMITER ;

-- LOCK TABLES must be the FIRST sql command
LOCK TABLES dataface__failed_logins WRITE, dataface__mtimes WRITE, dataface__preferences WRITE, dataface__version WRITE, datafaceew_users WRITE, err_sp WRITE, ew_arc_phase WRITE, ew_arc_summary WRITE, ew_error WRITE, ew_events_summary WRITE, ew_instance WRITE, ew_link WRITE, ew_magnitude_phase WRITE, ew_magnitude_summary WRITE, ew_module WRITE, ew_params_pick_sta WRITE, ew_pick_scnl WRITE, ew_quake2k WRITE, ew_region_geom WRITE, ew_region_kind WRITE, ew_scnl WRITE, ew_spkseq WRITE, ew_sqkseq WRITE, ew_strongmotionII WRITE, ew_tracebuf WRITE;

delete ew_arc_phase.* from ew_arc_phase left join ew_module on (ew_arc_phase.ewmodid=ew_module.id) where ew_module.id is NULL;
delete ew_arc_phase.* from ew_arc_phase left join ew_scnl on (ew_arc_phase.scnlid=ew_scnl.id) where ew_scnl.id is NULL;
delete ew_arc_phase.* from ew_arc_phase left join ew_sqkseq on (ew_arc_phase.ewsqkid=ew_sqkseq.id) where ew_sqkseq.id is NULL;
delete ew_arc_summary.* from ew_arc_summary left join ew_module on (ew_arc_summary.ewmodid=ew_module.id) where ew_module.id is NULL;
delete ew_arc_summary.* from ew_arc_summary left join ew_sqkseq on (ew_arc_summary.ewsqkid=ew_sqkseq.id) where ew_sqkseq.id is NULL;
delete ew_error.* from ew_error left join ew_module on (ew_error.ewmodid=ew_module.id) where ew_module.id is NULL;
delete ew_events_summary.* from ew_events_summary left join ew_sqkseq on (ew_events_summary.ewsqkid=ew_sqkseq.id) where ew_sqkseq.id is NULL;
delete ew_link.* from ew_link left join ew_module on (ew_link.ewmodid=ew_module.id) where ew_module.id is NULL;
delete ew_link.* from ew_link left join ew_sqkseq on (ew_link.ewsqkid=ew_sqkseq.id) where ew_sqkseq.id is NULL;
delete ew_link.* from ew_link left join ew_spkseq on (ew_link.ewspkid=ew_spkseq.id) where ew_spkseq.id is NULL;
delete ew_magnitude_phase.* from ew_magnitude_phase left join ew_module on (ew_magnitude_phase.ewmodid=ew_module.id) where ew_module.id is NULL;
delete ew_magnitude_phase.* from ew_magnitude_phase left join ew_scnl on (ew_magnitude_phase.scnlid=ew_scnl.id) where ew_scnl.id is NULL;
delete ew_magnitude_phase.* from ew_magnitude_phase left join ew_sqkseq on (ew_magnitude_phase.ewsqkid=ew_sqkseq.id) where ew_sqkseq.id is NULL;
delete ew_magnitude_summary.* from ew_magnitude_summary left join ew_module on (ew_magnitude_summary.ewmodid=ew_module.id) where ew_module.id is NULL;
delete ew_magnitude_summary.* from ew_magnitude_summary left join ew_sqkseq on (ew_magnitude_summary.ewsqkid=ew_sqkseq.id) where ew_sqkseq.id is NULL;
delete ew_module.* from ew_module left join ew_instance on (ew_module.ewid=ew_instance.id) where ew_instance.id is NULL;
delete ew_params_pick_sta.* from ew_params_pick_sta left join ew_module on (ew_params_pick_sta.ewmodid=ew_module.id) where ew_module.id is NULL;
delete ew_params_pick_sta.* from ew_params_pick_sta left join ew_scnl on (ew_params_pick_sta.scnlid=ew_scnl.id) where ew_scnl.id is NULL;
delete ew_pick_scnl.* from ew_pick_scnl left join ew_module on (ew_pick_scnl.ewmodid=ew_module.id) where ew_module.id is NULL;
delete ew_pick_scnl.* from ew_pick_scnl left join ew_scnl on (ew_pick_scnl.scnlid=ew_scnl.id) where ew_scnl.id is NULL;
delete ew_pick_scnl.* from ew_pick_scnl left join ew_spkseq on (ew_pick_scnl.ewspkid=ew_spkseq.id) where ew_spkseq.id is NULL;
delete ew_quake2k.* from ew_quake2k left join ew_module on (ew_quake2k.ewmodid=ew_module.id) where ew_module.id is NULL;
delete ew_quake2k.* from ew_quake2k left join ew_sqkseq on (ew_quake2k.ewsqkid=ew_sqkseq.id) where ew_sqkseq.id is NULL;
delete ew_spkseq.* from ew_spkseq left join ew_instance on (ew_spkseq.ewid=ew_instance.id) where ew_instance.id is NULL;
delete ew_sqkseq.* from ew_sqkseq left join ew_instance on (ew_sqkseq.ewid=ew_instance.id) where ew_instance.id is NULL;
delete ew_strongmotionII.* from ew_strongmotionII left join ew_module on (ew_strongmotionII.ewmodid=ew_module.id) where ew_module.id is NULL;
delete ew_strongmotionII.* from ew_strongmotionII left join ew_scnl on (ew_strongmotionII.scnlid=ew_scnl.id) where ew_scnl.id is NULL;
delete ew_strongmotionII.* from ew_strongmotionII left join ew_sqkseq on (ew_strongmotionII.ewsqkid=ew_sqkseq.id) where ew_sqkseq.id is NULL;
delete ew_tracebuf.* from ew_tracebuf left join ew_module on (ew_tracebuf.ewmodid=ew_module.id) where ew_module.id is NULL;
delete ew_tracebuf.* from ew_tracebuf left join ew_scnl on (ew_tracebuf.scnlid=ew_scnl.id) where ew_scnl.id is NULL;
ALTER TABLE mole.dataface__failed_logins  ENGINE=InnoDB;
ALTER TABLE mole.dataface__mtimes  ENGINE=InnoDB;
ALTER TABLE mole.dataface__preferences  ENGINE=InnoDB;
ALTER TABLE mole.dataface__version  ENGINE=InnoDB;
ALTER TABLE mole.datafaceew_users  ENGINE=InnoDB;
ALTER TABLE mole.err_sp  ENGINE=InnoDB;
ALTER TABLE mole.ew_arc_phase  ENGINE=InnoDB,  DROP KEY `ewmodid`,  DROP KEY `scnlid`,  DROP KEY `ewsqkid`;
ALTER TABLE mole.ew_arc_summary  ENGINE=InnoDB,  DROP KEY `ewmodid_2`,  DROP KEY `ewsqkid`;
ALTER TABLE mole.ew_error  ENGINE=InnoDB,  DROP KEY `ewmodid`;
ALTER TABLE mole.ew_events_summary  ENGINE=InnoDB,  DROP KEY `ewsqkid`;
ALTER TABLE mole.ew_instance  ENGINE=InnoDB;
ALTER TABLE mole.ew_link  ENGINE=InnoDB,  DROP KEY `ewmodid`,  DROP KEY `ewsqkid`,  DROP KEY `ewspkid`;
ALTER TABLE mole.ew_magnitude_phase  ENGINE=InnoDB,  DROP KEY `ewmodid`,  DROP KEY `scnlid`,  DROP KEY `ewsqkid`;
ALTER TABLE mole.ew_magnitude_summary  ENGINE=InnoDB,  DROP KEY `ewmodid_2`,  DROP KEY `ewsqkid`;
ALTER TABLE mole.ew_module  ENGINE=InnoDB,  DROP KEY `ewid_2`;
ALTER TABLE mole.ew_params_pick_sta  ENGINE=InnoDB,  DROP KEY `ewmodid`,  DROP KEY `scnlid_2`;
ALTER TABLE mole.ew_pick_scnl  ENGINE=InnoDB,  DROP KEY `ewmodid`,  DROP KEY `scnlid`,  DROP KEY `ewspkid`;
ALTER TABLE mole.ew_quake2k  ENGINE=InnoDB,  DROP KEY `ewmodid`,  DROP KEY `ewsqkid`;
ALTER TABLE mole.ew_region_geom  ENGINE=InnoDB;
ALTER TABLE mole.ew_region_kind  ENGINE=InnoDB;
ALTER TABLE mole.ew_scnl  ENGINE=InnoDB;
ALTER TABLE mole.ew_spkseq  ENGINE=InnoDB,  DROP KEY `ewid`;
ALTER TABLE mole.ew_sqkseq  ENGINE=InnoDB,  DROP KEY `ewid`;
ALTER TABLE mole.ew_strongmotionII  ENGINE=InnoDB,  DROP KEY `ewmodid`,  DROP KEY `scnlid`,  DROP KEY `ewsqkid`;
ALTER TABLE mole.ew_tracebuf  ENGINE=InnoDB,  DROP KEY `ewmodid`,  DROP KEY `scnlid`;
ALTER TABLE mole.ew_arc_phase  CHANGE `ewmodid`   `fk_module` bigint(20) NOT NULL COMMENT 'Foreign key to ew_module', ADD FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `scnlid`   `fk_scnl` bigint(20) NOT NULL COMMENT 'Foreign key to ew_scnl', ADD FOREIGN KEY (`fk_scnl`) REFERENCES ew_scnl(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `ewsqkid`   `fk_sqkseq` bigint(20) NOT NULL COMMENT 'Foreign key to ew_sqkseq', ADD FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_arc_summary  CHANGE `ewmodid`   `fk_module` bigint(20) NOT NULL COMMENT 'Foreign key to ew_module', ADD FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `ewsqkid`   `fk_sqkseq` bigint(20) NOT NULL COMMENT 'Foreign key to ew_sqkseq', ADD FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_error  CHANGE `ewmodid`   `fk_module` bigint(20) NOT NULL COMMENT 'Foreign key to ew_module', ADD FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_events_summary  CHANGE `ewsqkid`   `fk_sqkseq` bigint(20) NOT NULL COMMENT 'Foreign key to ew_sqkseq', ADD FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_link  CHANGE `ewmodid`   `fk_module` bigint(20) NOT NULL COMMENT 'Foreign key to ew_module', ADD FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `ewsqkid`   `fk_sqkseq` bigint(20) NOT NULL COMMENT 'Foreign key to ew_sqkseq', ADD FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `ewspkid`   `fk_spkseq` bigint(20) NOT NULL COMMENT 'Foreign key to ew_spkseq', ADD FOREIGN KEY (`fk_spkseq`) REFERENCES ew_spkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_magnitude_phase  CHANGE `ewmodid`   `fk_module` bigint(20) NOT NULL COMMENT 'Foreign key to ew_module', ADD FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `scnlid`   `fk_scnl` bigint(20) NOT NULL COMMENT 'Foreign key to ew_scnl', ADD FOREIGN KEY (`fk_scnl`) REFERENCES ew_scnl(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `ewsqkid`   `fk_sqkseq` bigint(20) NOT NULL COMMENT 'Foreign key to ew_sqkseq', ADD FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_magnitude_summary  CHANGE `ewmodid`   `fk_module` bigint(20) NOT NULL COMMENT 'Foreign key to ew_module', ADD FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `ewsqkid`   `fk_sqkseq` bigint(20) NOT NULL COMMENT 'Foreign key to ew_sqkseq', ADD FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_module  CHANGE `ewid`   `fk_instance` bigint(20) NOT NULL COMMENT 'Id of the EW instance running', ADD FOREIGN KEY (`fk_instance`) REFERENCES ew_instance(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_params_pick_sta  CHANGE `ewmodid`   `fk_module` bigint(20) NOT NULL COMMENT 'Foreign key to ew_module', ADD FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `scnlid`   `fk_scnl` bigint(20) NOT NULL COMMENT 'Foreign key to ew_scnl', ADD FOREIGN KEY (`fk_scnl`) REFERENCES ew_scnl(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_pick_scnl  CHANGE `ewmodid`   `fk_module` bigint(20) NOT NULL COMMENT 'Foreign key to ew_module', ADD FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `scnlid`   `fk_scnl` bigint(20) NOT NULL COMMENT 'Foreign key to ew_scnl', ADD FOREIGN KEY (`fk_scnl`) REFERENCES ew_scnl(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `ewspkid`   `fk_spkseq` bigint(20) NOT NULL COMMENT 'Foreign key to ew_spkseq', ADD FOREIGN KEY (`fk_spkseq`) REFERENCES ew_spkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_quake2k  CHANGE `ewmodid`   `fk_module` bigint(20) NOT NULL COMMENT 'Foreign key to ew_module', ADD FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `ewsqkid`   `fk_sqkseq` bigint(20) NOT NULL COMMENT 'Foreign key to ew_sqkseq', ADD FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_spkseq  CHANGE `ewid`   `fk_instance` bigint(20) NOT NULL COMMENT 'Id of the EW instance running', ADD FOREIGN KEY (`fk_instance`) REFERENCES ew_instance(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_sqkseq  CHANGE `ewid`   `fk_instance` bigint(20) NOT NULL COMMENT 'Id of the EW instance running', ADD FOREIGN KEY (`fk_instance`) REFERENCES ew_instance(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_strongmotionII  CHANGE `ewmodid`   `fk_module` bigint(20) NOT NULL COMMENT 'Foreign key to ew_module', ADD FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `scnlid`   `fk_scnl` bigint(20) NOT NULL COMMENT 'Foreign key to ew_scnl', ADD FOREIGN KEY (`fk_scnl`) REFERENCES ew_scnl(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `ewsqkid`   `fk_sqkseq` bigint(20) NOT NULL COMMENT 'Foreign key to ew_sqkseq', ADD FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE mole.ew_tracebuf  CHANGE `ewmodid`   `fk_module` bigint(20) NOT NULL COMMENT 'Foreign key to ew_module', ADD FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,  CHANGE `scnlid`   `fk_scnl` bigint(20) NOT NULL COMMENT 'Foreign key to ew_scnl', ADD FOREIGN KEY (`fk_scnl`) REFERENCES ew_scnl(`id`) ON DELETE CASCADE ON UPDATE CASCADE;

-- UNLOCK TABLES must be the LAST sql command 
UNLOCK TABLES;


-- TRIGGERS
DROP TRIGGER IF EXISTS tr_ew_arc_phase_a_i;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` TRIGGER `tr_ew_arc_phase_a_i`
AFTER INSERT ON `ew_arc_phase`
    FOR EACH ROW
    BEGIN
    DECLARE l_mag_ewmodid           BIGINT    DEFAULT NULL;
    DECLARE l_ewsqkid               BIGINT    DEFAULT NULL;
    DECLARE l_qkseq                 BIGINT    DEFAULT NULL;
    DECLARE l_version               BIGINT    DEFAULT NULL;
    DECLARE l_ewname                CHAR(80)  DEFAULT NULL;



    DECLARE l_writer_ph             VARCHAR(255)    DEFAULT NULL;
    DECLARE l_writer_ev             VARCHAR(255)    DEFAULT NULL;
    DECLARE l_seisev_is             TINYINT         DEFAULT 0;
    DECLARE l_net                   CHAR(2)         DEFAULT NULL;
    DECLARE l_sta                   VARCHAR(5)      DEFAULT NULL;
    DECLARE l_cha                   CHAR(3)         DEFAULT NULL;
    DECLARE l_loc                   CHAR(2)         DEFAULT NULL;





    SELECT ewname, qkseq INTO l_ewname,l_qkseq 
    FROM ew_instance, ew_sqkseq
    WHERE ew_sqkseq.fk_instance = ew_instance.id 
    AND ew_sqkseq.id = NEW.fk_sqkseq;
    
    SELECT CONCAT(IF(ISNULL(l_ewname), 'null', l_ewname), '#', IF(ISNULL(modname), 'null', modname)) INTO l_writer_ph
    FROM ew_module WHERE ew_module.id = NEW.fk_module;

    SELECT CONCAT(l_ewname, '#', m.modname) INTO l_writer_ev
    FROM ew_module m 
    JOIN ew_arc_summary e ON e.fk_module = m.id
    WHERE e.fk_sqkseq = NEW.fk_sqkseq 
    ORDER BY e.version DESC LIMIT 1;


    SELECT COUNT(*) INTO l_seisev_is 
    FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'seisev';



    IF l_seisev_is > 0 AND host_fill_db.`fn_fill_seisev`() = 1 THEN



        SELECT net, sta, cha, loc INTO l_net, l_sta, l_cha, l_loc
        FROM `mole`.`ew_scnl`
        WHERE id = NEW.fk_scnl;
        






        CALL seisev.`sp_ins_ph_ew`(l_qkseq, l_writer_ph, l_writer_ev, NEW.version, l_net, l_sta, l_cha, l_loc, NEW.Plabel, NEW.Slabel, NEW.Ponset, NEW.Sonset, NEW.Pat_dt, NEW.Pat_usec, NEW.Sat_dt, NEW.Sat_usec, NEW.Pres, NEW.Sres, NEW.Pqual, NEW.Squal, NEW.codalen, NEW.codawt, NEW.Pfm, NEW.Sfm, NEW.datasrc, NEW.Md, NEW.azm, NEW.takeoff, NEW.dist, NEW.Pwt, NEW.Swt, NEW.pamp, NEW.codalenObs, NEW.ccntr_0, NEW.ccntr_1, NEW.ccntr_2, NEW.ccntr_3, NEW.ccntr_4, NEW.ccntr_5, NEW.caav_0, NEW.caav_1, NEW.caav_2, NEW.caav_3, NEW.caav_4, NEW.caav_5
        );
    END IF;


END
$$
DELIMITER ;
DROP TRIGGER IF EXISTS tr_ew_events_summary_from_arc_sum;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` TRIGGER `tr_ew_events_summary_from_arc_sum`
AFTER INSERT ON `ew_arc_summary`
    FOR EACH ROW
    BEGIN
	DECLARE l_arc_ewmodid           BIGINT    DEFAULT NULL;
	DECLARE l_ewsqkid               BIGINT    DEFAULT NULL;
	DECLARE l_version               BIGINT    DEFAULT NULL;
	DECLARE l_ewname                CHAR(80)  DEFAULT NULL;
	DECLARE l_qkseq                 BIGINT    DEFAULT NULL;
	
	DECLARE l_writer                VARCHAR(255)    DEFAULT NULL;
	DECLARE l_seisev_is             TINYINT         DEFAULT 0;
	DECLARE l_moleslave             TINYINT         DEFAULT 0;
	
    
	
	SELECT ewname,qkseq INTO l_ewname,l_qkseq FROM ew_instance, ew_sqkseq
	WHERE ew_sqkseq.fk_instance = ew_instance.id 
	AND ew_sqkseq.id = NEW.fk_sqkseq;

	
	SELECT DISTINCT arc_ewmodid, fk_sqkseq,version  INTO l_arc_ewmodid, l_ewsqkid, l_version
	FROM `ew_events_summary`
	WHERE
	    arc_ewmodid = NEW.fk_module
	    AND fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version
	;

	
	
	IF l_arc_ewmodid = NEW.fk_module AND l_ewsqkid = NEW.fk_sqkseq AND l_version = NEW.version THEN
	UPDATE `ew_events_summary` 
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
	    arc_ewmodid = NEW.fk_module
	    AND fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version
	;
	ELSE
	INSERT INTO `ew_events_summary` 
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
	    arc_ewmodid = NEW.fk_module,
	    fk_sqkseq = NEW.fk_sqkseq,
	    qkseq = l_qkseq,           
	    region = `fn_find_region_name`(NEW.lat, NEW.lon),
	    version = NEW.version
	;
	END IF;


	SELECT CONCAT(IF(ISNULL(l_ewname), 'null', l_ewname), '#', IF(ISNULL(modname), 'null', modname)) INTO l_writer
	FROM ew_module WHERE id = NEW.fk_module;

	SELECT COUNT(*) INTO l_seisev_is 
	FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'seisev';
	
	

	IF l_seisev_is > 0 AND host_fill_db.`fn_fill_seisev`() = 1 THEN
	  
	  

	  CALL seisev.`sp_ins_ev_ew`(l_qkseq, l_writer, NEW.ot_dt, NEW.ot_usec, NEW.lat, NEW.lon,  NEW.z, NEW.nph, NEW.nphs, NEW.nphtot, NEW.nPfm, NEW.gap, NEW.dmin, NEW.rms, NEW.e0az, NEW.e0dp, NEW.e0, NEW.e1az, NEW.e1dp, NEW.e1, NEW.e2, NEW.erh, NEW.erz, NEW.version, NEW.quality);
	END IF;


    END
$$
DELIMITER ;
DROP TRIGGER IF EXISTS tr_ew_magnitude_phase_a_i;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` TRIGGER `tr_ew_magnitude_phase_a_i`
AFTER INSERT ON `ew_magnitude_phase`
    FOR EACH ROW
    BEGIN
    DECLARE l_mag_ewmodid           BIGINT    DEFAULT NULL;
    DECLARE l_ewsqkid               BIGINT    DEFAULT NULL;
    DECLARE l_qkseq                 BIGINT    DEFAULT NULL;
    DECLARE l_version               BIGINT    DEFAULT NULL;
    DECLARE l_ewname                CHAR(80)  DEFAULT NULL;



    DECLARE l_writer_amp            VARCHAR(255)    DEFAULT NULL;
    DECLARE l_writer_ev             VARCHAR(255)    DEFAULT NULL;
    DECLARE l_seisev_is             TINYINT         DEFAULT 0;
    DECLARE l_net                   CHAR(2)         DEFAULT NULL;
    DECLARE l_sta                   VARCHAR(5)      DEFAULT NULL;
    DECLARE l_cha                   CHAR(3)         DEFAULT NULL;
    DECLARE l_loc                   CHAR(2)         DEFAULT NULL;





    
    SELECT ewname, qkseq INTO l_ewname,l_qkseq 
    FROM ew_instance, ew_sqkseq
    WHERE ew_sqkseq.fk_instance = ew_instance.id 
    AND ew_sqkseq.id = NEW.fk_sqkseq;

    SELECT CONCAT(IF(ISNULL(l_ewname), 'null', l_ewname), '#', IF(ISNULL(modname), 'null', modname)) INTO l_writer_amp
    FROM ew_module WHERE ew_module.id = NEW.fk_module;

    SELECT CONCAT(l_ewname, '#', m.modname) INTO l_writer_ev
    FROM ew_module m 
    JOIN ew_arc_summary e ON e.fk_module = m.id
    WHERE e.fk_sqkseq = NEW.fk_sqkseq 
    ORDER BY e.version DESC LIMIT 1;


    SELECT COUNT(*) INTO l_seisev_is 
    FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'seisev';



    IF l_seisev_is > 0 AND host_fill_db.`fn_fill_seisev`() = 1 THEN



        SELECT net, sta, cha, loc INTO l_net, l_sta, l_cha, l_loc
        FROM `mole`.`ew_scnl`
        WHERE id = NEW.fk_scnl;
        







        CALL seisev.`sp_ins_amp_ew`(l_qkseq, l_writer_amp, l_writer_ev, NEW.version, l_net, l_sta, l_cha, l_loc, NEW.mag, NEW.dist, NEW.mag_correction, NEW.Time1_dt, NEW.Time1_usec, NEW.Amp1, NEW.Period1, NEW.Time2_dt, NEW.Time2_usec, NEW.Amp2, NEW.Period2
        );
    END IF;


END
$$
DELIMITER ;
DROP TRIGGER IF EXISTS tr_ew_events_summary_from_mag_sum;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` TRIGGER `tr_ew_events_summary_from_mag_sum`
AFTER INSERT ON `ew_magnitude_summary`
    FOR EACH ROW
    BEGIN
	DECLARE l_mag_ewmodid           BIGINT    DEFAULT NULL;
	DECLARE l_ewsqkid               BIGINT    DEFAULT NULL;
	DECLARE l_qkseq                 BIGINT    DEFAULT NULL;
	DECLARE l_version               BIGINT    DEFAULT NULL;

	DECLARE l_arc_ewmodid           BIGINT    DEFAULT NULL;
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

	
	DECLARE l_writer_mag            VARCHAR(255)    DEFAULT NULL;
	DECLARE l_writer_ev             VARCHAR(255)    DEFAULT NULL;
	DECLARE l_seisev_is             TINYINT         DEFAULT 0;
	DECLARE l_moleslave             TINYINT         DEFAULT 0;
	

	
	SELECT ewname,qkseq INTO l_ewname,l_qkseq FROM ew_instance, ew_sqkseq
	WHERE ew_sqkseq.fk_instance = ew_instance.id 
	AND ew_sqkseq.id = NEW.fk_sqkseq;

	
	SELECT DISTINCT mag_ewmodid, fk_sqkseq,version  INTO l_mag_ewmodid, l_ewsqkid, l_version
	FROM `ew_events_summary`
	WHERE
	    mag_ewmodid = NEW.fk_module
	    AND fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version
	;

	
	
	IF l_mag_ewmodid = NEW.fk_module AND l_ewsqkid = NEW.fk_sqkseq AND l_version = NEW.version THEN
	UPDATE `ew_events_summary` 
	    SET mag = NEW.mag,
	    mag_error = NEW.error,
	    mag_quality = NEW.quality,
	    mag_type = NEW.szmagtype,
	    nstations = NEW.nstations,
	    nchannels = NEW.nchannels,
	    ewname = l_ewname,
	    mag_modified = NEW.modified,
	    mag_ewmodid = NEW.fk_module
	WHERE
	    fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version
	;
	ELSE
	
	SELECT DISTINCT mag_ewmodid, fk_sqkseq,version  INTO l_mag_ewmodid, l_ewsqkid, l_version
	FROM `ew_events_summary`
	WHERE
	    fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version
	;
	
	
	IF l_mag_ewmodid IS NULL THEN
	    UPDATE `ew_events_summary` 
	    SET mag = NEW.mag,
	    mag_error = NEW.error,
	    mag_quality = NEW.quality,
	    mag_type = NEW.szmagtype,
	    nstations = NEW.nstations,
	    nchannels = NEW.nchannels,
	    ewname = l_ewname,
	    mag_modified = NEW.modified,
	    mag_ewmodid = NEW.fk_module
	    WHERE
	    fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version;
	ELSE
	

	SELECT arc_ewmodid, ot_dt, ot_usec, lat, lon, z, arc_quality, erh, erz, nphtot, gap, dmin, rms, region, arc_modified
	    INTO l_arc_ewmodid, l_ot_dt, l_ot_usec, l_lat, l_lon, l_z, l_arc_quality, l_erh, l_erz, l_nphtot, l_gap, l_dmin, l_rms, l_region, l_arc_modified
	FROM `ew_events_summary`
	WHERE
	    fk_sqkseq = NEW.fk_sqkseq
	    AND version = NEW.version;

	INSERT INTO `ew_events_summary` 
	    SET mag = NEW.mag,
	    mag_error = NEW.error,
	    mag_quality = NEW.quality,
	    mag_type = NEW.szmagtype,
	    nstations = NEW.nstations,
	    nchannels = NEW.nchannels,
	    ewname = l_ewname,
	    mag_modified = NEW.modified,
	    mag_ewmodid = NEW.fk_module,
	    fk_sqkseq = NEW.fk_sqkseq,
	    qkseq = l_qkseq,
	    version = NEW.version,
	    arc_ewmodid = l_arc_ewmodid ,
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


	SELECT CONCAT(IF(ISNULL(l_ewname), 'null', l_ewname), '#', IF(ISNULL(modname), 'null', modname)) INTO l_writer_mag
	FROM ew_module WHERE id = NEW.fk_module;

	SELECT CONCAT(l_ewname, '#', m.modname) INTO l_writer_ev
	FROM ew_module m 
	JOIN ew_arc_summary e ON e.fk_module = m.id
	WHERE e.fk_sqkseq = NEW.fk_sqkseq LIMIT 1;

	SELECT COUNT(*) INTO l_seisev_is 
	FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'seisev';

	IF l_seisev_is > 0 AND host_fill_db.`fn_fill_seisev`() = 1 THEN

	  
	  
	    
	      
	      
	      
	      
	      
	      
	      
	      
	      
	      
	      
	      
	    

	  CALL seisev.`sp_ins_mag_ew`(l_qkseq, l_writer_mag, l_writer_ev, NEW.version, NEW.mag, NEW.error, NEW.quality, NEW.mindist, NEW.azimuth, NEW.nstations, NEW.nchannels, NEW.mag_quality, CONCAT(NEW.szmagtype, '-', NEW.algorithm));
      END IF;



    END
$$
DELIMITER ;
DROP TRIGGER IF EXISTS tr_ew_strongmotionII_a_i;
DELIMITER $$
 CREATE DEFINER=`moleuser`@`%.int.ingv.it` TRIGGER `tr_ew_strongmotionII_a_i`
AFTER INSERT ON `ew_strongmotionII`
    FOR EACH ROW
    BEGIN
    DECLARE l_mag_ewmodid           BIGINT    DEFAULT NULL;
    DECLARE l_ewsqkid               BIGINT    DEFAULT NULL;
    DECLARE l_qkseq                 BIGINT    DEFAULT NULL;
    DECLARE l_version               BIGINT    DEFAULT NULL;
    DECLARE l_ewname                CHAR(80)  DEFAULT NULL;



    DECLARE l_writer_amp            VARCHAR(255)    DEFAULT NULL;
    DECLARE l_writer_ev             VARCHAR(255)    DEFAULT NULL;
    DECLARE l_seisev_is             TINYINT         DEFAULT 0;
    DECLARE l_net                   CHAR(2)         DEFAULT NULL;
    DECLARE l_sta                   VARCHAR(5)      DEFAULT NULL;
    DECLARE l_cha                   CHAR(3)         DEFAULT NULL;
    DECLARE l_loc                   CHAR(2)         DEFAULT NULL;





    
    SELECT ewname, qkseq INTO l_ewname,l_qkseq 
    FROM ew_instance, ew_sqkseq
    WHERE ew_sqkseq.fk_instance = ew_instance.id 
    AND ew_sqkseq.id = NEW.fk_sqkseq;

    SELECT CONCAT(IF(ISNULL(l_ewname), 'null', l_ewname), '#', IF(ISNULL(modname), 'null', modname)) INTO l_writer_amp
    FROM ew_module WHERE ew_module.id = NEW.fk_module;

    SELECT CONCAT(l_ewname, '#', m.modname) INTO l_writer_ev
    FROM ew_module m 
    JOIN ew_arc_summary e ON e.fk_module = m.id
    WHERE e.fk_sqkseq = NEW.fk_sqkseq 
    ORDER BY e.version DESC LIMIT 1;


    SELECT COUNT(*) INTO l_seisev_is 
    FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'seisev';



    IF l_seisev_is > 0 AND host_fill_db.`fn_fill_seisev`() = 1 THEN



        SELECT net, sta, cha, loc INTO l_net, l_sta, l_cha, l_loc
        FROM `mole`.`ew_scnl`
        WHERE id = NEW.fk_scnl;
    






        CALL seisev.`sp_ins_stm_ew`(l_qkseq, l_writer_amp, l_writer_ev, NEW.version, l_net, l_sta, l_cha, l_loc, NEW.t_dt, NEW.t_usec, NEW.t_alt_dt, NEW.t_alt_usec, NEW.altcode, NEW.pga, NEW.tpga_dt, NEW.tpga_usec, NEW.pgv, NEW.tpgv_dt, NEW.tpgv_usec, NEW.pgd, NEW.tpgd_dt, NEW.tpgd_usec, NEW.rsa
        );
    END IF;


END
$$
DELIMITER ;

-- VIEWS
DROP VIEW IF EXISTS dataface__view_ew_instance_21dbd5547d1928b4e911a42f4b720cac;
CREATE ALGORITHM=UNDEFINED DEFINER=`moleuser`@`%.int.ingv.it` SQL SECURITY DEFINER VIEW `dataface__view_ew_instance_21dbd5547d1928b4e911a42f4b720cac` AS select `i`.`id` AS `id`,`i`.`ewname` AS `ewname`,`i`.`hostname` AS `hostname`,`i`.`ewinstallation` AS `ewinstallation`,`i`.`modified` AS `modified`,count(`m`.`id`) AS `nummods` from (`ew_instance` `i` join `ew_module` `m` on((`m`.`fk_instance` = `i`.`id`))) group by `i`.`id`
;
DROP VIEW IF EXISTS dataface__view_ew_module_657a63b1d9ee640673773c3b0d10151c;
CREATE ALGORITHM=UNDEFINED DEFINER=`moleuser`@`%.int.ingv.it` SQL SECURITY DEFINER VIEW `dataface__view_ew_module_657a63b1d9ee640673773c3b0d10151c` AS select `ew_module`.`id` AS `id`,`ew_module`.`fk_instance` AS `fk_instance`,`ew_module`.`modname` AS `modname`,`ew_module`.`modified` AS `modified`,`ew_instance`.`ewname` AS `ewname` from (`ew_module` join `ew_instance`) where (`ew_module`.`fk_instance` = `ew_instance`.`id`)
;
DROP VIEW IF EXISTS dataface__view_ew_module_7fe9cea0e53997baf6285d4638f9b077;
CREATE ALGORITHM=UNDEFINED DEFINER=`moleuser`@`%.int.ingv.it` SQL SECURITY DEFINER VIEW `dataface__view_ew_module_7fe9cea0e53997baf6285d4638f9b077` AS select `ew_module`.`id` AS `id`,`ew_module`.`fk_instance` AS `fk_instance`,`ew_module`.`modname` AS `modname`,`ew_module`.`modified` AS `modified`,`ew_instance`.`ewname` AS `ewname` from (`ew_module` join `ew_instance` on((`ew_module`.`fk_instance` = `ew_instance`.`id`)))
;
DROP VIEW IF EXISTS dataface__view_ew_module_a2725f145496c20dd2a8b60c553e3ff1;
CREATE ALGORITHM=UNDEFINED DEFINER=`moleuser`@`%.int.ingv.it` SQL SECURITY DEFINER VIEW `dataface__view_ew_module_a2725f145496c20dd2a8b60c553e3ff1` AS select `m`.`id` AS `id`,`m`.`fk_instance` AS `fk_instance`,`m`.`modname` AS `modname`,`m`.`modified` AS `modified`,`i`.`ewname` AS `ewname` from (`ew_module` `m` join `ew_instance` `i` on((`m`.`fk_instance` = `i`.`id`)))
;
DROP VIEW IF EXISTS dataface__view_ew_module_c45d73fd3a6b11eb1a85ea99d79cc29b;
CREATE ALGORITHM=UNDEFINED DEFINER=`moleuser`@`%.int.ingv.it` SQL SECURITY DEFINER VIEW `dataface__view_ew_module_c45d73fd3a6b11eb1a85ea99d79cc29b` AS select `ew_module`.`id` AS `id`,`ew_module`.`fk_instance` AS `fk_instance`,`ew_module`.`modname` AS `modname`,`ew_module`.`modified` AS `modified` from `ew_module`
;
DROP VIEW IF EXISTS v_ew_error_message;
CREATE ALGORITHM=MERGE DEFINER=`moleuser`@`%.int.ingv.it` SQL SECURITY DEFINER VIEW `v_ew_error_message` AS select `ew_instance`.`ewname` AS `ewname`,`ew_module`.`modname` AS `modname`,`ew_error`.`time_dt` AS `time_dt`,`ew_error`.`message` AS `message` from ((`ew_error` join `ew_module` on((`ew_error`.`fk_module` = `ew_module`.`id`))) join `ew_instance` on((`ew_module`.`fk_instance` = `ew_instance`.`id`)))
;
DROP VIEW IF EXISTS v_ew_params_pick_id_last_revision;
CREATE ALGORITHM=UNDEFINED DEFINER=`moleuser`@`%.int.ingv.it` SQL SECURITY DEFINER VIEW `v_ew_params_pick_id_last_revision` AS select `ew_params_pick_sta`.`fk_module` AS `fk_module`,`ew_params_pick_sta`.`fk_scnl` AS `fk_scnl`,max(`ew_params_pick_sta`.`revision`) AS `last_revision` from `ew_params_pick_sta` group by `ew_params_pick_sta`.`fk_module`,`ew_params_pick_sta`.`fk_scnl` order by `ew_params_pick_sta`.`fk_module`,`ew_params_pick_sta`.`fk_scnl`
;
DROP VIEW IF EXISTS v_ew_params_pick_sta_last_revision;
CREATE ALGORITHM=MERGE DEFINER=`moleuser`@`%.int.ingv.it` SQL SECURITY DEFINER VIEW `v_ew_params_pick_sta_last_revision` AS select `ew_scnl`.`sta` AS `sta`,`ew_scnl`.`cha` AS `cha`,`ew_scnl`.`net` AS `net`,`ew_scnl`.`loc` AS `loc`,`a`.`fk_module` AS `fk_module`,`a`.`fk_scnl` AS `fk_scnl`,`a`.`revision` AS `revision`,`a`.`pick_flag` AS `pick_flag`,`a`.`pin_numb` AS `pin_numb`,`a`.`itr1` AS `itr1`,`a`.`minsmallzc` AS `minsmallzc`,`a`.`minbigzc` AS `minbigzc`,`a`.`minpeaksize` AS `minpeaksize`,`a`.`maxmint` AS `maxmint`,`a`.`i9` AS `i9`,`a`.`rawdatafilt` AS `rawdatafilt`,`a`.`charfuncfilt` AS `charfuncfilt`,`a`.`stafilt` AS `stafilt`,`a`.`ltafilt` AS `ltafilt`,`a`.`eventthresh` AS `eventthresh`,`a`.`rmavfilt` AS `rmavfilt`,`a`.`deadsta` AS `deadsta`,`a`.`codaterm` AS `codaterm`,`a`.`altcoda` AS `altcoda`,`a`.`preevent` AS `preevent`,`a`.`erefs` AS `erefs`,`a`.`clipcount` AS `clipcount`,`a`.`ext_id` AS `ext_id`,`a`.`modified` AS `modified` from ((`ew_params_pick_sta` `a` join `v_ew_params_pick_id_last_revision` `b` on(((`a`.`fk_module` = `b`.`fk_module`) and (`a`.`fk_scnl` = `b`.`fk_scnl`) and (`a`.`revision` = `b`.`last_revision`)))) join `ew_scnl` on((`ew_scnl`.`id` = `a`.`fk_scnl`)))
;
DROP VIEW IF EXISTS v_ew_picks;
CREATE ALGORITHM=MERGE DEFINER=`moleuser`@`%.int.ingv.it` SQL SECURITY DEFINER VIEW `v_ew_picks` AS select `ew_scnl`.`sta` AS `sta`,`ew_scnl`.`cha` AS `cha`,`ew_scnl`.`net` AS `net`,`ew_scnl`.`loc` AS `loc`,`fn_get_str_from_dt_usec`(`ew_pick_scnl`.`tpick_dt`,`ew_pick_scnl`.`tpick_usec`) AS `pick_time`,`ew_pick_scnl`.`dir` AS `dir`,`ew_pick_scnl`.`wt` AS `wt`,`ew_pick_scnl`.`pamp_0` AS `pamp_0`,`ew_pick_scnl`.`pamp_1` AS `pamp_1`,`ew_pick_scnl`.`pamp_2` AS `pamp_2`,`ew_instance`.`ewname` AS `ewname`,`ew_module`.`modname` AS `modname`,`ew_spkseq`.`pkseq` AS `pkseq`,`ew_pick_scnl`.`modified` AS `modified` from ((((`ew_pick_scnl` join `ew_module` on((`ew_pick_scnl`.`fk_module` = `ew_module`.`id`))) join `ew_instance` on((`ew_module`.`fk_instance` = `ew_instance`.`id`))) join `ew_scnl` on((`ew_pick_scnl`.`fk_scnl` = `ew_scnl`.`id`))) join `ew_spkseq` on((`ew_pick_scnl`.`fk_spkseq` = `ew_spkseq`.`id`)))
;
DROP VIEW IF EXISTS v_ew_quake_evolution;
CREATE ALGORITHM=UNDEFINED DEFINER=`moleuser`@`%.int.ingv.it` SQL SECURITY DEFINER VIEW `v_ew_quake_evolution` AS select distinct `ew_sqkseq`.`id` AS `id`,`inst_bind`.`ewname` AS `ewname`,`ew_sqkseq`.`qkseq` AS `qkseq`,`ew_arc_summary`.`version` AS `arc_ver`,`ew_magnitude_summary`.`version` AS `mag_ver`,`ew_modmag`.`modname` AS `modmag`,`ew_magnitude_summary`.`mag` AS `mag`,`ew_modarc`.`modname` AS `modarc`,`fn_get_str_from_dt_usec`(`ew_arc_summary`.`ot_dt`,`ew_arc_summary`.`ot_usec`) AS `ot_time`,`ew_arc_summary`.`quality` AS `quality`,round(`ew_arc_summary`.`lat`,2) AS `lat`,round(`ew_arc_summary`.`lon`,2) AS `lon`,round(`ew_arc_summary`.`z`,2) AS `depth`,timediff(`ew_arc_summary`.`modified`,`ew_arc_summary`.`ot_dt`) AS `diff_ot_loc`,timediff(`ew_magnitude_summary`.`modified`,`ew_arc_summary`.`ot_dt`) AS `diff_ot_mag`,`ew_modbind`.`modname` AS `modbind` from (((((((`ew_sqkseq` left join `ew_quake2k` on((`ew_sqkseq`.`id` = `ew_quake2k`.`fk_sqkseq`))) left join `ew_module` `ew_modbind` on((`ew_quake2k`.`fk_module` = `ew_modbind`.`id`))) join `ew_instance` `inst_bind` on((`ew_sqkseq`.`fk_instance` = `inst_bind`.`id`))) left join `ew_magnitude_summary` on((`ew_sqkseq`.`id` = `ew_magnitude_summary`.`fk_sqkseq`))) left join `ew_arc_summary` on((`ew_sqkseq`.`id` = `ew_arc_summary`.`fk_sqkseq`))) left join `ew_module` `ew_modmag` on((`ew_magnitude_summary`.`fk_module` = `ew_modmag`.`id`))) left join `ew_module` `ew_modarc` on((`ew_arc_summary`.`fk_module` = `ew_modarc`.`id`))) where if(((`ew_magnitude_summary`.`id` is not null) and (`ew_arc_summary`.`id` is not null)),(`ew_arc_summary`.`version` = `ew_magnitude_summary`.`version`),1)
;
DROP VIEW IF EXISTS vw_cmplocations;
CREATE ALGORITHM=UNDEFINED DEFINER=`moleuser`@`%.int.ingv.it` SQL SECURITY DEFINER VIEW `vw_cmplocations` AS select `e`.`fk_sqkseq` AS `fk_sqkseq`,`e`.`arc_quality` AS `arc_quality`,`e`.`ot_dt` AS `ot_dt`,`e`.`lat` AS `lat`,`e`.`lon` AS `lon`,`e`.`z` AS `z`,`e`.`mag_type` AS `mag_type`,`e`.`mag` AS `mag`,`e`.`region` AS `region`,`e`.`version` AS `version`,`m`.`modname` AS `modarc`,`k`.`modname` AS `modmag`,`e`.`ewname` AS `ewname`,`e`.`qkseq` AS `qkseq`,sec_to_time((((to_days(cast(`e`.`arc_modified` as date)) - to_days(cast(`e`.`ot_dt` as date))) + time_to_sec(cast(`e`.`arc_modified` as time))) - time_to_sec(cast(`e`.`ot_dt` as time)))) AS `diff_ot_loc`,sec_to_time((((to_days(cast(`e`.`mag_modified` as date)) - to_days(cast(`e`.`ot_dt` as date))) + time_to_sec(cast(`e`.`mag_modified` as time))) - time_to_sec(cast(`e`.`ot_dt` as time)))) AS `diff_ot_mag` from ((`ew_events_summary` `e` join `ew_module` `m` on((`e`.`arc_ewmodid` = `m`.`id`))) join `ew_module` `k` on((`e`.`mag_ewmodid` = `k`.`id`)))
;
