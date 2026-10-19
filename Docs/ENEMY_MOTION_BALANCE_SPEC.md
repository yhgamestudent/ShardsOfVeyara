# 📑 [명세서] 몬스터 모션밸런스 상세 사양서 (Monster Motion Balance Spec)

> 💡 **개요**: 플레이어의 창 조작 명세서(`DT_SpearComboData`)와 대칭되는 **몬스터 전용 데이터 주도 액션/모션 밸런스 명세서**입니다.  
> 몬스터의 애니메이션 선딜레이, 타격 판정, 후딜레이(딜타임), 데미지 배율, 가드 불가 여부, 모션 워핑 거리를 데이터테이블(`DT_EnemyMotionData`)로 규격화하여 소울라이크 공방의 손맛과 칼리브라 시뮬레이션의 정확도를 극대화합니다.

---

## 1. 핵심 아키텍처 및 데이터 파이프라인

```
┌─────────────────────────────────┐       ┌──────────────────────────────────────┐
│  DataTable/DT_EnemyMotionData   │ ────> │  FEnemyMotionData (C++ USTRUCT)      │
│  (CSV 기반 모션 밸런스 테이블)      │       │  [Plugins/Enemy/Public/...]          │
└────────────────┬────────────────┘       └──────────────────┬───────────────────┘
                 │                                           │
                 ▼                                           ▼
┌─────────────────────────────────┐       ┌──────────────────────────────────────┐
│  칼리브라 시뮬레이터                │       │  Unreal Engine AI / BehaviorTree     │
│  [Scripts/simulate_balance.py]  │       │  - 선딜 중 패링 판정 윈도우 제공        │
│  - 선후딜 프레임 단위 딜교환 시뮬레이션 │       │  - 후딜레이 딜타임 노출 & 모션 워핑 돌진 │
└─────────────────────────────────┘       └──────────────────────────────────────┘
```

---

## 2. 모션 파라미터 규격 및 공방 설계 기준

| 파라미터명 | 타입 | 권장 수치 범위 | 설계 의도 및 소울라이크 공방 규칙 |
|---|:---:|:---:|---|
| **`MonsterID`** | `FName` | - | 몬스터 식별자 (`Enemy_BaseMelee`, `Boss_BlackKnight` 등) |
| **`AttackName`** | `FText` | - | 모션 이름 (디버깅/기획 표시용) |
| **`AttackType`** | `Enum` | 6개 유형 | 근접 기본, 근접 강공, 원거리, 돌진, 광역, 반격기 |
| **`AttackMontage`** | `UAnimMontage*` | - | 언리얼 엔진 공격 몽타주 에셋 경로 |
| **`DamageMultiplier`**| `float` | `0.8 ~ 2.2x` | 기본 공격력 대비 배율 (약공 1.0x, 돌진 1.5x, 필살기 2.0x 이상) |
| **`AttackRange`** | `float` | `150 ~ 1200cm` | AI가 해당 공격을 선택/시작할 수 있는 최대 유효 거리 |
| **`Cooldown`** | `float` | `0.0 ~ 14.0s` | 해당 패턴의 재사용 쿨타임 (패턴 반복 방지) |
| **`WindupTime` (선딜)** | `float` | `0.35 ~ 1.5s` | **[플레이어 대응 윈도우]** 모션 시작 ~ 피격 판정까지 시간 (패링/가드/회피 가능 구간) |
| **`ActiveHitDuration`** | `float` | `0.2 ~ 0.8s` | 실제 히트박스(콜리전) 유효 지속 시간 |
| **`RecoveryTime` (후딜)**| `float` | `0.6 ~ 2.5s` | **[플레이어 딜타임]** 공격 종료 후 몬스터 경직 시간 (안전하게 창 콤보를 넣는 구간) |
| **`bIsUnblockable`** | `bool` | `True / False` | **가드 불가 여부**: 일반 가드 시 80% 감쇄 불가 ➡️ 저스트 패링 또는 순수 회피 필수 |
| **`bHasSuperArmor`** | `bool` | `True / False` | **슈퍼아머**: 공격 도중 플레이어 피격을 맞아도 공격이 끊기지 않음 |
| **`KnockbackPower`** | `float` | `0 ~ 500` | 적중 시 플레이어를 뒤로 밀쳐내는 물리 충격량 |
| **`bUseMotionWarp`** | `bool` | `True / False` | 모션 워핑 컴포넌트를 사용해 플레이어에게 유도 돌진 |
| **`WarpDistance`** | `float` | `0 ~ 500cm` | 모션 워핑으로 전진 파고드는 최대 거리 |
| **`RequiredPhase`** | `int32` | `0 ~ 2` | 0=상시, 1=1페이즈 전용, 2=보스 체력 50% 이하 2페이즈 광폭화 전용 |

---

