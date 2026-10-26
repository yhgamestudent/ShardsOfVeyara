# [구현 계획서] 글로벌 포션 공유 쿨다운 및 머티리얼 프로그레스바 UI 연동 시스템

---

## 1. 개요 및 요구사항

1. **글로벌 포션 공유 쿨다운 (Shared Global Potion Cooldown)**:
   - 하급, 중급, 중급 치유 물약 등 어떤 종류의 물약이든 하나를 마시면 모든 물약이 공통 쿨다운(기본 5.0초)에 진입.
   - 쿨다운 중에는 다른 어떤 종류의 물약도 사용 불가.
2. **머티리얼 인스턴스 프로그레스바 UI 동시 연동**:
   - 핫바 및 가방 인벤토리 내의 **모든 물약 슬롯에 머티리얼 인스턴스로 제작된 프로그레스바(Circular/Radial)가 동시에 회전**.
   - Dynamic Material Instance(DMI)의 Scalar Parameter(기본값: `Percent`)에 정규화된 쿨다운 비율(`0.0f ~ 1.0f`)을 실시간 전달.
   - 포션이 아닌 일반 아이템 슬롯은 오버레이를 자동으로 숨김(`Hidden` / `Collapsed`).
3. **슬롯 간 아이템 이동 시 쿨다운 UI 완벽 동기화 (사용자 신규 요구사항)**:
   - 쿨다운 도중 사용자가 포션을 핫바 내 다른 칸이나 가방 인벤토리의 다른 슬롯으로 옮기더라도(드래그 앤 드롭, 스왑, 퀵슬롯 교체 등):
     - 이동된 새 슬롯이 포션임을 즉시 감지하여 **현재 남아있는 쿨다운 비율을 그대로 이어받아 게이지가 회전**.
     - 포션이 빠져나간 원래 슬롯은 일반 아이템/빈 슬롯으로 변경되므로 쿨다운 게이지가 즉시 사라짐.
4. **실시간 체력 회복 로거 (`PlayerHealLog`) 동시 구축**:
   - `Saved/Logs/PlayerHealLogs/PlayerHealLog_<TimeStamp>.csv`에 실시간 Append 연동.
5. **하급 치유 물약 & 미믹 밸런스 패치**:
   - 하급 치유 물약 회복량 상향 (`40 HP ➡️ 60 HP`).
   - 미믹(BaseMimic) 전용 드롭 테이블 분리 (금괴 1개(100%), 중급 치유 물약(50%), 차원 크리스탈 2~3개(100%), 당근/비트 씨앗 각 1~2개(70%)).

---

## 2. SOLID 원칙 기반 아키텍처 설계

```
              ┌────────────────────────────────────────────────────────┐
              │                    AAGSDCharacter                      │
              │  - GlobalPotionCooldown: 5.0f                          │
              │  - LastGlobalPotionUseTime: float                      │
              │  + GetPotionCooldownRatio(): float (0.0f ~ 1.0f)       │
              │  + GetPotionCooldownRemaining(): float (seconds)       │
              │  + IsPotionOnCooldown(): bool                          │
              │  + IsPotionItem(const FStruct_ItemData&): bool         │
              └───────────────────────────▲────────────────────────────┘
                                          │ 상태 쿼리 (의존성 최소화)
              ┌───────────────────────────┴────────────────────────────┐
              │                  UAGSDSlotWidgetBase                   │
              │  (핫바 / 인벤토리 공통 슬롯 베이스 C++ 클래스)           │
              │  - IMG_CooldownOverlay: UImage (머티리얼 인스턴스 브러시)│
              │  - CooldownParamName: FName ("Percent")                │
              │  - CooldownDynamicMaterial: UMaterialInstanceDynamic*  │
              │  + SetItemData(): 아이템 교체 시 쿨다운 비주얼 즉각 반응 │
              │  + UpdateVisual(): 슬롯 데이터 변경 시 오버레이 동기화 │
              │  + NativeTick(): 포션 슬롯일 때 실시간 DMI 파라미터 갱신│
              └─────────────┬────────────────────────────┬─────────────┘
                            │                            │
             ┌──────────────▼─────────────┐ ┌────────────▼──────────────┐
             │   UAGSDHotbarSlotWidget    │ │  UAGSDInventorySlotWidget │
             │   (WBP_HotbarSlot)         │ │  (WBP_InventorySlot)      │
             └────────────────────────────┘ └───────────────────────────┘
```

