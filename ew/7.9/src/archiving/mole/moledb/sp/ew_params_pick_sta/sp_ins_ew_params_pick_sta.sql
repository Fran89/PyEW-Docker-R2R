
DROP PROCEDURE IF EXISTS `sp_ins_ew_params_pick_sta`;
DELIMITER $$
CREATE  DEFINER = CURRENT_USER  PROCEDURE `sp_ins_ew_params_pick_sta`(
    in_ewname              VARCHAR(80),
    in_modname             VARCHAR(80),
    in_Pick_Flag           INT,
    in_Pin_Numb            BIGINT,
    in_Station             CHAR(5),
    in_Comp                CHAR(3),
    in_Net                 CHAR(2),
    in_Loc                 CHAR(2),
    in_Itr1                BIGINT,
    in_MinSmallZC          BIGINT,
    in_MinBigZC            BIGINT,
    in_MinPeakSize         BIGINT,
    in_MaxMint             BIGINT,
    in_i9                  BIGINT,
    in_RawDataFilt         DOUBLE,
    in_CharFuncFilt        DOUBLE,
    in_StaFilt             DOUBLE,
    in_LtaFilt             DOUBLE,
    in_EventThresh         DOUBLE,
    in_RmavFilt            DOUBLE,
    in_DeadSta             DOUBLE,
    in_CodaTerm            DOUBLE,
    in_AltCoda             DOUBLE,
    in_PreEvent            DOUBLE,
    in_Erefs               DOUBLE,
    in_ClipCount           BIGINT
)
BEGIN
DECLARE l_seisnet_is 		BIGINT DEFAULT 0;
DECLARE l_ext_id 		BIGINT DEFAULT NULL;
DECLARE l_fk_scnl_present	BIGINT DEFAULT 0;
-- DECLARE l_ext_id_ew		BIGINT DEFAULT 0;
-- DECLARE l_id_fk_cmp		BIGINT DEFAULT 0;
DECLARE l_fk_scnl_last_revision	BIGINT DEFAULT 0;
DECLARE l_id_max_new_revision	BIGINT DEFAULT 0;
DECLARE l_id_max_revision	BIGINT DEFAULT 0;

DECLARE l_fk_module               BIGINT;
DECLARE l_fk_scnl               BIGINT;

CALL `sp_get_ew_scnl_out`(in_Station, in_Comp, in_Net, in_Loc, l_fk_scnl);
CALL `sp_get_ew_module_out`(in_ewname, in_modname, l_fk_module);

-- INGV database station information
-- Check if exists the database 'seisnet'
SELECT COUNT(*) INTO l_seisnet_is
FROM information_schema.SCHEMATA
WHERE SCHEMA_NAME = 'seisnet';

IF l_seisnet_is > 0 THEN
    -- BEGIN ONLY INGV
    -- Get l_ext_id from external INGV table channel_52
    -- If l_ext_id exists in external table get value, otherwise is set NULL
    SELECT c.id INTO l_ext_id FROM `seisnet`.`channel_52` c
	    JOIN `seisnet`.`station` s ON s.id = c.fk_station
	    JOIN `seisnet`.`network` n ON n.id = c.fk_network
    WHERE 	n.net_code = in_Net
	    AND s.id_inter = in_Station
	    AND c.code = in_Comp
	    AND c.end_time > NOW()
	    AND c.main = 1
	    AND IF (ISNULL(c.location), '--', c.location) = IF (ISNULL(in_Loc), '--', in_Loc);
    -- END ONLY INGV
END IF;

-- Get l_fk_scnl_present from ew_params_pick_sta for fk_scnl, fk_module
SELECT DISTINCT fk_scnl INTO l_fk_scnl_present FROM `ew_params_pick_sta`
WHERE fk_scnl = l_fk_scnl AND fk_module=l_fk_module;

-- Check if is already present a configuration line for l_fk_scnl
IF l_fk_scnl_present = 0 THEN
	INSERT INTO `ew_params_pick_sta` (ext_id, fk_module, fk_scnl, revision, pick_flag, itr1, minsmallzc, minbigzc, minpeaksize, maxmint, i9, rawdatafilt, charfuncfilt, stafilt, ltafilt, eventthresh, rmavfilt, deadsta, codaterm, altcoda, preevent, erefs, clipcount)
	VALUES (l_ext_id, l_fk_module, l_fk_scnl, 0, in_Pick_Flag, in_Itr1, in_MinSmallZC, in_MinBigZC, in_MinPeakSize, in_MaxMint, in_i9, in_RawDataFilt, in_CharFuncFilt, in_StaFilt, in_LtaFilt, in_EventThresh, in_RmavFilt, in_DeadSta, in_CodaTerm, in_AltCoda, in_PreEvent, in_Erefs, in_ClipCount);
	-- Output message
	SELECT 0, in_Station, in_Comp, in_Net, in_Loc, l_ext_id, l_fk_scnl, l_fk_module, 0, 'channel inserted';
