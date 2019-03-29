METHODS OF USING diskinfo.c disklist.c diskget.c diskput.c 

disk: contains 6 files  
      diskinfo.c disklist.c diskget.c diskput.c 
      makefile 
      testImage.IMA

Fist:  type 'make' to compile all four files 
       diskinfo.c disklist.c diskget.c diskput.c 

Get information of disk: 

	./diskinfo name.IMA 

	EG: ./diskinfo testImage.IMA

Get list of disk:

	./disklist name.IMA 

	EG: ./disklist testImage.IMA

Get file from disk:

	./diskget name.IMA filename 

	EG: ./diskget testImage.IMA test.txt 

Put file into disk memory:

        ./diskput name.IMA filename 

	EG: ./diskput testImage.IMA test.txt 


The format of showing subdirectory for disklist:

EG: Between two SUBLAYER is file among subdirectory 

root directory
==================
 F        540                 MFS.H   04-11-2018 09:38
 F       1910            README.TXT   04-11-2018 09:38
 D          0              SUBLAYER   00-00-1980 00:00
     (SUBLAYER)==================
     F        579                 MSGSEND.C   04-11-2018 09:39
     (SUBLAYER)==================
 F        145              TEST.TXT   04-11-2018 09:38






