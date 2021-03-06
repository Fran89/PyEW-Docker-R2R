
DELIMITER $$
DROP FUNCTION IF EXISTS `fn_isdate`$$
CREATE  DEFINER = CURRENT_USER  FUNCTION `fn_isdate`(in_str VARCHAR(255)) RETURNS tinyint 
DETERMINISTIC
BEGIN
IF LENGTH(DATE(in_str)) IS NOT NULL AND LENGTH(TIME(in_str)) IS NOT NULL THEN
    RETURN 0;
ELSE
    RETURN 1;
END IF;
END$$
DELIMITER ;

