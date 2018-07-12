
DELIMITER $$
DROP FUNCTION IF EXISTS `fn_get_moledb_version`$$
CREATE  DEFINER = CURRENT_USER  FUNCTION `fn_get_moledb_version`(
) RETURNS VARCHAR(255) NOT DETERMINISTIC
BEGIN

  DECLARE RET    VARCHAR(255) DEFAULT '';

  SELECT version INTO RET FROM version;

  RETURN RET;

END$$
DELIMITER ;
