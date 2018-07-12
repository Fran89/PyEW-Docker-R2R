
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_error`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_error`(
in_ewname                VARCHAR(80) /* Host running the EW Module */,
in_modname               VARCHAR(32) /* Module name */,
in_time_dt               VARCHAR(30) /* Time of earthworm error message*/,
in_message               VARCHAR(256) /* Earthworm error message */
)
uscita: BEGIN
DECLARE l_ewname                VARCHAR(80);
DECLARE l_modname               VARCHAR(32);
DECLARE l_time_dt               DATETIME;
DECLARE l_message               VARCHAR(256);

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

IF TRIM(in_time_dt)='' THEN 
        SELECT 'ERROR: time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_time_dt=SUBSTR(in_time_dt, 1, 19);

SET l_message=in_message;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);

INSERT INTO `ew_error` (fk_module,time_dt,message)
VALUES (l_fk_module, l_time_dt, l_message);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

