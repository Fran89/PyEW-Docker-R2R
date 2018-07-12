
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_scnl`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_scnl`(
in_sta                   VARCHAR(5)  /* Station code */,
in_cha                   VARCHAR(3)  /* Channel/Component code */,
in_net                   VARCHAR(2)  /* Net code */,
in_loc                   VARCHAR(2)  /* Location code */,
in_lat                   DOUBLE      /* Latitude */,
in_lon                   DOUBLE      /* Longitude */,
in_ele                   DOUBLE      /* Elevation */,
in_name                  VARCHAR(80) /* Site name */,
in_ext_id                BIGINT      /* External table id, negative value for NULL */
)
uscita: BEGIN
DECLARE l_sta                   VARCHAR(5);
DECLARE l_cha                   VARCHAR(3);
DECLARE l_net                   VARCHAR(2);
DECLARE l_loc                   VARCHAR(2);
DECLARE l_lat                   DOUBLE;
DECLARE l_lon                   DOUBLE;
DECLARE l_ele                   DOUBLE;
DECLARE l_name                  VARCHAR(80);
DECLARE l_ext_id                BIGINT;

DECLARE l_fk_scnl                BIGINT;

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

IF TRIM(in_loc)='' THEN 
    SET in_loc=null;
END IF;
SET l_loc=in_loc;

SET l_lat=in_lat;
SET l_lon=in_lon;
SET l_ele=in_ele;

IF TRIM(in_name)='' THEN 
    SET in_name=null;
END IF;
SET l_name=in_name;

IF TRIM(in_ext_id) < 0 THEN 
    SET in_ext_id=null;
END IF;
SET l_ext_id=in_ext_id;

CALL `sp_get_ew_scnl_out`(l_sta, l_cha, l_net, l_loc, l_fk_scnl);

UPDATE `ew_scnl`
    SET lat=l_lat,lon=l_lon,ele=l_ele,name=l_name,ext_id=l_ext_id
    WHERE id=l_fk_scnl;

SELECT l_fk_scnl;

END$$
DELIMITER ;

