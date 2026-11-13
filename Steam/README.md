# Steam 업로드 (SteamPipe)

## 폴더 구성
```
Steam/
 ├ scripts/
 │   ├ app_build.vdf              ← AppID, Depot ID 입력
 │   └ depot_build_windows.vdf    ← Depot ID 입력, 업로드 제외 파일 목록
 ├ content/                       ← 패키징 결과물(Windows 폴더 안 내용)을 여기에 복사
 ├ output/                        ← steamcmd 로그 (자동 생성)
 └ upload.bat                     ← 업로드 실행
```
`content/`, `output/`은 git에 올라가지 않습니다(.gitignore).

## 처음 한 번만
1. Steamworks SDK를 받아서 `tools\ContentBuilder\builder\steamcmd.exe` 위치를 `upload.bat`의 `STEAMCMD`에 적습니다.
2. Steamworks 파트너 사이트에서 번호를 확인해 아래 세 곳의 `0000000` / `0000001`을 바꿉니다.
   - `scripts/app_build.vdf` → `"AppID"`, `"Depots"` 안의 Depot ID
   - `scripts/depot_build_windows.vdf` → `"DepotID"`
3. Steamworks > **설치(Installation) > 일반 설치**에서 실행 옵션을 추가합니다.
   - 실행 파일: `ShardsOfVeyara.exe`
   - 운영체제: Windows
4. Steamworks > **재배포 가능 패키지(Redistributables)**에서 *Visual C++ Redist 2015-2022*를 체크합니다.
   (언리얼 실행에 필요한 런타임을 Steam이 대신 설치해 줍니다.)

## 빌드할 때마다
1. 언리얼에서 패키징 (Windows) → 출력 폴더의 `Windows` 폴더 **안의 내용**을 `Steam/content/`에 복사합니다.
   `Steam/content/ShardsOfVeyara.exe`가 바로 보이면 올바르게 넣은 것입니다.
2. `Steam/upload.bat` 실행 → Steam 계정 이름 입력 → steamcmd가 비밀번호와 Steam Guard 코드를 물어봅니다.
3. 업로드가 끝나면 Steamworks > **SteamPipe > Builds**에서 새 빌드를 브랜치(default 또는 테스트 브랜치)에 **Set Build Live**.
4. 변경 사항을 게시(Publish)해야 플레이어에게 적용됩니다.

## 업로드에서 제외되는 파일
- `*.pdb` (디버그 심볼)
- `Manifest_DebugFiles_*.txt`
- `Saved` 폴더 (테스트 실행으로 생긴 로그·세이브·전투 로그)

## 처음 확인용
`app_build.vdf`의 `"Preview"`를 `"1"`로 바꾸고 실행하면 실제로 올리지 않고 업로드될 파일 목록만 `output/`에 남깁니다. 확인 후 다시 `"0"`으로 돌리세요.