### [SRP] 단일 책임 원칙 (Single Responsibility Principle)
- **`AAGSDCharacter`**: 쿨다운 시간 측정 및 사용 허가/차단 로직만 책임집니다.
- **`UAGSDSlotWidgetBase`**: 슬롯 UI 표현(아이콘, 수량, 머티리얼 인스턴스 파라미터 전달)만 책임집니다.
- **`UGameplayLogSubsystem`**: 실시간 CSV 기록 및 파일 스트림 관리만 책임집니다.

### [OCP] 개방-폐쇄 원칙 (Open-Closed Principle)
- 머티리얼의 파라미터 명칭(`CooldownParamName`)을 UPROPERTY로 외부에 열어두어, 향후 머티리얼 파라미터 이름이 바뀌어도 C++ 코드를 수정할 필요가 없습니다.
- `IsPotionItem()` 함수를 통해 향후 마나 물약, 강화 영약 등이 추가되어도 기존 로직 변경 없이 공유 쿨다운 및 UI 타겟에 자연스럽게 흡수됩니다.

### [LSP] 리스코프 치환 원칙 (Liskov Substitution Principle)
- 모든 쿨다운 UI 처리 로직은 공통 베이스인 `UAGSDSlotWidgetBase`에 존재하므로, 이를 파생한 `UAGSDHotbarSlotWidget`과 `UAGSDInventorySlotWidget` 모두 완벽히 동일한 동작 규약을 따릅니다.

### [ISP] 인터페이스 분리 원칙 (Interface Segregation Principle)
- 위젯은 오직 `GetPotionCooldownRatio()`와 `IsPotionOnCooldown()` 등 꼭 필요한 최소한의 인터페이스만 조회하며, 캐릭터 내부의 불필요한 세부 전투 로직을 알 필요가 없습니다.

### [DIP] 의존성 역전 원칙 (Dependency Inversion Principle)
- 슬롯 위젯은 구체적인 조작 계통에 직접 결합되지 않고 소유자 폰(`GetOwningPlayerPawn`)의 공개 인터페이스를 통해 쿨다운 정보를 주입받습니다.

---

## 3. 포션 이동 시 쿨다운 UI 동기화 메커니즘 (핵심 해결책)

> **문제 인식**:
> 특정 슬롯 인덱스(예: 1번 칸)에 쿨타임을 종속시키면, 플레이어가 1번 칸의 물약을 3번 칸으로 드래그 앤 드롭했을 때 3번 칸에는 쿨타임이 안 돌거나 1번 칸에 빈 상태로 게이지가 도는 문제가 발생할 수 있습니다.

> **해결 메커니즘**:
> 1. **아이템 데이터 기반 상태 판별**:
>    각 슬롯 위젯은 매 프레임 또는 데이터 갱신 시 자신이 현재 들고 있는 `SlotItemData`가 포션인지 여부(`IsPotionItem`)를 실시간으로 확인합니다.
> 2. **글로벌 공유 쿨다운 비율 동기화**:
>    슬롯이 포션을 들고 있다면, 캐릭터의 글로벌 잔여 쿨다운 비율(`GetPotionCooldownRatio()`)을 머티리얼에 바로 전달합니다.
> 3. **슬롯 간 이동 즉각 반영**:
>    - 포션을 A 슬롯에서 B 슬롯으로 옮길 때, `SetItemData` 및 `UpdateVisual`이 양쪽 슬롯에서 호출됩니다.
>    - **A 슬롯 (원래 칸)**: 빈 슬롯 또는 다른 아이템이 들어오므로 `IMG_CooldownOverlay`가 즉시 `Hidden` 처리됩니다.
>    - **B 슬롯 (새 칸)**: 포션 데이터가 들어오는 즉시 `NativeTick`에서 현재 돌고 있는 글로벌 쿨다운 비율을 이어받아 머티리얼 프로그레스바가 매끄럽게 회전합니다.

