************************************  Da-Yi (Ta-Yi) Chen, March, 17th,2016 

The Earthquake Early Warning (EEW) modules developed in the Earthworm environment were contributed by the 

Professor Yih-Min Wu at National Taiwan University, and Dr. Da-Yi (Ta-Yi) Chen at Central Weather Bureau (CWB), Taiwan.

Originally, major functions in the EEW programs were developed by Prof. Wu. The program deals with estimating peak amplitudes, 

associating P-wave arrivals, determining hypocenter and magnitude. In order to implement those programs in the CWB's real-time 

seismic network, Dr. Chen modified the codes and integrated them into the Earthworm.

----------------------------------------------------------------------------------------------------------------------------------------

This folder contains EEW modules and an earthquake data from Taiwan. 

The folder was tested in the Windows. But the codes can also be compiled in the Linux.

Some configured files used in the EEW modules are contained in the folder named 'params_cwb_pub'.

You can take this folder as an example to configure your system.

In the following of this file, I will explain the structures of two new message types created in the EEW modules, 

as well as two configured files related to auto-picking and earthquake parameter estimations.

Other guildlines have been written in the configured files (.d files).

You can also refer to the paper:

Chen, D. Y., N. C. Hsiao, and Y. M. Wu, (2015). The Earthworm based earthquake alarm reporting system in Taiwan. Bull. Seismol. Soc. Am., 105 ,568-579.

Finally, if you need help you can send email to Da-Yi (Ta-Yi) Chen: dayi@scman.cwb.gov.tw

----------------------------------------------------------------------------------------------------------------------------------------

The EEW modules contains 3 modules:

Pick_eew : Pick P-wave arrivals and calculate Pa, Pv, Pd values

Tcpd :     Associate picks, evolutionarily determine earthquake location and magnitude

Dcsn_xml:  Decision making for releasing EEW message by XML formatted file.

----------------------------------------------------------------------------------------------------------------------------------------

Although there are only three basic EEW modules, based on the Earthworm platform you can ceate a parallel system for data processing.

For example, first, assumed that real-time data entering into several shared memories, WAVE_RINGs. 

You may have several Pick_eew modules processing in different shared memories, WAVE_RINGs, and sending picks into  different shared memories, PICK_RINGs.

But you can use Ringdup or Export modules to integrate all picks into the same shared memory, PICK_RING_ALL.

Then the Tcpd module can access all picks in the PICK_RING_ALL for estimating earthquake parameters.

You also can have two Tcpd modules running with different configured file, but send EEW message into the same shared memory, EEW_RING.

Finally, the Dcsn_xml module can receive earthquake message from the EEW_RING and make better choice for releasing outside.



Following are some references for you:

*****************************************************************************************************************************************************
< TYPE_EEW >

A new message type named TYPE_EEW was created for EEW information.

Following is an example, you can use the sniffring command to reveal it in the EEW_RING.

5 1458193037.523000 1458193017.257403 34 Mpd 6.0 23.58 121.43 20.0 38 25 18 0.8 0.5 -4 161 20.3 231 0.0

----------------------------------------------------------------------------------------------------------------------------------------
	           There are 19 columns in the file, followings are the description.
		
		   Column  1 ( 5                  ) : NoEQ,     Event number, each earthquake has its own unique number.
		   Column  2 ( 1458193037.523000  ) : T_now,    When report this message
		   Column  3 ( 1458193017.257403  ) : hyptime,  Origin time of the event 
		   Column  4 ( 34                 ) : Ccount,   Updated order of this event. (34 means 34th report for this event) 
		   Column  5 ( Mpd                ) : Mpd,      Magnitude type
		   Column  6 ( 6.0                ) : Magnitude
		   Column  7 ( 23.58              ) : Lat,      Latitude of the event 
		   Column  8 ( 121.43             ) : Lon,      Longitude of the event  
		   Column  9 ( 20.0               ) : Dep,      Depth of the event  
		   Column 10 ( 38                 ) : G_n,      Number of triggered stations
		   Column 11 ( 25                 ) : G_sn,     Number of triggered stations, not including cosite stations
		   Column 12 ( 18                 ) : G_n_mag,  Number of stations we used for magnitude estimations
		   Column 13 ( 0.8                ) : Averr,    Average travel time residuals of all triggered stations
		   Column 14 ( 0.5                ) : Avwei,    Average weighting of all triggered stations
		   Column 15 ( -4                 ) : G_Q,      Quality of the inversion procedure
		   Column 16 ( 161                ) : Gap,      Station coverage
		   Column 17 ( 20.3               ) : Pro_time, Elapsed time for reporting after the earthquake occurred
		   Column 18 ( 231                ) : Mark,     Identifying which system made this report

*****************************************************************************************************************************************************
< Type_EEW_record >

A new message type named Type_EEW_record was created for EEW information.

Following is an example, you can use the sniffring command to reveal it in the PICK_RING.

