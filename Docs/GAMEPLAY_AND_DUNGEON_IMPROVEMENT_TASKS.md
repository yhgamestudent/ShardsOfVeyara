# [내일 작업 예정] 게임플레이 및 던전 맵 6대 개선 과제 구현 가이드

**문서 위치**: `Docs/GAMEPLAY_AND_DUNGEON_IMPROVEMENT_TASKS.md`  
**작성일**: 2026-10-20  
**상태**: **작업 대기 (Ready for Implementation)**  

---

## 📋 과제 요약 및 작업 체크리스트

- [ ] **Task 1. 공격 직후 이동 시 애니메이션 블렌드 안 되는 현상 복구**
- [ ] **Task 2. 잡초 뽑을 때 손에 포크 쥔 채 뽑는 현상 (임시 숨김 처리)**
- [ ] **Task 3. 튜토리얼 마을의 밭이 거점 맵의 밭과 연동되는 버그 (맵별 키 분리)**
- [ ] **Task 4. 던전 맵 몬스터들의 체력 상향 조정 (`DT_CombatStats.csv`)**
- [ ] **Task 5. 던전 맵 스포너의 몬스터 미처치 시 원거리 사운드 누출 해결 (`PlaySoundAtLocation`)**
- [ ] **Task 6. 독극물 점프맵 벽돌 콜리전 돌출 현상 (스태틱 메시 콜리전 재조정)**
- [ ] **Bonus Task. 스켈레톤 메이지 2페이즈 연속 연출/스킬로 인한 과도한 프리딜 타임 개선**

---

## 🛠️ 세부 분석 및 구현 가이드

