
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_get_ew_module_out`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_get_ew_module_out`(
in_ewname                VARCHAR(80)  /* Name of the EW instance running */,
in_modname               VARCHAR(32)  /* EW module name*/,
OUT l_id                 BIGINT
)
uscita: BEGIN
DECLARE l_ewname         VARCHAR(80);
DECLARE l_modname        VARCHAR(32);
DECLARE l_fk_instance           BIGINT;

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

CALL `sp_get_ew_instance_out`(l_ewname, l_fk_instance);

SELECT id INTO l_id FROM `ew_module` WHERE fk_instance=l_fk_instance AND modname=l_modname;

IF l_id = -1 THEN
    INSERT INTO `ew_module`(fk_instance,modname) VALUES (l_fk_instance,l_modname);
    SELECT LAST_INSERT_ID() INTO l_id;
END IF;

-- SELECT l_id;

END$$
DELIMITER ;

