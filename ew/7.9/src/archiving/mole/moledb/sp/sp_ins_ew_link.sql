
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_ins_ew_link`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_link`(
in_ewname        VARCHAR(80)     /*Host running the EW Module*/,
in_modname       VARCHAR(32)     /*Module name*/,
in_qkseq         BIGINT          /*link sequence number*/,
in_pkseq         BIGINT          /*phase sequence number*/,
in_iphs          INT
)
uscita: BEGIN
DECLARE l_ewname    VARCHAR(80);
DECLARE l_modname     VARCHAR(32);
DECLARE l_qkseq       INT;
DECLARE l_pkseq       BIGINT;
DECLARE l_iphs        INT;

DECLARE l_fk_module               BIGINT;
DECLARE l_sqkseq                BIGINT;
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

IF TRIM(in_qkseq)='' THEN 
        SELECT 'ERROR: link sequence number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_qkseq=in_qkseq;

IF TRIM(in_pkseq)='' THEN 
        SELECT 'ERROR: phase sequence number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_pkseq=in_pkseq;

IF TRIM(in_iphs)='' THEN 
        SELECT 'ERROR: phase number can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_iphs=in_iphs;

CALL `sp_get_ew_module_out`(l_ewname, l_modname, l_fk_module);
CALL `sp_get_ew_sqkseq_out`(l_ewname, l_qkseq, l_sqkseq);
CALL `sp_get_ew_spkseq_out`(l_ewname, l_pkseq, l_spkseq);

INSERT INTO `ew_link` (fk_module,fk_sqkseq,fk_spkseq,iphs)
    VALUES (l_fk_module,l_sqkseq,l_spkseq,l_iphs);

SELECT LAST_INSERT_ID();

END$$
DELIMITER ;

