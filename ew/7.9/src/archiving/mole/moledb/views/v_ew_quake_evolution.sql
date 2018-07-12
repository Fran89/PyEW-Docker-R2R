
DROP VIEW IF EXISTS `v_ew_quake_evolution`;
CREATE ALGORITHM = MERGE VIEW `v_ew_quake_evolution` AS
SELECT DISTINCT
    ew_sqkseq.id,
    inst_bind.ewname,
    ew_sqkseq.qkseq,
    ew_arc_summary.version AS arc_ver,
    ew_magnitude_summary.version AS mag_ver,
    ew_modmag.modname AS modmag,
    ew_magnitude_summary.mag,
    ew_modarc.modname AS modarc,
    fn_get_str_from_dt_usec(ew_arc_summary.ot_dt, ew_arc_summary.ot_usec) AS ot_time,
    ew_arc_summary.quality,
    ROUND(ew_arc_summary.lat, 2) AS lat,
    ROUND(ew_arc_summary.lon, 2) AS lon,
    ROUND(ew_arc_summary.z, 2) AS depth,
    TIMEDIFF(ew_arc_summary.modified, ew_arc_summary.ot_dt) as diff_ot_loc,
    TIMEDIFF(ew_magnitude_summary.modified, ew_arc_summary.ot_dt) as diff_ot_mag,
    ew_modbind.modname AS modbind
FROM
    ew_sqkseq
LEFT OUTER JOIN ew_quake2k
ON
    (
        ew_sqkseq.id = ew_quake2k.fk_sqkseq
    )
LEFT OUTER JOIN ew_module ew_modbind
ON
    (
        ew_quake2k.fk_module = ew_modbind.id
    )
INNER JOIN ew_instance inst_bind
ON
    (
        ew_sqkseq.fk_instance = inst_bind.id
    )
LEFT OUTER JOIN ew_magnitude_summary
ON
    (
        ew_sqkseq.id = ew_magnitude_summary.fk_sqkseq
    )
LEFT OUTER JOIN ew_arc_summary
ON
    (
        ew_sqkseq.id = ew_arc_summary.fk_sqkseq
    )
LEFT OUTER JOIN ew_module ew_modmag
ON
    (
        ew_magnitude_summary.fk_module = ew_modmag.id
    )
LEFT OUTER JOIN ew_module ew_modarc
ON
    (
        ew_arc_summary.fk_module = ew_modarc.id
    ) 
WHERE
IF(ew_magnitude_summary.id IS NOT NULL AND ew_arc_summary.id IS NOT NULL,
    ew_arc_summary.version = ew_magnitude_summary.version,
TRUE );
-- ORDER BY ew_sqkseq.modified ASC;

