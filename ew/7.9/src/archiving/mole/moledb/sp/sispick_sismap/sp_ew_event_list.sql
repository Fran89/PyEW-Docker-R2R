
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ew_event_list` $$
CREATE PROCEDURE  `sp_ew_event_list`(
in_start_time  VARCHAR(19),
in_end_time    VARCHAR(19)
)
uscita: BEGIN
    IF fn_isdate(in_start_time) THEN
        SELECT'ERROR: start_time not datatime format yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;
    IF fn_isdate(in_end_time) THEN
        SELECT'ERROR: end_time not datatime format  yyy-mm-dd HH:mm:ss';
        LEAVE uscita;
    END IF;
    SELECT `ew_arc_summary`.`id` AS `db_id`,
    `ew_sqkseq`.`qkseq` AS `ew_id`,
     CONCAT(`ew_arc_summary`.`ot_dt`, '.', LEFT(`ew_arc_summary`.`ot_usec`,4)) AS `ot_time`,
    `ew_magnitude_summary`.`mag` AS `mag`,
    `ew_magnitude_summary`.`szmagtype` AS `mag_type`,
    `ew_magnitude_summary`.`error` AS `mag_error`,
    `ew_arc_summary`.`quality` AS `quality`,
    ROUND(`ew_arc_summary`.`lat`,3) AS `lat`,
    ROUND(`ew_arc_summary`.`lon`,3) AS `lon`,
    ROUND(`ew_arc_summary`.`z`,3) AS `depth`,
    `ew_arc_summary`.`erh` AS `error_h`,
    `ew_arc_summary`.`erz` AS `error_z`,
    `ew_arc_summary`.`nphtot` AS `nphtot`,
    `ew_arc_summary`.`gap` AS `gap`,
    `ew_arc_summary`.`dmin` AS `dmin`,
    `ew_arc_summary`.`rms` AS `rms`,
    `ew_instance`.`ewname` AS `ewname`,
    `ew_instance`.`hostname` AS `hostname`,
    `modmag`.`modname` AS `modmag`,
--    `ew_module`.`modname` AS `modarc`,
--    `ew_sqkseq`.`qkseq` AS `qkseq`,
    `ew_arc_summary`.`version` AS `version`,
    `ew_arc_summary`.`modified` AS `modified`,
    GROUP_CONCAT(DISTINCT CONCAT(`ew_scnl`.`sta`, '_', `ew_arc_phase`.`dist`, '_', `ew_arc_phase`.`Pwt`) SEPARATOR ';') AS `station_list`
    FROM `ew_arc_summary`
    JOIN `ew_module` ON `ew_arc_summary`.`fk_module` = `ew_module`.`id`
    JOIN `ew_instance` ON `ew_module`.`fk_instance` = `ew_instance`.`id`
    JOIN `ew_sqkseq` ON `ew_arc_summary`.`fk_sqkseq` = `ew_sqkseq`.`id`
    LEFT JOIN `ew_magnitude_summary` ON `ew_arc_summary`.`fk_sqkseq` = `ew_magnitude_summary`.`fk_sqkseq`
        AND `ew_arc_summary`.`version` = `ew_magnitude_summary`.`version`
    LEFT JOIN `ew_module` `modmag` ON IF(ISNULL(`ew_magnitude_summary`.`mag`),0,`ew_magnitude_summary`.`fk_module` = `modmag`.`id`)
    LEFT JOIN `ew_arc_phase` ON `ew_arc_phase`.`fk_sqkseq` = `ew_arc_summary`.`fk_sqkseq`
    JOIN `ew_scnl` ON `ew_arc_phase`.`fk_scnl` = `ew_scnl`.`id`
    WHERE `ew_arc_summary`.`ot_dt` >= in_start_time 
        AND `ew_arc_summary`.`ot_dt` <= in_end_time
    GROUP BY `ew_instance`.`ewname`,`ew_module`.`modname`,`ew_sqkseq`.`qkseq`,`ew_arc_summary`.`version`
    ORDER BY `ew_arc_summary`.`ot_dt`,`ew_arc_summary`.`ot_usec`;
END $$
DELIMITER ;

