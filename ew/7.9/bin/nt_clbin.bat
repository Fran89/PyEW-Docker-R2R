cd "%1" 
@echo "Cleaning binaries from %1"
nmake -f makefile.nt clean_bin
cd ..
