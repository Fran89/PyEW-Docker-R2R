This is PsnAdSend release 2.7 for Windows

File updated: 5/12/16

Notes:
	Make sure you copy the new PsnAdSend.exe AND PSNADBoard32.dll files to your
	Earthworm working directory. 
	
	You will also need to update your PsnAdSend.d file
	with the new configuration format. See the html
	documentation files for more information

	This program won't compile with 'express' versions of Visual Studio
	because it needs Microsoft Foundation Classes from 'pro' versions.

        Update. The current version of Visual Studio Community 2015 (as of 5/12/16)
        can be used to compile Earthworm and PsnAdSend. The Community version like
        the older Express version is free and it now includes the Microsoft Foundation 
	Class (MFC) libraries need to compile the Windows version of PsnAdSend.	

Here is a short how-to on installing Visual Studio Community 2015 and compiling EW:
	
        I had problems with the online install of Visual Studio Update 2 so I downloaded 
        the ISO version. It can be downloaded here
        https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx. On that 
        page you select the Visual Studio 2015 link and then Community 2015. You should see 
        an option to download the Web installer or ISO file. I selected the ISO file.

        The ISO file is around 7gb so you can't burn it to a standard DVD. To get around this 
        I unpacked the ISO file using 7-Zip (7zFM.exe). I then ran the vs_community.exe file 
        located in the unpacked ISO file directory. In the setup program

        1. I ignore the IE Explorer 10 warning
        2. Set the install location and select Custom install
        3. Under Program Languages I select Visual C++
        4. Under Visual C++ I make sure that the three check boxes are checked, one being MFC for C++
        5. Optionally select Python Tools
        6. Press Next and then Install
	
        Once you have installed VS you may need to update the ew_nt.cmd file located in the
        environment sub-directory to the path of the C compiler. As an example I installed
        VS Community into the c:\apps\MVStudio directory. The following line under "Set up 
        Visual C++ compilation environment" was added to the file: 
	      
              call "c:\apps\MVStudio\VC\bin\vcvars32.bat"
        
	You can now run the ew_nt.cmd file to setup your EW environment. To compile EW move
        to the src directory, run 'nmake clean_nt' and then 'nmake nt'. If you do not have a fortran
        compiler you will need to comment out the two fortran based files hyp2000 and hyp2000_mgr under
        ALL_MODULES and NT_MODULES before you compile EW. 
-end
