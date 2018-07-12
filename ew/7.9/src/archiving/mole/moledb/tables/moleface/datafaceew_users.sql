
-- DROP TABLE IF EXISTS datafaceew_users;

CREATE TABLE  `datafaceew_users` (
  `id`              BIGINT NOT NULL AUTO_INCREMENT                      COMMENT 'Unique incremental id',
  `usr_name`        VARCHAR(50) NOT NULL                                COMMENT 'user name',
  `name`            VARCHAR(50) NULL                                    COMMENT 'First Name',
  `surname`         VARCHAR(50) NULL                                    COMMENT 'Surname',
  `pwd`             VARCHAR(50) NULL                                    COMMENT 'encrypted password by MySQL function PASSWORD()',
  `tel_num`         VARCHAR(50) NULL                                    COMMENT 'telephon numner',
  `email`           VARCHAR(50) NULL                                    COMMENT 'email address',
  `modified`        TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',
  `fk_group`        BIGINT NULL                                         COMMENT 'Link: user_group',
  `Role`            ENUM('READ ONLY','NO ACCESS','EDIT','ADMIN','BACKOFFICE') DEFAULT 'READ ONLY' COMMENT 'moleface role',
  PRIMARY KEY (`id`),
  UNIQUE (`usr_name`)
)
ENGINE = INNODB COMMENT 'Users that can access mole via moleface';

