
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ew_last_event_id` $$
CREATE PROCEDURE  `sp_ew_last_event_id`(
)
BEGIN
SELECT MAX(id) FROM `ew_hypocenters_summary`;

END $$
DELIMITER ;

