@echo off

msdev FariaRomEditor.dsw /MAKE "ALL - ALL"

dir /s/b *.exe
