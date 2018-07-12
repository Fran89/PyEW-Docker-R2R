
DROP TABLE IF EXISTS `eb_alarm`;
CREATE TABLE `eb_alarm` (
  `id`               BIGINT   NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
  `fk_module`        BIGINT   NOT NULL COMMENT 'Foreign key to ew_module',

  `message`          VARCHAR(512) DEFAULT NULL COMMENT 'Alarm text message',

  `modified`        TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',            
  PRIMARY KEY (`id`),
  FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'TYPE_ALARM';

