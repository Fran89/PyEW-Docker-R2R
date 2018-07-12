
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_pick_scnl`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_pick_scnl`(

in_ewname        VARCHAR(80)     /*Host running the EW Module*/,
in_modname       VARCHAR(32)     /*Module name*/,
in_seq           INT             /*Sequence number*/,
in_net           VARCHAR(2)      /*Net code*/,
in_sta           VARCHAR(5)      /*Station code => pick->site*/,
in_cha           VARCHAR(3)      /*Channel code => pick->component*/,
in_loc           VARCHAR(2)      /*Location code*/,
in_dir           CHAR(1)         /*First-motion descriptor (U,D,-space-,?) => pick->fm*/,
in_wt            TINYINT         /*Pick weight or quality (0-4)*/,
in_tpick         VARCHAR(30)     /*Time of pick - a string in datetime format*/,
in_pamp_0        BIGINT          /*pick->pamp[0]*/,
in_pamp_1        BIGINT          /*pick->pamp[1]*/,    
in_pamp_2        BIGINT          /*pick->pamp[2]*/        
)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname     VARCHAR(32);
DECLARE l_seq         INT;
DECLARE l_net         VARCHAR(2);
DECLARE l_sta         VARCHAR(5);
DECLARE l_cha         VARCHAR(3);
DECLARE l_loc         VARCHAR(2);
DECLARE l_dir         CHAR(1);
DECLARE l_wt          TINYINT;
DECLARE l_tpick_dt    DATETIME;
DECLARE l_tpick_usec  BIGINT;
DECLARE l_pamp_0      BIGINT;
DECLARE l_pamp_1      BIGINT;
DECLARE l_pamp_2      BIGINT;
DECLARE l_ew_pick_id  BIGINT;

DECLARE l_fk_module               BIGINT;
DECLARE l_fk_scnl               BIGINT;
DECLARE l_spkseq                BIGINT;

IF TRIM(in_ewname)='' THEN 
        SELECT 'ERROR: EW instance name can not be empty' out_msg,-1 id;
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

IF TRIM(in_dir)='' THEN 
        SELECT 'ERROR: first motion can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_dir=in_dir;

IF TRIM(in_wt)='' THEN 
        SELECT 'ERROR: weight can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_wt=in_wt;

IF TRIM(in_tpick)='' THEN 
        SELECT 'ERROR: pick time can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;

SET l_tpick_dt=SUBSTR(in_tpick, 1, 19);
SELECT MICROSECOND(in_tpick) INTO l_tpick_usec ;

SET l_pamp_0=in_pamp_0;
SET l_pamp_1=in_pamp_1;
SET l_pamp_2=in_pamp_2;

CALL `sp_get_ew_scnl_out`(l_sta, l_cha, l_net, l_loc, l_fk_scnl);
CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);
CALL `sp_get_ew_spkseq_out`(l_ewname, l_seq, l_spkseq);

INSERT INTO `ew_pick_scnl` (fk_module,fk_spkseq,fk_scnl,dir,wt,tpick_dt,tpick_usec,pamp_0,pamp_1,pamp_2)
    VALUES (l_fk_module,l_spkseq,l_fk_scnl,l_dir,l_wt,l_tpick_dt,l_tpick_usec,l_pamp_0,l_pamp_1,l_pamp_2);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

