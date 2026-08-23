#!/bin/bash
echo "==================================================="
echo "  ShardsOfVeyara - DVC 및 Git Hooks 자동 설정"
echo "==================================================="
echo ""

echo "[1/3] Git Hooks 경로를 .githooks 로 설정 중..."
git config core.hooksPath .githooks
if [ $? -ne 0 ]; then
    echo "[ERROR] Git 설정을 적용하지 못했습니다."
    exit 1
fi
echo "[OK] core.hooksPath 가 .githooks 로 설정되었습니다."
echo ""

echo "[2/3] DVC 설치 상태 확인 중..."
if ! command -v dvc >/dev/null 2>&1; then
    echo "[경고] dvc 명령어를 찾을 수 없습니다!"
    echo "DVC 및 Google Drive 확장을 설치해 주세요: pip install 'dvc[gdrive]'"
else
    echo "[OK] DVC가 설치되어 있습니다."
    dvc --version
fi
echo ""

echo "[3/3] DVC 원격 저장소(Google Drive) 연결 및 풀 테스트..."
dvc pull

echo ""
echo "==================================================="
echo "  설정이 완료되었습니다!"
echo "==================================================="