ECL HSZ ST 01 120.962000 22.596000 1.093318 0.038467 0.001974 0.358703 1458195511.82500 0 2 3

----------------------------------------------------------------------------------------------------------------------------------------
	           There are 19 columns in the file, followings are the description.
		
		   Column  1 ( ECL                 ) : Station
		   Column  2 ( HSZ                 ) : Comp  
		   Column  3 ( ST                  ) : Net
		   Column  4 ( 01                  ) : Loc
		   Column  7 ( 120.962000          ) : Lon,        Longitude of the station
		   Column  8 ( 22.596000           ) : Lat,        Latitude of the station
		   Column  9 ( 1.093318            ) : Pa,         Peak amplitude in acceleration	  
		   Column 10 ( 0.038467            ) : Pv,         Peak amplitude in velocity
		   Column 11 ( 0.001974            ) : Pd,     	   Peak amplitude in displacement
		   Column 12 ( 0.358703            ) : Tc,         Average period
		   Column 13 ( 1458195511.82500    ) : Parr,       P-wave arrival time
		   Column 14 ( 0                   ) : Wei,        Pick Weighting, 0 is the best, 5 is the worst
		   Column 15 ( 2                   ) : Inst,       Instrument type, 1: Acceleration, 2: Velocity
		   Column 16 ( 3                   ) : Inter,      Time window length after P-wave arrival



*****************************************************************************************************************************************************
< pick_eew >

It needs two input files, 'pick_eew.sta' and 'sta_hisn_Z'.

1. 'pick_eew.sta': This file is for picking the onset of P-wave arrival.
		   Each channel has its own parameters.

		   Following is a default setting :
					
		   0	1   NWF   HLZ  BH 02    3    70      0.939 3.0 0.6 0.015   6.0 0.9961 1000000  0.001   0.001
----------------------------------------------------------------------------------------------------------------------------------------
		   When you add new channel in the picking list, I suggest you can use default values.
	           There are 17 columns in the file, followings are the description.
		
		   Column  1 ( 0       ) : Pick flag,    the same to the one used in the original pick_ew module. 
		   Column  2 ( 1       ) : Pin number,   the same to the one used in the original pick_ew module. 
		   Column  3 ( NWF     ) : Station,      the same to the one used in the original pick_ew module. 
		   Column  4 ( HLZ     ) : Comp,         the same to the one used in the original pick_ew module. 
		   Column  5 ( BH      ) : Net,          the same to the one used in the original pick_ew module.  
		   Column  6 ( 02      ) : Loc,          the same to the one used in the original pick_ew module.  
		   Column  7 ( 3       ) : MinZC,        the minimum number of zero crossing. 
		   Column  8 ( 70      ) : MaxMint,      the same to the one used in the original pick_ew module. 
		   Column  9 ( 0.939   ) : RawDataFilt,  the same to the one used in the original pick_ew module.
		   Column 10 ( 3.0     ) : CharFuncFilt, the same to the one used in the original pick_ew module.
		   Column 11 ( 0.6     ) : StaFilt,      the same to the one used in the original pick_ew module.
		   Column 12 ( 0.015   ) : LtaFilt,      the same to the one used in the original pick_ew module. 
		   Column 13 ( 6.0     ) : EventThresh,  the same to the one used in the original pick_ew module. 
		   Column 14 ( 0.9961  ) : RmavFilt,     the same to the one used in the original pick_ew module.
		   Column 15 ( 1000000 ) : DeadSta,      the same to the one used in the original pick_ew module.
		   Column 16 ( 0.001   ) : MaxMintPa,    the minimum value of peak amplitude in acceleration.
		   Column 17 ( 0.001   ) : MaxMintPv,    the minimum value of peak amplitude in velocity.
----------------------------------------------------------------------------------------------------------------------------------------
2. 'sta_hisn_Z':   This file is for estimations of earthquake location and magnitude.
		   Each channel has its own parameters.

		   Following is an example :
					
		   ALS   HHZ  BB 01 23.5083 120.8134 100 2.331763E+007 2 
----------------------------------------------------------------------------------------------------------------------------------------
	           There are 9 columns in the file, followings are the description.
		
		   Column  1 ( ALS           ) : Station
		   Column  2 ( HHZ           ) : Comp
		   Column  3 ( BB            ) : Net
		   Column  4 ( 01            ) : Loc
		   Column  5 ( 23.5083       ) : Latitude  
		   Column  6 ( 120.8134      ) : Longitude    
		   Column  7 ( 100           ) : Sampling rate, samples per second 
		   Column  8 ( 2.331763E+007 ) : gain factor,  ( count ) / ( gain factor )  =  (physical value) 
		   Column  9 ( 2             ) : instrument type, 1: acceleration, 2: velocity 
----------------------------------------------------------------------------------------------------------------------------------------


*****************************************************************************************************************************************************


			