
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_get_ew_instance_out`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_get_ew_instance_out`(
in_ewname                VARCHAR(80)  /* Name of the EW instance running */,
OUT l_id                 BIGINT
)
uscita: BEGIN
DECLARE l_ewname         VARCHAR(80);
DECLARE l_hostname       VARCHAR(80);

SET l_id = -1;
SET l_hostname = '';

IF TRIM(in_ewname)='' THEN 
        SELECT 'ERROR: Ew instance name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_ewname=in_ewname;

SELECT id, hostname INTO l_id, l_hostname FROM `ew_instance` WHERE ewname=l_ewname;

IF l_id = -1 THEN
    INSERT INTO `ew_instance`(ewname, hostname) VALUES (l_ewname, fn_get_client_hostname());
    SELECT LAST_INSERT_ID() INTO l_id;
ELSE
    IF l_hostname IS NULL THEN
	UPDATE `ew_instance` SET hostname = fn_get_client_hostname() WHERE id = l_id;
    END IF;
END IF;

-- SELECT l_id;

END$$
DELIMITER ;

