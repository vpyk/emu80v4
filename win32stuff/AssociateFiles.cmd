set EXE=Emu80qt.exe
set EXE=%~dp0%EXE%

REG ADD HKCU\Software\Classes\.rk /t REG_SZ /d Emu80.rk /f
REG ADD HKCU\Software\Classes\Emu80.rk /t REG_SZ /d "Emu80 tape file" /f
REG ADD HKCU\Software\Classes\Emu80.rk\shell\open\command /t REG_SZ /d "\"%EXE%\" -r \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.rk\DefaultIcon /t REG_SZ /d %EXE%,1 /f

REG ADD HKCU\Software\Classes\.rkr /t REG_SZ /d Emu80.rkr /f
REG ADD HKCU\Software\Classes\Emu80.rkr /t REG_SZ /d "RK86 tape file" /f
REG ADD HKCU\Software\Classes\Emu80.rkr\shell\open\command /t REG_SZ /d "\"%EXE%\" -r \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.rkr\DefaultIcon /t REG_SZ /d %EXE%,2 /f

REG ADD HKCU\Software\Classes\.gam /t REG_SZ /d Emu80.gam /f
REG ADD HKCU\Software\Classes\Emu80.gam /t REG_SZ /d "RK86 game file" /f
REG ADD HKCU\Software\Classes\Emu80.gam\shell\open\command /t REG_SZ /d "\"%EXE%\" -r \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.gam\DefaultIcon /t REG_SZ /d %EXE%,3 /f

REG ADD HKCU\Software\Classes\.rka /t REG_SZ /d Emu80.rka /f
REG ADD HKCU\Software\Classes\Emu80.rka /t REG_SZ /d "Apogey tape file" /f
REG ADD HKCU\Software\Classes\Emu80.rka\shell\open\command /t REG_SZ /d "\"%EXE%\" -a \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.rka\DefaultIcon /t REG_SZ /d %EXE%,4 /f

REG ADD HKCU\Software\Classes\.rkp /t REG_SZ /d Emu80.rkp /f
REG ADD HKCU\Software\Classes\Emu80.rkp /t REG_SZ /d "Partner tape file" /f
REG ADD HKCU\Software\Classes\Emu80.rkp\shell\open\command /t REG_SZ /d "\"%EXE%\" -p \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.rkp\DefaultIcon /t REG_SZ /d %EXE%,5 /f

REG ADD HKCU\Software\Classes\.rkm /t REG_SZ /d Emu80.rkm /f
REG ADD HKCU\Software\Classes\Emu80.rkm /t REG_SZ /d "Mikrosha tape file" /f
REG ADD HKCU\Software\Classes\Emu80.rkm\shell\open\command /t REG_SZ /d "\"%EXE%\" -m \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.rkm\DefaultIcon /t REG_SZ /d %EXE%,6 /f

REG ADD HKCU\Software\Classes\.rk8 /t REG_SZ /d Emu80.rk8 /f
REG ADD HKCU\Software\Classes\Emu80.rk8 /t REG_SZ /d "Mikro-80 tape file" /f
REG ADD HKCU\Software\Classes\Emu80.rk8\shell\open\command /t REG_SZ /d "\"%EXE%\" -m80 \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.rk8\DefaultIcon /t REG_SZ /d %EXE%,7 /f

REG ADD HKCU\Software\Classes\.rku /t REG_SZ /d Emu80.rku /f
REG ADD HKCU\Software\Classes\Emu80.rku /t REG_SZ /d "UT-88 tape file" /f
REG ADD HKCU\Software\Classes\Emu80.rku\shell\open\command /t REG_SZ /d "\"%EXE%\" -u \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.rku\DefaultIcon /t REG_SZ /d %EXE%,8 /f

REG ADD HKCU\Software\Classes\.rks /t REG_SZ /d Emu80.rks /f
REG ADD HKCU\Software\Classes\Emu80.rks /t REG_SZ /d "Specialist tape file" /f
REG ADD HKCU\Software\Classes\Emu80.rks\shell\open\command /t REG_SZ /d "\"%EXE%\" -s \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.rks\DefaultIcon /t REG_SZ /d %EXE%,9 /f

REG ADD HKCU\Software\Classes\.rko /t REG_SZ /d Emu80.rko /f
REG ADD HKCU\Software\Classes\Emu80.rko /t REG_SZ /d "Orion tape file" /f
REG ADD HKCU\Software\Classes\Emu80.rko\shell\open\command /t REG_SZ /d "\"%EXE%\" -o \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.rko\DefaultIcon /t REG_SZ /d %EXE%,10 /f

REG ADD HKCU\Software\Classes\.bru /t REG_SZ /d Emu80.bru /f
REG ADD HKCU\Software\Classes\Emu80.bru /t REG_SZ /d "Orion BRU file" /f
REG ADD HKCU\Software\Classes\Emu80.bru\shell\open\command /t REG_SZ /d "\"%EXE%\" -o \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.bru\DefaultIcon /t REG_SZ /d %EXE%,11 /f

REG ADD HKCU\Software\Classes\.ord /t REG_SZ /d Emu80.ord /f
REG ADD HKCU\Software\Classes\Emu80.ord /t REG_SZ /d "Orion ORD file" /f
REG ADD HKCU\Software\Classes\Emu80.ord\shell\open\command /t REG_SZ /d "\"%EXE%\" -o \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.ord\DefaultIcon /t REG_SZ /d %EXE%,12 /f

REG ADD HKCU\Software\Classes\.cpu /t REG_SZ /d Emu80.cpu /f
REG ADD HKCU\Software\Classes\Emu80.cpu /t REG_SZ /d "Specialist-MX CPU file" /f
REG ADD HKCU\Software\Classes\Emu80.cpu\shell\open\command /t REG_SZ /d "\"%EXE%\" -mx \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.cpu\DefaultIcon /t REG_SZ /d %EXE%,13 /f

REG ADD HKCU\Software\Classes\.rke /t REG_SZ /d Emu80.rke /f
REG ADD HKCU\Software\Classes\Emu80.rke /t REG_SZ /d "Eureka tape file" /f
REG ADD HKCU\Software\Classes\Emu80.rke\shell\open\command /t REG_SZ /d "\"%EXE%\" -eu \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.rke\DefaultIcon /t REG_SZ /d %EXE%,14 /f

REG ADD HKCU\Software\Classes\.lvt /t REG_SZ /d Emu80.lvt /f
REG ADD HKCU\Software\Classes\Emu80.lvt /t REG_SZ /d "Lvov tape file" /f
REG ADD HKCU\Software\Classes\Emu80.lvt\shell\open\command /t REG_SZ /d "\"%EXE%\" -lv \"%%1"\" /f 
REG ADD HKCU\Software\Classes\Emu80.lvt\DefaultIcon /t REG_SZ /d %EXE%,15 /f


set IE4UINIT=%SYSTEMROOT%\SYSTEM32\ie4uinit.exe
if exist %IE4UINIT% goto proceed

set IE4UINIT=%SYSTEMROOT%\SYSNATIVE\ie4uinit.exe

:proceed

for /f "tokens=2 delims=[]" %%v in ('ver') do set WINVER=%%v
rem set WINVER=%WINVER:Version =%
for /f "tokens=2 delims=. " %%v in ('echo %WINVER%') do set WINVER=%%v

if %WINVER% geq 10 goto ten
%IE4UINIT% -ClearIconCache
goto end

:ten                                                              
%IE4UINIT% -show

:end
