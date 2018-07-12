
DELIMITER $$
DROP TRIGGER IF EXISTS `tr_ew_quake2k_a_i`$$
CREATE TRIGGER `tr_ew_quake2k_a_i`
AFTER INSERT ON `ew_quake2k`
FOR EACH ROW
BEGIN

  UPDATE ew_sqkseq SET qknph = NEW.nph WHERE ew_sqkseq.id = NEW.fk_sqkseq;

  -- --  Check if the event has been cancelled by binder_ew
  -- IF NEW.nph <= 0 THEN
  --   -- Earthquake has been canceled, no associated phases
  --   UPDATE ew_sqkseq SET qkcancel = 1 WHERE ew_sqkseq.id = NEW.fk_sqkseq;
  -- ELSE
  --   -- Earthquake is ok, associated phases are greater than zero and it has not been canceled before
  --   UPDATE ew_sqkseq SET qkcancel = 0 WHERE ew_sqkseq.id = NEW.fk_sqkseq AND ew_sqkseq.qkcancel <> 1;
  -- END IF;

END;
$$
DELIMITER ;

