
DELIMITER $$
DROP FUNCTION IF EXISTS `fn_get_str_from_dt_usec`$$
CREATE  DEFINER = CURRENT_USER  FUNCTION `fn_get_str_from_dt_usec`(
in_dt              DATETIME             /* lastnhour to retrieve */,
in_usec            BIGINT
) RETURNS CHAR(24) DETERMINISTIC
BEGIN
-- Example of output "2010-01-21 16:20:47.6900"

RETURN CONCAT(CAST(in_dt AS CHAR),  SUBSTR((in_usec/1000000.0), 2) );

END$$
DELIMITER ;

