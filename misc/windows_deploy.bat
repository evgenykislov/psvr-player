# Composing

pushd ..
rd /S /Q deploy_win_release
mkdir deploy_win_release

copy build-win-release\psvrplayer\Release\psvrplayer.exe deploy_win_release\
copy C:\libhidapi\x64\hidapi.dll deploy_win_release\
xcopy /E C:\libvlc deploy_win_release\
rename deploy_win_release\README.txt vlc_readme.txt
rename deploy_win_release\THANKS.txt vlc_thanks.txt
copy misc\windows_readme.md deploy_win_release\README.md
copy misc\windows_readme_rus.md deploy_win_release\README_RUS.md

popd
