
DROP VIEW IF EXISTS `v_ew_params_picks_vs_phases`;
CREATE ALGORITHM = MERGE VIEW `v_ew_params_picks_vs_phases` AS
SELECT i.ewname, p.starttime, p.endtime, s.sta, s.cha, s.net, s.loc, mp.modname as pmodname, p.npicks, ml.modname as lmodname, p.nphases, p.ndiff
FROM ew_params_picks_vs_phases p
JOIN ew_instance i ON (p.fk_instance = i.id)
JOIN ew_scnl s ON (p.fk_scnl = s.id)
JOIN ew_module mp ON (p.fk_modulepick = mp.id)
JOIN ew_module ml ON (p.fk_moduleloc = ml.id);

