@echo off
chcp 65001 > nul
echo ===================================================
echo   ShardsOfVeyara - DVC 및 Git Hooks 자동 설정
echo ===================================================
echo.

echo [1/3] Git Hooks 경로를 .githooks 로 설정 중...
git config core.hooksPath .githooks
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Git 설정을 적용하지 못했습니다. Git이 설치되어 있는지 확인해 주세요.
    pause
    exit /b 1
)
echo [OK] core.hooksPath 가 .githooks 로 설정되었습니다.
echo.

echo [2/3] DVC 설치 상태 확인 중...
where dvc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [경고] dvc 명령어를 찾을 수 없습니다!
    echo DVC 및 Google Drive 확장을 설치해 주세요:
    echo   pip install "dvc[gdrive]"
    echo 또는 DVC 공식 설치 프로그램을 이용해 주세요.
    echo.
) else (
    echo [OK] DVC가 설치되어 있습니다.
    dvc --version
)
echo.

echo [3/3] DVC 원격 저장소(Google Drive) 연결 및 풀 테스트...
echo (처음 실행 시 브라우저에서 Google Drive 로그인 인증 창이 뜰 수 있습니다.)
dvc pull
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [참고] dvc pull 중 오류가 발생했거나 인증이 필요할 수 있습니다.
    echo 터미널에서 'dvc pull'을 직접 실행하여 구글 드라이브 로그인을 완료해 주세요.
) else (
    echo [OK] DVC 에셋이 성공적으로 최신화되었습니다.
)

echo.
echo ===================================================
echo   설정이 완료되었습니다!
echo   이제 git commit / git push / git pull 시
echo   DVC가 자동으로 동기화됩니다.
echo ===================================================
echo.
pause
