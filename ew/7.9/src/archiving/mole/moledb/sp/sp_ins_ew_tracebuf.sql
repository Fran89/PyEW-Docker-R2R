
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_tracebuf`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_tracebuf`(
in_ewname                VARCHAR(80) /* Host running the EW Module */,
in_modname               VARCHAR(32) /* Module name */,
in_pinno                 BIGINT      /* Pin number */,
in_nsamp                 BIGINT      /* Number of samples in packet */,
in_starttime             VARCHAR(30) /* time of first sample - string in datetime format with useconds */,
in_endtime               VARCHAR(30) /* time of last sample  - string in datetime format with useconds*/,
in_samprate              DOUBLE      /* Sample rate; nominal */,
in_net                   VARCHAR(2)  /* Net code */,
in_sta                   VARCHAR(5)  /* Station code */,
in_cha                   VARCHAR(3)  /* Channel/Component code */,
in_loc                   VARCHAR(2)  /* Location code */,
in_version               VARCHAR(10) /* numeric version fields (99-99) */,
in_datatype              VARCHAR(3)  /* Data format code */,
in_quality               VARCHAR(10) /* Data-quality fields (99-99) */
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

DECLARE l_fk_module               BIGINT;
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

CALL `sp_get_ew_scnl_out`(l_sta, l_cha, l_net, l_loc, l_fk_scnl);
CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);

INSERT INTO `ew_tracebuf`(fk_module,pinno,nsamp,starttime_dt,starttime_usec,endtime_dt,endtime_usec,samprate,fk_scnl,version,datatype,quality)
    VALUES (l_fk_module,l_pinno,l_nsamp,l_starttime_dt,l_starttime_usec,l_endtime_dt,l_endtime_usec,l_samprate,l_fk_scnl,l_version,l_datatype,l_quality);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