ELSE

    -- Get max_revision for a fk_scnl, fk_module
    SELECT MAX(revision) INTO l_id_max_revision FROM `ew_params_pick_sta`
    WHERE fk_scnl = l_fk_scnl AND fk_module=l_fk_module;

    SET l_id_max_new_revision=l_id_max_revision+1;

    -- UPDATE `ew_params_pick_sta`
	-- SET revision = l_id_max_new_revision
	-- WHERE revision = 0 AND ext_id = l_ext_id AND fk_module=l_fk_module;
	
    -- Check if last revision is equal to the current we want insert
    SELECT fk_scnl INTO l_fk_scnl_last_revision  FROM `ew_params_pick_sta`
    WHERE
    fk_scnl = l_fk_scnl
    AND fk_module=l_fk_module
    AND revision = l_id_max_revision
    AND pick_flag = in_Pick_Flag
    AND itr1 = in_Itr1
    AND minsmallzc = in_MinSmallZC
    AND minbigzc = in_MinBigZC
    AND minpeaksize = in_MinPeakSize
    AND maxmint = in_MaxMint
    AND i9 = in_i9
    AND ABS(rawdatafilt - in_RawDataFilt) <= 0.0000001
    AND ABS(charfuncfilt - in_CharFuncFilt) <= 0.0000001
    AND ABS(stafilt - in_StaFilt) <= 0.0000001
    AND ABS(ltafilt - in_LtaFilt) <= 0.0000001
    AND ABS(eventthresh - in_EventThresh) <= 0.0000001
    AND ABS(rmavfilt - in_RmavFilt) <= 0.0000001
    AND ABS(deadsta - in_DeadSta) <= 0.0000001
    AND ABS(codaterm - in_CodaTerm) <= 0.0000001
    AND ABS(altcoda - in_AltCoda) <= 0.0000001
    AND ABS(preevent - in_PreEvent) <= 0.0000001
    AND ABS(erefs - in_Erefs) <= 0.0000001
    AND clipcount = in_ClipCount
    AND ext_id = l_ext_id;

    IF l_fk_scnl_last_revision = 0 THEN
	# Not exists
	INSERT INTO `ew_params_pick_sta` (ext_id, fk_module, fk_scnl, revision, pick_flag, itr1, minsmallzc, minbigzc, minpeaksize, maxmint, i9, rawdatafilt, charfuncfilt, stafilt, ltafilt, eventthresh, rmavfilt, deadsta, codaterm, altcoda, preevent, erefs, clipcount)
	VALUES (l_ext_id, l_fk_module, l_fk_scnl, l_id_max_new_revision, in_Pick_Flag, in_Itr1, in_MinSmallZC, in_MinBigZC, in_MinPeakSize, in_MaxMint, in_i9, in_RawDataFilt, in_CharFuncFilt, in_StaFilt, in_LtaFilt, in_EventThresh, in_RmavFilt, in_DeadSta, in_CodaTerm, in_AltCoda, in_PreEvent, in_Erefs, in_ClipCount);
	-- Output message
	SELECT 0, in_Station, in_Comp, in_Net, in_Loc, l_ext_id, l_fk_scnl, l_fk_module, l_id_max_new_revision, 'channel inserted';
    ELSE
	-- Already exists
	-- Output message
	SELECT -2, in_Station, in_Comp, in_Net, in_Loc, l_ext_id, l_fk_scnl, l_fk_module, l_id_max_revision, 'equal to last revision';
    END IF;


    # UPDATE `ew_params_pick_sta`
    # SET fk_module=l_fk_module, fk_scnl=l_fk_scnl, pick_flag = in_Pick_Flag, itr1 = in_Itr1, minsmallzc = in_MinSmallZC, minbigzc = in_MinBigZC, minpeaksize = in_MinPeakSize, maxmint = in_MaxMint, i9 =in_i9, rawdatafilt = in_RawDataFilt, charfuncfilt = in_CharFuncFilt, stafilt = in_StaFilt, ltafilt = in_LtaFilt, eventthresh = in_LtaFilt, rmavfilt = in_RmavFilt, deadsta = in_DeadSta, codaterm = in_CodaTerm, altcoda = in_AltCoda, preevent = in_PreEvent, erefs = in_Erefs, clipcount = in_ClipCount
    # WHERE ext_id = l_ext_id AND fk_module=l_fk_module;

END IF;

END$$
DELIMITER ;

