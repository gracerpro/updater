del /Q *.sdf
del /Q bin\*.pdb
del /Q bin\*.ilk

rmdir /S /Q ipch
rmdir /S /Q build\VisualStudio\Debug
rmdir /S /Q build\VisualStudio\Release

rmdir /S /Q build\codeblocks\obj

pause