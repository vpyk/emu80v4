REG DELETE HKCU\Software\Classes\.rk /f
REG DELETE HKCU\Software\Classes\Emu80.rk /f

REG DELETE HKCU\Software\Classes\.rkr /f
REG DELETE HKCU\Software\Classes\Emu80.rkr /f

REG DELETE HKCU\Software\Classes\.gam /f
REG DELETE HKCU\Software\Classes\Emu80.gam /f

REG DELETE HKCU\Software\Classes\.rka /f
REG DELETE HKCU\Software\Classes\Emu80.rka /f

REG DELETE HKCU\Software\Classes\.rkp /f
REG DELETE HKCU\Software\Classes\Emu80.rkp /f

REG DELETE HKCU\Software\Classes\.rkm /f
REG DELETE HKCU\Software\Classes\Emu80.rkm /f

REG DELETE HKCU\Software\Classes\.rk8 /f
REG DELETE HKCU\Software\Classes\Emu80.rk8 /f

REG DELETE HKCU\Software\Classes\.rku /f
REG DELETE HKCU\Software\Classes\Emu80.rku /f

REG DELETE HKCU\Software\Classes\.rks /f
REG DELETE HKCU\Software\Classes\Emu80.rks /f

REG DELETE HKCU\Software\Classes\.rko /f
REG DELETE HKCU\Software\Classes\Emu80.rko /f

REG DELETE HKCU\Software\Classes\.bru /f
REG DELETE HKCU\Software\Classes\Emu80.bru /f

REG DELETE HKCU\Software\Classes\.ord /f
REG DELETE HKCU\Software\Classes\Emu80.ord /f

REG DELETE HKCU\Software\Classes\.cpu /f
REG DELETE HKCU\Software\Classes\Emu80.cpu /f

REG DELETE HKCU\Software\Classes\.rke /f
REG DELETE HKCU\Software\Classes\Emu80.rke /f

REG DELETE HKCU\Software\Classes\.lvt /f
REG DELETE HKCU\Software\Classes\Emu80.lvt /f

REG DELETE HKCU\Software\Classes\.rk4 /f
REG DELETE HKCU\Software\Classes\Emu80.rk4 /f


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
