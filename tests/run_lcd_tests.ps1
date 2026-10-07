# Author: thuanngo
param([string]$MsvcRoot = 'D:\VStool\VC\Tools\MSVC\14.51.36231')
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
Push-Location $repo
try {
    $out = Join-Path $repo 'Objects/lcd_tests'
    New-Item -ItemType Directory -Force -Path $out | Out-Null
    $sources = @('Hardware/st7735.c','Hardware/font.c','tests/test_lcd.c')
    $objects = @()
    foreach ($src in $sources) {
        $obj = Join-Path $out (([IO.Path]::GetFileNameWithoutExtension($src)) + '.obj')
        & "$MsvcRoot/bin/Hostx64/x64/cl.exe" /nologo /c /TC /GS- /Zl /Od /W3 /utf-8 "/I$MsvcRoot/include" /Itests/lcd_mock /IHardware "/Fo$obj" $src
        if ($LASTEXITCODE -ne 0) { throw "Host compilation failed: $src" }
        $objects += $obj
    }
    $dll = Join-Path $out 'lcd_test.dll'
    & "$MsvcRoot/bin/Hostx64/x64/link.exe" /nologo /dll /noentry /nodefaultlib "/out:$dll" $objects
    if ($LASTEXITCODE -ne 0) { throw 'Host test link failed' }
    python -c 'import ctypes,sys; d=ctypes.CDLL(sys.argv[1]); r=d.LCD_RunTests(); f=d.LCD_GetMockFault(); print(f"LCD regression: result={r}, mock_fault={f} (0 = PASS)"); sys.exit(0 if r==0 and f==0 else 1)' $dll
    if ($LASTEXITCODE -ne 0) { throw 'LCD regression failed; result/mock_fault identifies a line in tests/test_lcd.c' }
} finally { Pop-Location }
