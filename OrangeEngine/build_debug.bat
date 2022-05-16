
:: Build LinkerBuildTest project first
"C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\amd64\MSBuild.exe" OrangeEngine.sln -target:LinkerBuildTests:Rebuild /property:Configuration=Debug

:: Build the OrangeEngine project next
::"C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\amd64\MSBuild.exe" OrangeEngine.sln -target:OrangeEngine:Rebuild /property:Configuration=Debug