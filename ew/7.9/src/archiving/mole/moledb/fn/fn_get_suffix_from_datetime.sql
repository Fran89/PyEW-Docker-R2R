
DELIMITER $$
DROP FUNCTION IF EXISTS `fn_get_suffix_from_datetime`$$
CREATE  DEFINER = CURRENT_USER  FUNCTION `fn_get_suffix_from_datetime`(
in_dt              DATETIME             /* datetime from which extract prefix */
) RETURNS CHAR(7) DETERMINISTIC
BEGIN
-- Example of output "2010_01"

RETURN CONCAT(SUBSTRING(in_dt, 1, 4), '_', SUBSTRING(in_dt, 6, 2));

END$$
DELIMITER ;

