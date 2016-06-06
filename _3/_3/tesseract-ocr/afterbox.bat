tesseract.exe dir\fingkey.font.exp0.tif dir\fingkey.font.exp0 box.train

tesseract.exe dir\fingkey.font.exp1.tif dir\fingkey.font.exp1 box.train

tesseract.exe dir\fingkey.font.exp2.tif dir\fingkey.font.exp2 box.train

tesseract.exe dir\fingkey.font.exp3.tif dir\fingkey.font.exp3 box.train

tesseract.exe dir\fingkey.font.exp4.tif dir\fingkey.font.exp4 box.train

tesseract.exe dir\fingkey.font.exp5.tif dir\fingkey.font.exp5 box.train

tesseract.exe dir\fingkey.font.exp6.tif dir\fingkey.font.exp6 box.train

unicharset_extractor.exe dir\fingkey.font.exp0.box dir\fingkey.font.exp1.box dir\fingkey.font.exp2.box dir\fingkey.font.exp3.box dir\fingkey.font.exp4.box dir\fingkey.font.exp5.box  dir\fingkey.font.exp6.box

mftraining.exe -F font_properties -U unicharset -O fingkey.unicharset dir\fingkey.font.exp0.tr dir\fingkey.font.exp1.tr dir\fingkey.font.exp2.tr dir\fingkey.font.exp3.tr dir\fingkey.font.exp4.tr dir\fingkey.font.exp5.tr dir\fingkey.font.exp6.tr
 
cntraining.exe dir\fingkey.font.exp0.tr dir\fingkey.font.exp1.tr dir\fingkey.font.exp2.tr dir\fingkey.font.exp3.tr dir\fingkey.font.exp4.tr dir\fingkey.font.exp5.tr dir\fingkey.font.exp6.tr