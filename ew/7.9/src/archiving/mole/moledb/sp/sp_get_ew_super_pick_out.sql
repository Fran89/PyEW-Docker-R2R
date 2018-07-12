
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_get_ew_super_pick_out`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_get_ew_super_pick_out`(
in_ewname                VARCHAR(80)  /* Name of the EW instance running */,
in_modname               VARCHAR(80)  /* Name of the EW module */,
in_pick_id               BIGINT       /* Pick sequence number from EW module */,
OUT l_id                 BIGINT
)
uscita: BEGIN
DECLARE l_ewname         VARCHAR(80);
DECLARE l_modname        VARCHAR(80);
DECLARE l_pick_id        BIGINT;
DECLARE l_fk_module      BIGINT;

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

IF TRIM(in_pick_id)='' THEN 
        SELECT 'ERROR: Pick sequence number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_pick_id=in_pick_id;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);

SELECT id INTO l_id FROM `ew_super_pick` WHERE fk_module=l_fk_module AND pick_id=l_pick_id;

IF l_id = -1 THEN
    INSERT INTO `ew_super_pick`(fk_module,pick_id) VALUES (l_fk_module,l_pick_id);
    SELECT LAST_INSERT_ID() INTO l_id;
END IF;

-- SELECT l_id;

END$$
DELIMITER ;