### 1. 공격 직후 움직일 때 블렌드 안 되는 현상
* **문제 파일**: [`Source/AGSD/Character/AGSDCharacter.cpp`](file:///Source/AGSD/Character/AGSDCharacter.cpp)
* **원인**:
  피격 넉백 루트모션 제거를 위해 `StopAnimMontage` 시작부에 추가했던 `AnimInstance->StopAllMontages(0.0f)` 때문에,
  공격 후딜레이 이동 캔슬(`bCanMoveCancel` [L914](file:///Source/AGSD/Character/AGSDCharacter.cpp#L914)) 시에도 몽타주가 $0$초 만에 뚝 끊겨 걷기 모션과의 블렌딩이 사라짐.
* **해결 방안**:
  * `StopAnimMontage`의 기본 호출은 언리얼 기본 블렌드아웃(자연스러운 보간)을 유지하도록 복구.
  * 보스 넉백 피격 시에만 즉시 루트 모션을 끊는 전용 처리(`bForceImmediateMontageStop` 플래그 또는 별도 함수)로 분리.

---

### 2. 잡초를 뽑을 때 손에 포크를 쥐고 뽑는 현상
* **문제 파일**: [`Source/AGSD/FarmingSystem/Weeds.cpp`](file:///Source/AGSD/FarmingSystem/Weeds.cpp)
* **원인**:
  `Weeds::Interact_Implementation`([L150](file:///Source/AGSD/FarmingSystem/Weeds.cpp#L150))에서 잡초 뽑기 몽타주(`PlayPullPlant`)를 재생할 때, 손에 쥐고 있는 무기(`HoldingActor`)를 숨기거나 등에 넣는 처리가 누락됨.
* **해결 방안**:
  * `Interact_Implementation` 시작 시:
    ```cpp
    if (player->HoldingActor)
    {
        player->HoldingActor->SetActorHiddenInGame(true);
    }
    ```
  * `OnMontageEnded`([L110](file:///Source/AGSD/FarmingSystem/Weeds.cpp#L110)) 종료 시:
    ```cpp
    if (Player && Player->HoldingActor)
    {
        Player->HoldingActor->SetActorHiddenInGame(false);
    }
    ```

---

### 3. 튜토리얼 마을의 밭이 거점 맵의 밭과 연동되는 현상
* **문제 파일**: [`Source/AGSD/FarmingSystem/ACultivationPlot.cpp`](file:///Source/AGSD/FarmingSystem/ACultivationPlot.cpp)
* **원인**:
  `BeginPlay`([L236](file:///Source/AGSD/FarmingSystem/ACultivationPlot.cpp#L236)) 및 `EndPlay`([L278](file:///Source/AGSD/FarmingSystem/ACultivationPlot.cpp#L278))에서 `GI->GetPlotData(GetName(), LoadedData)`를 호출하여, 맵 구분 없이 액터 이름(`GetName()`)만 키로 사용하여 튜토리얼 맵의 밭 데이터가 거점 맵으로 그대로 로드됨.
* **해결 방안**:
  * 맵 이름을 포함한 고유 키 생성 헬퍼 함수 적용:
    ```cpp
    FString AACultivationPlot::GetPlotSaveKey() const
    {
        FString MapName = GetWorld() ? GetWorld()->GetMapName() : TEXT("");
        MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
        return FString::Printf(TEXT("%s_%s"), *MapName, *GetName());
    }
    ```
  * `GI->GetPlotData(GetPlotSaveKey(), LoadedData)` 및 저장 시에도 `SaveData.PlotName = GetPlotSaveKey()` 사용.

---

### 4. 던전 맵 몬스터들의 체력이 너무 낮은 현상
* **문제 파일**: [`DataTable/DT_CombatStats.csv`](file:///DataTable/DT_CombatStats.csv)
* **원인**:
  플레이어의 평타 1콤보 데미지가 약 $350$인데, 잡몹들의 HP가 $160\sim 260$으로 설정되어 있어 스치기만 해도 죽어버림.
* **해결 방안**:
  * `Enemy_Forest_Bee` (벌): HP $180 \rightarrow \mathbf{420}$ (목표 TTK 6초)
  * `Enemy_BaseMelee` (근접 쥐): HP $260 \rightarrow \mathbf{550}$ (목표 TTK 8초)
  * `Enemy_BaseRanged` (원거리): HP $220 \rightarrow \mathbf{450}$ (목표 TTK 6초)
  * `Enemy_BasePassive` (버섯): HP $240 \rightarrow \mathbf{480}$
  * `Enemy_BaseTranspar` (뱀): HP $240 \rightarrow \mathbf{520}$

---

### 5. 던전 맵 스포너의 몬스터 미처치 시 원거리 사운드 누출
* **문제 파일**: [`Plugins/Enemy/Source/Enemy/Private/EnemySpawner/EnemySpawner.cpp`](file:///Plugins/Enemy/Source/Enemy/Private/EnemySpawner/EnemySpawner.cpp)
* **원인**:
  스포너에서 스폰(`SpawnSound` [L295](file:///Plugins/Enemy/Source/Enemy/Private/EnemySpawner/EnemySpawner.cpp#L295)) 및 사망(`DeadSound` [L314](file:///Plugins/Enemy/Source/Enemy/Private/EnemySpawner/EnemySpawner.cpp#L314)) 시 `PlaySound2D`를 호출하여 전 맵의 플레이어 귀에 거리 감쇠 없이 2D로 쩌렁쩌렁 울림.
* **해결 방안**:
  * `UGameplayStatics::PlaySoundAtLocation`으로 변경하고 스포너 위치(`GetActorLocation()`) 및 사운드 감쇠(Attenuation Settings) 지정.

---

### 6. 독극물 점프맵 벽돌 콜리전 돌출 현상
* **원인**:
  점프맵에 배치된 벽돌 스태틱 메시의 콜리전이 자동 생성된 거대 직육면체(Box)로 설정되어 있어, 비주얼 모델링 바깥으로 콜리전이 튀어나와 플레이어의 낙하/도약 경로를 방해함.
* **해결 방안 (에디터 작업)**:
  1. 에디터 콘텐츠 브라우저에서 해당 벽돌 스태틱 메시 더블 클릭.
  2. `Collision Complexity` 설정을 **`Use Complex Collision As Simple`**로 변경하여 시각적 메쉬 폴리곤 그대로 충돌하게 하거나,
  3. 기존 단순 박스 콜리전을 제거하고 메시 크기에 딱 맞게 콜리전 박스를 재설정.
