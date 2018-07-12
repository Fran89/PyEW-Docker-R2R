
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_heartbeat`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_heartbeat`(
in_ewname                VARCHAR(80) /* Host running the EW Module */,
in_modname               VARCHAR(32) /* Module name */,
in_last_dt               VARCHAR(30) /* Time of earthworm heartbeat message */,
in_pid                   BIGINT      /* PID */
)
uscita: BEGIN
DECLARE l_ewname         VARCHAR(80);
DECLARE l_modname        VARCHAR(32);
DECLARE l_last_dt        DATETIME;
DECLARE l_pid            BIGINT;

DECLARE l_fk_module      BIGINT;

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

IF TRIM(in_last_dt)='' THEN 
        SELECT 'ERROR: time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_last_dt=SUBSTR(in_last_dt, 1, 19);
SET l_pid=in_pid;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);

INSERT INTO `ew_heartbeat` (fk_module,last_dt,pid)
VALUES (l_fk_module, l_last_dt, l_pid)
  ON DUPLICATE KEY UPDATE last_dt = l_last_dt, pid = l_pid;

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