## 3. C++ 비헤이비어 트리 연동 몬스터 공격 패턴 밸런스 테이블 (전체 49종)

### 🗡️ [일반 몬스터 10종]
* **`Enemy_BaseMelee` (근접 미니언)**
  * `Melee_NormalSlash`: 기본 베기 (1.0x, 선딜 0.4s, 후딜 0.8s) ➡️ 가벼운 평타
  * `Melee_LungeStab`: 전진 찌르기 (1.3x, 선딜 0.65s, 후딜 1.1s, 워핑 150) ➡️ 패링 찬스 패턴
* **`Enemy_BaseGuard` (방패병 정예)**
  * `Guard_ShieldBash`: 방패 밀치기 (0.8x, 선딜 0.35s, 후딜 0.6s, 넉백 300, 슈퍼아머) ➡️ 근접 밀어내기
  * `Guard_NormalAttack`: 일반 타격 (1.0x, 선딜 0.5s, 후딜 0.9s)
  * `Guard_ReactionCounter`: 가드 리액션 카운터 (2.0x, 선딜 0.4s, 후딜 1.0s, 슈퍼아머) ➡️ 가드 중 타격 시 반격
* **`Enemy_BaseRanged` (원거리 궁수/마법사)**
  * `Ranged_MagicBolt`: 마법 화살 투사 (1.0x, 사거리 900, 선딜 0.9s, 후딜 1.3s)
* **`Enemy_BasePassive` (비선공 반격 몬스터)**
  * `Passive_CounterMelee`: 피격 후 반격타 (1.0x, 사거리 200, 선딜 0.45s, 후딜 0.8s)
* **`Enemy_Forest_Bee` (숲 벌)**
  * `Bee_StingDart`: 급강하 독침 (1.2x, 사거리 300, 선딜 0.5s, 후딜 0.7s, 워핑 180)
* **`Enemy_BaseExploder` (자폭 몬스터)**
  * `Exploder_SuicideBomb`: 광역 자폭 폭발 (2.5x, 사거리 200, 선딜 1.2s, **가드 불가**)
* **`Enemy_BaseBurrow` (잠복 몬스터)**
  * `Burrow_UnburrowSurprise`: 언버로우 기습 솟구치기 (1.5x, 사거리 250, 선딜 0.6s, 후딜 1.0s, 넉백 300)
  * `Burrow_FireBreath`: 화염 방사 (1.2x, 사거리 450, 선딜 0.8s, 후딜 1.5s)
* **`Enemy_BaseMage` (일반 마법사)**
  * `Mage_CastSphere`: 에너지 구체 발사 (1.0x, 사거리 850, 선딜 0.95s, 후딜 1.4s)
* **`Enemy_BaseMimic` (미믹/보물상자 위장)**
  * `Mimic_SurpriseBite`: 상호작용 기습 물어뜯기 (2.0x, 사거리 180, 선딜 0.3s, **가드 불가**)
* **`Enemy_BaseSlime` (슬라임)**
  * `Slime_BodySlam`: 도약 바디슬램 (1.1x, 사거리 280, 선딜 0.55s, 후딜 0.9s, 워핑 150)
* **`Enemy_BaseTranspar` (은신 몬스터)**
  * `Transpar_AmbushSlash`: 은신 기습 베기 (1.4x, 사거리 220, 선딜 0.35s, 후딜 0.8s)

### 👑 [보스 몬스터 5종]
* **`Boss_BlackKnight` (동굴 보스 - 블랙 나이트)**
  * `BK_NormalAttack`: 대검 횡베기 (1.0x, 사거리 300, 선딜 0.55s, 후딜 1.0s)
  * `BK_ChargeAttack`: 차지 꿰뚫기 (1.4x, 사거리 450, 선딜 0.75s, 후딜 1.3s, 워핑 200)
  * `BK_RushAttack`: 고속 돌진 러쉬 (1.6x, 사거리 750, 선딜 0.85s, 후딜 1.6s, **가드 불가**, 워핑 500, 슈퍼아머)
  * `BK_GuardDefense`: 방패 방어 자세 (0.0x, 사거리 200, 쿨 4.0s, 슈퍼아머)
  * `BK_GuardCounter`: 방패 튕겨내기 반격 (1.2x, 사거리 250, 선딜 0.3s, 후딜 0.9s, 넉백 500)
  * `BK_LightningZap`: [2페이즈] 낙뢰 강타 (1.8x, 사거리 400, 선딜 1.1s, 후딜 2.0s, **가드 불가**, 슈퍼아머)
