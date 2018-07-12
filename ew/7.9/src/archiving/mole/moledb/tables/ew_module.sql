
DROP TABLE IF EXISTS `ew_module`;
CREATE TABLE `ew_module` (
    `id`                    BIGINT    NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_instance`                  BIGINT    NOT NULL COMMENT 'Id of the EW instance running',
    `modname`               CHAR(32)  NOT NULL COMMENT 'EW module name',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',        
    PRIMARY KEY (`id`),
    UNIQUE (`fk_instance`, `modname`),
    FOREIGN KEY (`fk_instance`) REFERENCES ew_instance(`id`) ON DELETE CASCADE ON UPDATE CASCADE ,
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'EW module name';

