
DELIMITER $$
DROP FUNCTION IF EXISTS `fn_get_tbname`$$
CREATE  DEFINER = CURRENT_USER  FUNCTION `fn_get_tbname`(
in_dbname                VARCHAR(80)  /* global name of database before the suffix */,
in_dt                    DATETIME     /* datetime from which extract prefix */,
in_tbname                VARCHAR(80)  /* table name */
) RETURNS VARCHAR(255) DETERMINISTIC
BEGIN
-- Example of output "`mole_2010_01`.`ew_arc_summary`"

RETURN CONCAT('`', in_dbname, '_', fn_get_suffix_from_datetime(in_dt), '`.`', in_tbname, '`');
-- RETURN CONCAT('`', in_dbname, '`.`', in_tbname, '`');

END$$
DELIMITER ;

