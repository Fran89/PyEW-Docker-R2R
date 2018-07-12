
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_get_ew_super_hypocenter_out`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_get_ew_super_hypocenter_out`(
in_ewname                VARCHAR(80)  /* Name of the EW instance running */,
in_modname               VARCHAR(80)  /* Name of the EW module */,
in_hypo_id               BIGINT       /* Hypocenter sequence number from EW module */,
OUT l_id                 BIGINT
)
uscita: BEGIN
DECLARE l_ewname         VARCHAR(80);
DECLARE l_modname        VARCHAR(80);
DECLARE l_hypo_id        BIGINT;
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

IF TRIM(in_hypo_id)='' THEN 
        SELECT 'ERROR: Quake sequence number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_hypo_id=in_hypo_id;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);

SELECT id INTO l_id FROM `ew_super_hypocenter` WHERE fk_module=l_fk_module AND hypo_id=l_hypo_id;

IF l_id = -1 THEN
    INSERT INTO `ew_super_hypocenter`(fk_module,hypo_id) VALUES (l_fk_module,l_hypo_id);
    SELECT LAST_INSERT_ID() INTO l_id;
END IF;

-- SELECT l_id;

END$$
DELIMITER ;

