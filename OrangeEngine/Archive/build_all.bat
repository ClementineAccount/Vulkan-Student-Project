@echo off
echo "Building Debug..."
call build_debug.bat > nul
if %errorLevel% == 0 (echo "Debug build successful...") else (echo "Build failed. Rebuilding with log: " & call build_debug.bat & pause)

echo "Building Release..."
call build_release.bat > nul
if %errorLevel% == 0 (echo "Release build successful...") else (echo "Build failed. Rebuilding with log: " & call build_release.bat)
