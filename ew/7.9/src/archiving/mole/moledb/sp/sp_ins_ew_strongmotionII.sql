
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_strongmotionII`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_strongmotionII`(
in_ewname        VARCHAR(80)     /*Host running the EW Module*/,
in_modname       VARCHAR(32)     /*Module name*/,
in_qid           BIGINT          /*quake id*/,
-- in_version       BIGINT          /*version number of the origin*/,
in_site          VARCHAR(5)      /*site code, null terminated*/,
in_comp          VARCHAR(3)      /*component code, null terminated*/,
in_net           VARCHAR(2)      /*seismic network code, null terminated*/,
in_loc           VARCHAR(2)      /*location code, null terminated*/,
in_qauthor       VARCHAR(32),
in_t             VARCHAR(30)     /*time: trigger reported by SM box - a string in datetime format*/,
in_t_alt         VARCHAR(30)     /*alternate time: trigger reported by SM box - a string in datetime format*/,
in_altcode       INT             /*code specifying the source of the alternate time field*/,
in_pga           DOUBLE          /*REQUIRED: peak ground acceleration (cm/s/s)*/,
in_tpga          VARCHAR(30)     /*OPTIONAL: time of pga - datetime part*/,
in_pgv           DOUBLE          /*REQUIRED: peak ground velocity (cm/s)*/,
in_tpgv          VARCHAR(30)     /*OPTIONAL: time of pgv - datetime part*/,
in_pgd           DOUBLE          /*REQUIRED: peak ground displacement (cm)*/,
in_tpgd          VARCHAR(30)     /*OPTIONAL: time of pgd - datetime part*/,
in_rsa           VARCHAR(256)    /*RSA(response spectrum accel)  string format: NRSA/periodRSA valueRSA/periodRSA valueRSA*/
)
uscita: BEGIN
DECLARE l_ewname      VARCHAR(80);
DECLARE l_modname     VARCHAR(32);
DECLARE l_qid         BIGINT;
-- DECLARE l_version     BIGINT;
DECLARE l_site        VARCHAR(5);
DECLARE l_net         VARCHAR(3);
DECLARE l_comp        VARCHAR(4);
DECLARE l_loc         VARCHAR(2);
DECLARE l_qauthor     VARCHAR(32);
DECLARE l_t_dt        DATETIME;
DECLARE l_t_usec      DOUBLE;
DECLARE l_t_alt_dt    DATETIME;
DECLARE l_t_alt_usec  DATETIME;
DECLARE l_altcode     INT;
DECLARE l_pga         DOUBLE;
DECLARE l_tpga_dt     DATETIME;
DECLARE l_tpga_usec   DOUBLE;
DECLARE l_pgv         DOUBLE;
DECLARE l_tpgv_dt     DATETIME;
DECLARE l_tpgv_usec   DOUBLE;
DECLARE l_pgd         DOUBLE;
DECLARE l_tpgd_dt     DATETIME;
DECLARE l_tpgd_usec   DOUBLE;
DECLARE l_rsa         VARCHAR(256);

DECLARE l_fk_module     BIGINT;
DECLARE l_sqkseq      BIGINT;
DECLARE l_fk_scnl      BIGINT;

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

IF TRIM(in_qid)='' THEN 
        SELECT 'ERROR: quake id can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_qid = in_qid;

SET l_site = in_site;
SET l_net = in_net;
SET l_comp = in_comp;

IF TRIM(in_loc)='' THEN 
     SET in_loc = null;
END IF;
SET l_loc = in_loc;

SET l_qauthor=in_qauthor;

SET l_t_dt = in_t;
SELECT MICROSECOND(in_t) INTO l_t_usec;

SET l_t_alt_dt = in_t_alt;
/*IF TRIM(in_t_alt)='' THEN 
    SET l_t_alt_usec = '0';
ELSE*/
    SELECT MICROSECOND(in_t_alt) INTO l_t_alt_usec;
-- END IF;

SET l_altcode = in_altcode;
SET l_pga = in_pga;

SET l_tpga_dt = in_tpga;
SELECT MICROSECOND(in_tpga) INTO l_tpga_usec;

SET l_pgv = in_pgv;

SET l_tpgv_dt = in_tpgv;
SELECT MICROSECOND(in_tpgv) INTO l_tpgv_usec;

SET l_pgd = in_pgd;

SET l_tpgd_dt = in_tpgd;
SELECT MICROSECOND(in_tpgd) INTO l_tpgd_usec;

SET l_rsa = in_rsa;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qid, l_sqkseq);
CALL `sp_get_ew_scnl_out`(l_site, l_comp, l_net, l_loc, l_fk_scnl);

INSERT INTO `ew_strongmotionII` (fk_module,fk_sqkseq,fk_scnl,qauthor,
        t_dt,t_usec,t_alt_dt,t_alt_usec,altcode,
        pga,tpga_dt,tpga_usec,pgv,tpgv_dt,tpgv_usec,pgd,tpgd_dt,tpgd_usec,rsa)
VALUES (l_fk_module,l_sqkseq,l_fk_scnl,l_qauthor,
        l_t_dt,l_t_usec,l_t_alt_dt,l_t_alt_usec,l_altcode,
        l_pga,l_tpga_dt,l_tpga_usec,l_pgv,l_tpgv_dt,l_tpgv_usec,l_pgd,l_tpgd_dt,l_tpgd_usec,l_rsa);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

