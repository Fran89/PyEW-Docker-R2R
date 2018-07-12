
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_get_ew_scnl_out`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_get_ew_scnl_out`(
in_sta                   VARCHAR(5)  /* Station code */,
in_cha                   VARCHAR(3)  /* Channel/Component code */,
in_net                   VARCHAR(2)  /* Net code */,
in_loc                   VARCHAR(2)  /* Location code */,
OUT l_id                 BIGINT
)
uscita: BEGIN
DECLARE l_sta                   VARCHAR(5);
DECLARE l_cha                   VARCHAR(3);
DECLARE l_net                   VARCHAR(2);
DECLARE l_loc                   VARCHAR(2);

SET l_id = -1;

IF TRIM(in_sta)='' THEN 
        SELECT 'ERROR: station (site) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_sta=in_sta;

IF TRIM(in_cha)='' THEN 
        SELECT 'ERROR: channel (component) name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_cha=in_cha;

IF TRIM(in_net)='' THEN 
        SELECT 'ERROR: net name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_net=in_net;

IF in_loc IS NULL OR TRIM(in_loc)='' OR TRIM(in_loc)='--' THEN 
    SET in_loc='--';
END IF;
SET l_loc=in_loc;

SELECT id INTO l_id FROM `ew_scnl` WHERE sta=l_sta AND cha=l_cha AND net=l_net AND loc=l_loc;

IF l_id = -1 THEN
    INSERT INTO `ew_scnl`(sta,cha,net,loc) VALUES (l_sta,l_cha,l_net,l_loc);
    SELECT LAST_INSERT_ID() INTO l_id;
END IF;

-- SELECT l_id;

END$$
DELIMITER ;

