
DROP VIEW IF EXISTS `v_ew_error_message`;
CREATE ALGORITHM = MERGE VIEW `v_ew_error_message` AS
SELECT ew_instance.ewname, ew_module.modname, ew_error.time_dt, ew_error.message FROM ew_error
JOIN ew_module ON ew_error.fk_module = ew_module.id
JOIN ew_instance ON ew_module.fk_instance = ew_instance.id;