---

## 4. 세부 구현 상세

### 4.1 캐릭터 포션 쿨다운 (`Source/AGSD/Character/AGSDCharacter.h / .cpp`)
1. **헤더 선언**:
   ```cpp
   // 글로벌 포션 공유 쿨다운 시간 (기본 5.0초)
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Potion")
   float GlobalPotionCooldown = 5.0f;

   // 마지막으로 포션을 복용한 게임 내 시간초
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Potion")
   float LastGlobalPotionUseTime = -100.0f;

   UFUNCTION(BlueprintPure, Category = "Combat|Potion")
   float GetPotionCooldownRatio() const;

   UFUNCTION(BlueprintPure, Category = "Combat|Potion")
   float GetPotionCooldownRemaining() const;

   UFUNCTION(BlueprintPure, Category = "Combat|Potion")
   bool IsPotionOnCooldown() const;

   UFUNCTION(BlueprintPure, Category = "Combat|Potion")
   bool IsPotionItem(const FStruct_ItemData& ItemData) const;
   ```
2. **구현 로직**:
   - `IsPotionItem`: `ItemData.EquipHoldingState == EHoldingState::EHS_Potion || ItemData.ItemID.Contains(TEXT("Health")) || ItemData.ItemID.Contains(TEXT("Heal"))`
   - `IsPotionOnCooldown`: `(GetWorld()->GetTimeSeconds() - LastGlobalPotionUseTime) < GlobalPotionCooldown`
   - `GetPotionCooldownRatio`: `FMath::Clamp(Remaining / GlobalPotionCooldown, 0.0f, 1.0f)` (1.0에서 0.0으로 감소)
   - `Input_PrimaryAttack`: 포션 사용 전 `IsPotionOnCooldown()` 체크하여 쿨타임 중이면 `return;`, 사용 성공 시 `LastGlobalPotionUseTime = GetWorld()->GetTimeSeconds();` 갱신.

### 4.2 슬롯 위젯 머티리얼 인스턴스 연동 (`Source/AGSD/Inventory/UI/AGSDSlotWidgetBase.h / .cpp`)
1. **바인딩 및 프로퍼티**:
   ```cpp
   UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "UI|Cooldown")
   TObjectPtr<UImage> IMG_CooldownOverlay;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Cooldown")
   FName CooldownParamName = FName(TEXT("Percent"));

   UPROPERTY(Transient)
   TObjectPtr<UMaterialInstanceDynamic> CooldownDynamicMaterial;
   ```
2. **`NativeTick` 갱신 로직**:
   - 현재 슬롯에 아이템이 있고 포션 아이템인 경우:
     - 캐릭터의 `IsPotionOnCooldown()` 검사.
     - 쿨다운 중이면: `IMG_CooldownOverlay`를 `ESlateVisibility::HitTestInvisible`로 표시하고, DMI의 `CooldownParamName` 스칼라 파라미터에 `GetPotionCooldownRatio()` 전달.
     - 쿨다운이 끝났으면: `IMG_CooldownOverlay`를 `ESlateVisibility::Hidden`으로 숨김.
   - 포션 아이템이 아니거나 빈 슬롯인 경우:
     - `IMG_CooldownOverlay`를 즉시 `ESlateVisibility::Hidden`으로 숨김.

### 4.3 실시간 체력 회복 로거 (`GameplayLogSubsystem`)
1. **헤더 선언**:
   ```cpp
   UFUNCTION(BlueprintCallable, Category = "Logging")
   void LogPlayerHealed(const FVector& PlayerLoc, const FString& SourceName, float HealAmount, float PrevHealth, float CurrentHealth, float MaxHealth);
   ```
