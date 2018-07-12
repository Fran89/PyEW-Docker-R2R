
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_coda_scnl`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_coda_scnl`(

in_ewname        VARCHAR(80)     /*Host running the EW Module*/,
in_modname       VARCHAR(32)     /*Module name*/,
in_seq           INT             /*Sequence number*/,
in_net           VARCHAR(2)      /*Net code*/,
in_sta           VARCHAR(5)      /*Station code => pick->site*/,
in_cha           VARCHAR(3)      /*Channel code => pick->component*/,
in_loc           VARCHAR(2)      /*Location code*/,
in_caav_0        BIGINT          /*coda->caav[0]*/,
in_caav_1        BIGINT          /*coda->caav[1]*/,
in_caav_2        BIGINT          /*coda->caav[2]*/,
in_caav_3        BIGINT          /*coda->caav[3]*/,
in_caav_4        BIGINT          /*coda->caav[4]*/,
in_caav_5        BIGINT          /*coda->caav[5]*/,
in_dur           BIGINT          /*coda->dur*/
)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname     VARCHAR(32);
DECLARE l_seq         INT;
DECLARE l_net         VARCHAR(2);
DECLARE l_sta         VARCHAR(5);
DECLARE l_cha         VARCHAR(3);
DECLARE l_loc         VARCHAR(2);
DECLARE l_caav_0      BIGINT;
DECLARE l_caav_1      BIGINT;
DECLARE l_caav_2      BIGINT;
DECLARE l_caav_3      BIGINT;
DECLARE l_caav_4      BIGINT;
DECLARE l_caav_5      BIGINT;
DECLARE l_dur         BIGINT;

DECLARE l_fk_module               BIGINT;
DECLARE l_fk_scnl               BIGINT;
DECLARE l_spkseq                BIGINT;

IF TRIM(in_ewname)='' THEN 
        SELECT 'ERROR: EW instance can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_ewname=in_ewname;

IF TRIM(in_modname)='' THEN 
        SELECT 'ERROR: modname can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_modname=in_modname;


IF TRIM(in_seq)='' THEN 
        SELECT 'ERROR: sequence number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_seq=in_seq;

IF TRIM(in_net)='' THEN 
        SELECT 'ERROR: net name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_net=in_net;

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

IF TRIM(in_loc)='' THEN 
    SET in_loc=null;
END IF;
SET l_loc=in_loc;

SET l_caav_0=in_caav_0;
SET l_caav_1=in_caav_1;
SET l_caav_2=in_caav_2;
SET l_caav_3=in_caav_3;
SET l_caav_4=in_caav_4;
SET l_caav_5=in_caav_5;
SET l_dur=in_dur;

CALL `sp_get_ew_scnl_out`(l_sta, l_cha, l_net, l_loc, l_fk_scnl);
CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);
CALL `sp_get_ew_spkseq_out`(l_ewname, l_seq, l_spkseq);

-- UPDATE `ew_pick_scnl`
-- SET caav_0=l_caav_0,caav_1=l_caav_1,caav_2=l_caav_2,caav_3=l_caav_3,caav_4=l_caav_4,caav_5=l_caav_5,dur=l_dur
-- WHERE ewname=l_ewname AND modname=l_modname AND seq=l_seq AND net=l_net AND sta=l_sta AND cha=l_cha AND loc=l_loc;

UPDATE `ew_pick_scnl`
SET caav_0=l_caav_0,caav_1=l_caav_1,caav_2=l_caav_2,caav_3=l_caav_3,caav_4=l_caav_4,caav_5=l_caav_5,dur=l_dur
WHERE fk_module=l_fk_module AND fk_spkseq=l_spkseq AND fk_scnl=l_fk_scnl;

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

