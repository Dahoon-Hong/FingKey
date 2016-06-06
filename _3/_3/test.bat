@echo off
rem set TESSDATA_PREFIX=C:\Users\dahoon\Downloads\jTessBoxEditor\tesseract-ocr
FOR /L %%x IN (1, 1, %1) DO (	
	tesseract.exe %%x%.png out -l fingkey -psm 10
	@echo on
	echo %%x
	@echo off
	type out.txt
)