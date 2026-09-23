@echo off
cd /d "%~dp0"
if exist "build\stratego-campaign.exe" (
    start "" "build\stratego-campaign.exe"
) else (
    echo Candidat de campagne absent. Voir README.md, section Campagne IA.
    pause
)
