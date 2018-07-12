
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ew_instance` $$
CREATE PROCEDURE  `sp_ew_instance`(
)
BEGIN
    SELECT * FROM ew_instance;
END $$
DELIMITER ;

