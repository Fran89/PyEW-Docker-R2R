Building picker C-function and test program (picker_func_test_memory):
	use gmake or equivalent in this directory


Running the test program (compares Java and C versions of picker):
	1. install latest version of SG2K: http://www.alomax.net/seisgram/beta/SeisGram2K60.jar
	2. cd to test_data
	3. choose a SAC data file (must be in local machine byte order, examples are in PC_INTEL byte order,
		order can be changed by saving active trace in SG2K as SAC_BINARY with desired byte order)
	4. use script run.bash to run SG2K and picker_func_test_memory on same data file
		e.g. ./run.bash 2006_05_06_16_57_24_COL3_0.SAC
	5. displayed SG2K and picker_func_test_memory pick results should be identical (picker_func_test_memory picks have labels that end in "_" and are drawn in grey in SG2K)


Documentation:
	FilterPickerN: use "grep _DOC_" on FilterPickerN.c and FilterPickerN_Memory.c, or see DOC.txt
	picker C-function: see picker_func_test_memory.c to understand how to call picker function


References:
    Lomax, A., M. Vassallo and C. Satriano (2011), Automatic picker developments and optimization: FilterPicker - a robust, broadband picker for real-time seismic monitoring and earthquake early-warning, submitted to SRL.
    Vassallo, M., C. Satriano and A. Lomax, (2011), Automatic picker developments and optimization: an optimization strategy for improving the performances of automatic phase pickers, submitted to SRL.

	see also: http://www.alomax.net/pub_list.html 