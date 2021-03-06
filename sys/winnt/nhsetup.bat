@REM  SCCS Id: @(#)nhsetup.bat      96/10/30                
@REM  Copyright (c) NetHack PC Development Team 1993, 1996
@REM  NetHack may be freely redistributed.  See license for details. 
@REM  Win32 setup batch file, see Install.nt for details
@REM
@echo off
echo Checking to see if directories are set up properly
if not exist ..\..\include\hack.h goto err_dir
if not exist ..\..\src\hack.c goto err_dir
if not exist ..\..\dat\wizard.des goto err_dir
if not exist ..\..\util\makedefs.c goto err_dir
if not exist ..\..\sys\winnt\winnt.c goto err_dir
echo Directories look ok.

:do_rest
echo "Copying Makefile.NT to ..\..\src\Makefile"
copy makefile.NT ..\..\src\Makefile >nul
echo Makefile copied ok.

if exist .\jnethack.ico goto hasicon
if exist .\nhico.uu uudecode nhico.uu >nul
if NOT exist .\jnethack.ico goto err_nouu
:hasicon
echo NetHack icon exists ok.
echo done!
echo.
echo Proceed with the next step documented in Install.nt
echo.
goto done
:err_nouu
echo Apparently you have no UUDECODE utility in your path.  You need a UUDECODE
echo utility in order to turn "nhico.uu" into "jnethack.ico".
echo Check "Install.nt" for a list of the steps required to build NetHack.
goto done
:err_plev
echo A required file ..\..\include\patchlev.h seems to be missing.
echo Check "Files." in the root directory for your NetHack distribution
echo and make sure that all required files exist.
goto done
:err_data
echo A required file ..\..\dat\data.bas seems to be missing.
echo Check "Files." in the root directory for your NetHack distribution
echo and make sure that all required files exist.
goto done
:err_dir
echo Your directories are not set up properly, please re-read the
echo documentation.
:done
