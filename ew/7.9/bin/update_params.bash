#! /bin/bash
cd $EW_HOME/$EW_VERSION

for i in src/seismic_processing src/display src/grab_bag src/archiving src/data_exchange src/data_sources src/display src/diagnostic_tools src/reporting src/system_control
do
	cp $i/*/*.d params
	cp $i/*/*.desc params
done
cp environment/earthworm.d params
cp environment/earthworm_global.d params