2. **로그 파일 위치**: `Saved/Logs/PlayerHealLogs/PlayerHealLog_<TimeStamp>.csv`
3. **`AAGSDCharacter::HealthRecovery` 연동**:
   - `ActualHealed > 0.0f`일 때 서브시스템의 `LogPlayerHealed` 호출.

### 4.4 밸런스 패치
1. `DataTable/DT_InventoryItemData.csv`: 하급 치유 물약 회복량 `40.0 ➡️ 60.0` 상향.
2. `DataTable/DT_EnemyItemDrop.csv`: `DungeonMimic` 분리 (금괴 1개(100%), 중급 치유 물약(50%), 차원 크리스탈 2~3개(100%), 당근/비트 씨앗 각 1~2개(70%)).

---

## 5. UI 블루프린트 연동 방법 (작업 완료 후 설정)
1. `WBP_HotbarSlot` 및 `WBP_InventorySlot` 에디터를 엽니다.
2. `IMG_ItemIcon` 위에 새 `Image` 위젯을 얹고 이름을 **`IMG_CooldownOverlay`** 로 지정합니다.
3. 해당 Image의 Brush에 제작하신 **머티리얼 인스턴스(Circular ProgressBar MI)**를 지정합니다.
4. 머티리얼의 Scalar Parameter 이름이 `Percent`가 아니라면 디테일 패널의 `CooldownParamName`을 해당 이름으로 맞추시면 됩니다.

- **중급 치유 물약**: +100 HP 고정 회복 (봉헌 3단계 체력 1,000 기준 20% 회복)

## 6. 봉헌 6단계 마일스톤 스펙업 확정
- **봉헌 3단계**: ATK 100.0 / HP 1,000.0 (숲 맵 클리어 마일스톤)
- **봉헌 6단계**: **ATK 150.0 / HP 1,250.0** (던전 맵 2회차 보스 도전 마일스톤, ATK +30 / HP +200 대폭 강화)
- **보스전 적정 스펙**: 봉헌 6단계 + 중급 치유 물약(+100 HP) 3개


## 7. 상자 2개 보상 및 던전 2회차 황금 밸런스 확정
- **상자 A (1번 상자)**: GoldIngots 1개, BeetSeed 2개, CarrotSeed 2개, GreaterHealingPotion 1개
- **상자 B (2번 상자)**: GoldIngots 1개, BeetSeed 2개, CarrotSeed 2개 (물약 없음)
- **던전 2회 완료 시 최종 결산**:
  - 총 비트 수확: 약 9~10개
  - 봉헌 4~6단계: 비트 8개 소모 ➡️ **봉헌 6단계 마일스톤(ATK 150 / HP 1,250) 확정 달성!**
  - 남은 비트 1~2개: **중급 치유 물약 1~2개 연금술 제작**
  - 상자 완제품 물약: **2개 획득**
  - **보스전 진입 시 최종 물약 보유량: 중급 치유 물약(+100 HP) 딱 3~4개! (총 +600~800 HP 회복, 5초 쿨타임과 최상의 긴장감 조화)**


## 8. 차원 크리스탈(DimensionCrystal) 희소성 밸런스 패치
- **보스 드롭 축소**: 5~10개 ➡️ **3 ~ 5개**
- **숲 일반 몬스터**: 50% ➡️ **30% 확률 (1개)**
- **던전 일반 몬스터**: 50%(1~3개) ➡️ **30% 확률 (1~2개)**
- **미믹**: 2~3개 ➡️ **1 ~ 2개**
- **최종 효과**: 숲 1회 완주 시 크리스탈 약 4~5개, 던전 1회 완주 시 약 3~4개로 튜닝되어, 물약 제작 시 크리스탈의 촉매 가치와 희소성이 확립됨.
