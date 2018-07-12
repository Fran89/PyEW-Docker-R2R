
DROP VIEW IF EXISTS `v_ew_params_pick_id_last_revision`;
CREATE ALGORITHM = MERGE VIEW `v_ew_params_pick_id_last_revision` AS
SELECT fk_module, fk_scnl, MAX(revision) as last_revision
FROM ew_params_pick_sta
GROUP BY fk_module, fk_scnl
ORDER BY fk_module, fk_scnl;

