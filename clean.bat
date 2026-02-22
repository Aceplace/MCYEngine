@echo off
echo Cleaning build artifacts...

if exist *.obj del /q *.obj
if exist *.ilk del /q *.ilk
if exist vc140.pdb del /q vc140.pdb