
DELIMITER $$
DROP PROCEDURE IF EXISTS `sp_get_ew_spkseq_out`$$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_get_ew_spkseq_out`(
in_ewname                VARCHAR(80)  /* Name of the EW instance running */,
in_pkseq                 BIGINT  /* Pick sequence number from picker */,
OUT l_id                 BIGINT
)
uscita: BEGIN
DECLARE l_ewname         VARCHAR(80);
DECLARE l_pkseq          BIGINT;
DECLARE l_fk_instance           BIGINT;

SET l_id = -1;

IF TRIM(in_ewname)='' THEN 
        SELECT 'ERROR: Ew instance name can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_ewname=in_ewname;

IF TRIM(in_pkseq)='' THEN 
        SELECT 'ERROR: Pick sequence number from picker can not be empty' out_msg,-1 id;
        LEAVE uscita;
END IF;
SET l_pkseq=in_pkseq;

CALL `sp_get_ew_instance_out`(l_ewname, l_fk_instance);

SELECT id INTO l_id FROM `ew_spkseq` WHERE fk_instance=l_fk_instance AND pkseq=l_pkseq;

IF l_id = -1 THEN
    INSERT INTO `ew_spkseq`(fk_instance,pkseq) VALUES (l_fk_instance,l_pkseq);
    SELECT LAST_INSERT_ID() INTO l_id;
END IF;

-- SELECT l_id;

END$$
DELIMITER ;