* **`Boss_MagicSwordMan` (도시 보스 - 마법검사)**
  * `MSM_CloseAttack`: 근접 쾌검 3연격 (1.1x, 사거리 260, 선딜 0.4s, 후딜 0.8s)
  * `MSM_DashAttack`: 순간 대시 찌르기 (1.3x, 사거리 450, 선딜 0.55s, 후딜 1.0s, 워핑 300)
  * `MSM_WaveAttack`: 원거리 검기 파동 (1.0x, 사거리 800, 선딜 0.65s, 후딜 1.1s)
  * `MSM_CloseJumpUp` ➡️ `MSM_AirAttack`: 도약 띄우기 및 공중 급강하 (1.4x ~ 1.5x)
  * `MSM_JumpAttack`: 대형 점프 내려찍기 (1.6x, 사거리 500, 선딜 0.8s, 후딜 1.4s, **가드 불가**, 워핑 300)
  * `MSM_DashBack`: 백대시 거리 벌리기 (0.0x, 선딜 0.1s, 후딜 0.5s)
  * `MSM_GuardCounter`: 가드 반격 카운터 (1.5x, 사거리 250, 선딜 0.25s, 후딜 0.9s)
  * `MSM_PowerAttack`: [2페이즈] 궁극 폭풍 검무 (2.2x, 사거리 600, 선딜 1.2s, 후딜 2.2s, **가드 불가**, 슈퍼아머)
* **`Boss_Dungeon_SkeletonMage` (던전 보스 - 스켈레톤 메이지)**
  * `SM_FireBall`: 유도 화염구 발사 (1.0x, 사거리 1000, 선딜 0.75s, 후딜 1.1s)
  * `SM_GroundArea`: 대지 폭발 장판 (1.75x, 사거리 500, 선딜 1.0s, 후딜 1.5s, **가드 불가**)
  * `SM_GravityAttack`: 중력장 흡인 구속 (0.8x, 사거리 600, 선딜 0.9s, 후딜 1.5s)
  * `SM_PushTarget`: 충격파 밀쳐내기 (0.6x, 사거리 300, 선딜 0.35s, 후딜 0.8s, 넉백 600)
  * `SM_Teleport`: 순간이동 거리벌리기 (0.0x, 사거리 800, 선딜 0.5s, 후딜 1.5s)
  * `SM_SummonEnemy`: 해골 잡몹 소환 (0.0x, 사거리 400, 선딜 1.5s, 후딜 2.0s)
  * `SM_MeteorAttack`: [2페이즈] 종말의 유성 폭격 (2.0x, 사거리 1200, 선딜 1.4s, 후딜 2.5s, **가드 불가**, 슈퍼아머)
* **`Boss_BossWorm` (숲/사막 보스 - 보스 웜)**
  * `Worm_NormalBite`: 물어뜯기 (1.0x, 사거리 300, 선딜 0.6s, 후딜 1.0s)
  * `Worm_LungeAttack`: 몸통 박치기 (1.4x, 사거리 550, 선딜 0.85s, 후딜 1.4s, 워핑 350, 슈퍼아머)
  * `Worm_RangedSpit`: 원거리 가시 투사 (1.0x, 사거리 850, 선딜 0.8s, 후딜 1.2s)
  * `Worm_LinearBreath` / `Worm_FanBreath`: 직선/부채꼴 화염 브레스 (1.3x ~ 1.5x, **가드 불가**)
  * `Worm_Burrow` ➡️ `Worm_UnBurrow`: 땅속 잠복 후 발밑 솟구치기 (1.6x, 사거리 400, **가드 불가**, 에어본)
  * `Worm_Suction`: 모래폭풍 강제 흡입 (1.8x, 사거리 800, 선딜 1.3s, **가드 불가**, 슈퍼아머)
* **`Boss_Hechi` (신수 보스 - 해치)**
  * `Hechi_Laser`: 전방 파괴 레이저 (1.5x, 사거리 1000, 선딜 1.0s, **가드 불가**)
  * `Hechi_GravityAttack`: 중력장 붕괴 (1.4x, 사거리 600, 선딜 0.9s, 후딜 1.5s)
  * `Hechi_ThrowMagicBall`: 거대 마법구 투척 (1.3x, 사거리 850, 선딜 0.8s, 후딜 1.3s)
  * `Hechi_Teleport`: 신수 점멸 (0.0x, 사거리 800, 선딜 0.4s, 후딜 1.2s)


---

## 4. 플레이어 창 콤보와의 상성 및 공방 리듬

* **패링 타이밍 공식**:
  $$\text{Parry Window} = \text{WindupTime} \pm 0.15\text{s}$$
  * 선딜(`WindupTime`)이 0.6초 이상인 공격은 일반 유저도 보고 반응할 수 있는 **"패링 찬스 패턴"**입니다.
* **딜타임 공식**:
  $$\text{Player Safe Attack Count} = \left\lfloor \frac{\text{RecoveryTime}}{\text{Spear Attack Interval (0.45s)}} \right\rfloor$$
  * 후딜(`RecoveryTime`)이 1.2초인 패턴을 피한 후에는 **플레이어가 안전하게 창 2~3타 콤보**를 넣을 수 있습니다.
