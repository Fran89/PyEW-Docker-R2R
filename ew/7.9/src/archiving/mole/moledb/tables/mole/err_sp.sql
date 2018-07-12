
DROP TABLE IF EXISTS `err_sp`;
CREATE TABLE IF NOT EXISTS `err_sp` (
    `id`                BIGINT NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `sp_name`           VARCHAR(255) NULL          COMMENT 'stored procedure name',
    `error`             VARCHAR(1024) NULL          COMMENT 'error description',
    `gtime`          DOUBLE                     COMMENT 'instance call identifier',
    `modified`          TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',
    PRIMARY KEY (`id`)
)
ENGINE = InnoDB COMMENT 'Error from sp call by trigger';

