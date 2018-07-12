
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ew_show_hosts` $$
CREATE PROCEDURE  `sp_ew_show_hosts`(
)
BEGIN
Select distinct hostname From `ew_instance` Where hostname is not  NULL;

END $$
DELIMITER ;

