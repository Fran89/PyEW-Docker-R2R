
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_update_mole_version`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_update_mole_version`(
in_version               VARCHAR(255)  /* moledb version  */,
in_release_date          DATE          /* Date of release */
)
uscita: BEGIN

IF TRIM(in_version)='' THEN 
        SELECT 'ERROR: in_version can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;

INSERT INTO version (id, version, release_date)
  VALUES(0, in_version, in_release_date)
  ON DUPLICATE KEY UPDATE version=in_version, release_date=in_release_date;

END$$
DELIMITER ;

