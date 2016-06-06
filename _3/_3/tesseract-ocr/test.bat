@echo off
FOR /L %%x IN (1, 1, %1) DO (	
	tesseract.exe %%x.png out%%x% -l num -psm 10
	@echo on
	echo %%x
	@echo off
	type out%%x%.txt
)