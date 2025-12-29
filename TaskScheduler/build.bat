@echo off
echo 正在构建 TaskScheduler 项目...
echo.

REM 检查是否安装了Visual Studio编译器
if defined VCINSTALLDIR (
    echo 使用已配置的Visual Studio环境
) else (
    echo 正在查找Visual Studio安装...
    REM 尝试调用Visual Studio的开发者命令提示符环境
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
    if errorlevel 1 (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
    )
    if errorlevel 1 (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
    )
    if errorlevel 1 (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
    )
    if errorlevel 1 (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
    )
    if errorlevel 1 (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
    )
    if errorlevel 1 (
        echo 未找到Visual Studio安装。请确保已安装Visual Studio并配置了环境。
        pause
        exit /b 1
    )
)

REM 编译程序
cl scheduler_simulator.c /Fe:TaskScheduler.exe /W3 /nologo
if errorlevel 1 (
    echo 构建失败。
    pause
    exit /b 1
)

echo.
echo 构建成功！
echo.
echo 现在可以运行 TaskScheduler.exe
echo.
pause