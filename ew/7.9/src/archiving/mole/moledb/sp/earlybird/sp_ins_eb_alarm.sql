
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_eb_alarm`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_eb_alarm`(
  in_ewname       VARCHAR(80)  /* Host running the EW Module*/,
  in_modname      VARCHAR(32)  /* Module name */,

  in_message      VARCHAR(512) /* Alarm text message */

)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname     VARCHAR(32);
DECLARE l_fk_module   BIGINT;

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

INSERT INTO `eb_alarm`(
  fk_module,
  message)
VALUES (
  l_fk_module,
  in_message
);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

