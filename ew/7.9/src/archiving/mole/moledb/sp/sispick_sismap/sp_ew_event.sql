
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ew_event` $$
CREATE PROCEDURE  `sp_ew_event`(
in_start_time       VARCHAR(50),
in_end_time         VARCHAR(50)
)
-- call sp_ew_event('hew1_mole','2010-04-08',now());
uscita: BEGIN
    
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
    `e`.`ewname` AS `instance`,
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
    ORDER BY `e`.`fk_sqkseq` DESC, `e`.`version` DESC, `p`.`dist` ASC;
END $$
DELIMITER ;

