DELIMITER $$
DROP FUNCTION IF EXISTS `fn_check_if_host_fill_db` $$
CREATE FUNCTION `fn_check_if_host_fill_db` (
)
RETURNS TINYINT
NOT DETERMINISTIC
BEGIN

    DECLARE ret                     TINYINT         DEFAULT 0;
    DECLARE l_seisev_is             TINYINT         DEFAULT 0;
    DECLARE l_host_fill_db_is       TINYINT         DEFAULT 0;

    SELECT COUNT(*) INTO l_seisev_is 
    FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'seisev';
    -- controllo messo per non inserire da restore fatto sulla macchina (localhost)
    -- i dati provenienti da macchine di test, ovvero diverse da hew1 e hew2

    SELECT COUNT(*) INTO l_host_fill_db_is 
    FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = 'host_fill_db';

    IF l_seisev_is > 0 AND l_host_fill_db_is > 0 THEN
	IF host_fill_db.`fn_fill_seisev`() = 1 THEN
	    SET ret = 1;
	END IF;
    END IF;

RETURN ret;

END $$
DELIMITER ;
