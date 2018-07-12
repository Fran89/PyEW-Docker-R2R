
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ew_event_instance` $$
CREATE PROCEDURE  `sp_ew_event_instance`(
in_instance         VARCHAR(19),
in_start_time       VARCHAR(19),
in_end_time         VARCHAR(19)
)
-- call sp_ew_event_instance('endeavour','2010-01-01',now());
uscita: BEGIN
    DECLARE l_id_instance        BIGINT DEFAULT 0;
    
    SELECT id INTO l_id_instance
    FROM `ew_instance` 
    WHERE ewname = in_instance;
    IF l_id_instance = 0 THEN
        SELECT'ERROR: wrong istance name';
        LEAVE uscita;
    END IF;
    
    IF fn_isdate(in_start_time) THEN
        SELECT'ERROR: start_time not datatime format yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;
    IF fn_isdate(in_end_time) THEN
        SELECT'ERROR: end_time not datatime format  yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;
    SELECT CAST(CONCAT(`e`.`fk_sqkseq`, '_', `e`.`version`) AS CHAR(50)) AS `sqkid_version`,
    `e`.`fk_sqkseq` AS `sqkid`,
    `e`.`qkseq` AS `qkseq`,
    CAST(CONCAT(`e`.`ot_dt`, '.', LEFT(`e`.`ot_usec`,4)) AS CHAR(50)) AS `ot_time`,
    `e`.`mag` AS `mag`,
    `e`.`mag_type` AS `mag_type`,
    `e`.`mag_error` AS `mag_error`,
    `e`.`arc_quality` AS `quality`,
    ROUND(`e`.`lat`,3) AS `lat`,
    ROUND(`e`.`lon`,3) AS `lon`,
    ROUND(`e`.`z`,3) AS `depth`,
    `e`.`erh` AS `error_h`,
    `e`.`erz` AS `error_z`,
    `e`.`nphtot` AS `nphtot`,
    `e`.`gap` AS `gap`,
    `e`.`dmin` AS `dmin`,
    `e`.`rms` AS `rms`,
    `e`.`ewname` AS `ewname`,
    `e`.`mag_fk_module` AS `modmag`,
    `e`.`region` AS `region_name`,
    `e`.`version` AS `version`,
    `e`.`modified` AS `modified`,
    `s`.`net`,
    `s`.`sta`,
    `p`.`dist`,
    `p`.`Pwt`
    FROM `ew_hypocenters_summary` e
    JOIN `ew_arc_phase` p ON `p`.`fk_sqkseq` = `e`.`fk_sqkseq` AND `p`.`version` = `e`.`version`
    JOIN `ew_scnl` s ON `p`.`fk_scnl` = `s`.`id`
    WHERE `e`.`ot_dt` >= in_start_time 
        AND `e`.`ot_dt` <= in_end_time
        AND `e`.`ewname` = in_instance
    ORDER BY `e`.`fk_sqkseq` DESC, `e`.`version` DESC; -- , `p`.`dist` ASC;
END $$
DELIMITER ;

