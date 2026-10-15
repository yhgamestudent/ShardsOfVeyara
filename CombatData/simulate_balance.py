#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
=============================================================================
[Shards of Veyara] Kalivra-Inspired Monte Carlo Combat Balance Simulator (v2.0)
=============================================================================
- 인게임 실제 데이터 연동:
    * DT_CombatStats.csv (플레이어/몬스터 전역 스탯)
    * DT_BossSkillPatterns.csv (보스 4종 패턴, 쿨타임, 가중치, 가드불가, 딜로스)
    * DT_PotionData.csv / DT_TributeData.csv (농사-연금술-봉헌 생태계)
- 전투 시스템 정밀 모델링:
    * 쇠스랑 콤보 분기 (기본 3단, 전진 4단, 후진 4단) & 완주율
    * 저스트 가드(패링) 성공 시 [Parry_Attack (2.0x -> 3.0x)] 카운터 폭딜 연계
    * 일반 가드 피해 80% 감쇄 (20% 칩 데미지)
    * 포션 복용 애니메이션 락(1.2초 무방비 딜로스)
    * 보스 HP 50% 이하 2페이즈 광폭화 (공격속도 25% 가속 및 궁극 패턴 활성화)
    * 텔레포트/버로우 등 거리 벌리기 딜로스(Downtime)
"""

import csv
import os
import random
import sys
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

if sys.stdout.encoding != 'utf-8':
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass


@dataclass
class CombatStat:
    row_name: str
    character_name: str
    combat_role: str
    max_health: float
    base_attack: float
    defense: float
    crit_chance: float
    crit_multiplier: float
    attack_interval: float
    knockback_res: float
    target_ttk: float


@dataclass
class BossSkillPattern:
    skill_name: str
    weight: float
    cooldown: float
    damage_multiplier: float
    is_unblockable: bool
    downtime_sec: float
    phase: int
    last_used_time: float = -999.0


@dataclass
class CohortProfile:
    name: str
    dodge_rate: float         # 회피 성공 확률 (0.0 ~ 1.0)
    parry_rate: float         # 저스트 가드(패링) 성공 확률 (0.0 ~ 1.0)
    regular_guard_rate: float # 일반 가드 확률 (0.0 ~ 1.0, 데미지 80% 경감)
    dps_efficiency: float     # 공격 딜로스 효율 (1.0 = 완벽한 타격)
    combo_weights: Dict[str, float] = field(default_factory=dict) # 선호 콤보 비중


COHORTS = {
    "Casual": CohortProfile(
        name="초보자 (Casual)",
        dodge_rate=0.25,
        parry_rate=0.05,
        regular_guard_rate=0.20,
        dps_efficiency=0.75,
        combo_weights={"Combo5": 0.65, "Combo2": 0.20, "Combo1": 0.10, "Combo5_Finisher": 0.05},
    ),
    "Standard": CohortProfile(
        name="일반 유저 (Standard)",
        dodge_rate=0.60,
        parry_rate=0.15,
        regular_guard_rate=0.15,
        dps_efficiency=0.90,
        combo_weights={"Combo5": 0.45, "Combo2": 0.25, "Combo1": 0.15, "Combo5_Finisher": 0.15},
    ),
    "Hardcore": CohortProfile(
        name="숙련자 (Hardcore)",
        dodge_rate=0.90,
        parry_rate=0.50,
        regular_guard_rate=0.05,
        dps_efficiency=1.00,
        combo_weights={"Combo5": 0.25, "Combo2": 0.25, "Combo1": 0.20, "Combo5_Finisher": 0.30},
    ),
}

# 콤보 세부 데이터: (소요 시간(초), 데미지 배율)
COMBO_DEFINITIONS = {
    "Combo5": [(0.5, 1.0), (0.6, 1.2), (0.7, 1.5)],                              # 기본 3단 (총 3.7x, 1.8s)
    "Combo2": [(0.5, 1.0), (0.6, 1.2), (0.6, 1.5), (0.7, 2.2)],                 # 전진 4단 (총 5.9x, 2.4s)
    "Combo1": [(0.5, 1.0), (0.6, 1.3), (0.7, 1.6), (0.7, 2.0)],                 # 후진 4단 (총 5.9x, 2.5s)
    "Combo5_Finisher": [(0.5, 1.0), (0.6, 1.2), (0.7, 1.5), (0.8, 2.0)],        # 4타 피니시 (총 5.7x, 2.6s)
    "Parry_Attack": [(0.4, 2.0), (0.6, 3.0)],                                   # 패링 반격 폭딜 (총 5.0x, 1.0s)
}


def load_combat_stats(filepath: str) -> Dict[str, CombatStat]:
    stats = {}
    with open(filepath, mode="r", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        for row in reader:
            row_name = row.get("---") or row.get("RowName")
            if not row_name:
                continue
            stats[row_name] = CombatStat(
                row_name=row_name,
                character_name=row["CharacterName"],
                combat_role=row["CombatRole"],
                max_health=float(row["MaxHealth"]),
                base_attack=float(row["BaseAttackDamage"]),
                defense=float(row["Defense"]),
                crit_chance=float(row["CriticalChance"]),
                crit_multiplier=float(row["CriticalMultiplier"]),
                attack_interval=float(row["AttackInterval"]),
                knockback_res=float(row["KnockbackResistance"]),
                target_ttk=float(row["TargetTTK"]),
            )
    return stats


def load_boss_skill_patterns(filepath: str) -> Dict[str, List[BossSkillPattern]]:
    patterns: Dict[str, List[BossSkillPattern]] = {}
    if not os.path.exists(filepath):
        return patterns

    with open(filepath, mode="r", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        for row in reader:
            boss_id = row.get("BossID")
            if not boss_id:
                continue
            if boss_id not in patterns:
                patterns[boss_id] = []
            patterns[boss_id].append(
                BossSkillPattern(
                    skill_name=row["SkillName"],
                    weight=float(row["Weight"]),
                    cooldown=float(row["Cooldown"]),
                    damage_multiplier=float(row["DamageMultiplier"]),
                    is_unblockable=row["IsUnblockable"].strip().lower() in ("true", "1"),
                    downtime_sec=float(row["DowntimeSec"]),
                    phase=int(row["Phase"]),
                )
            )
    return patterns


def select_boss_skill(skills: List[BossSkillPattern], current_time: float, is_phase2: bool) -> Optional[BossSkillPattern]:
    """쿨다운 및 페이즈 조건을 만족하는 스킬 중 가중치 기반으로 선택"""
    available_skills = []
    weights = []

    for skill in skills:
        # 페이즈 체크: 1페이즈 스킬은 전 구간 사용 가능, 2페이즈 스킬은 Phase 2에서만 사용 가능
        if skill.phase == 2 and not is_phase2:
            continue
        # 쿨다운 체크
        if current_time - skill.last_used_time >= skill.cooldown:
            available_skills.append(skill)
            weights.append(skill.weight)

    if not available_skills:
        return None

    selected = random.choices(available_skills, weights=weights, k=1)[0]
    selected.last_used_time = current_time
    return selected


def simulate_single_battle(
    player_stat: CombatStat,
    enemy_stat: CombatStat,
    cohort: CohortProfile,
    boss_skills: Optional[List[BossSkillPattern]] = None,
    potions_count: int = 0,
    potion_heal_ratio: float = 0.40,  # 기본: 중급 포션 (40% 회복)
    potion_cooldown: float = 8.0,
    potion_drinking_time: float = 1.2, # 포션 복용 시전 무방비 시간
) -> Tuple[bool, float, int, float, float]:
    """
    단일 전투 시뮬레이션
    반환값: (승리여부, 소요시간, 포션소모량, 잔여체력%, 패링카운터딜비중%)
    """
    player_hp = player_stat.max_health
    enemy_hp = enemy_stat.max_health
    enemy_max_hp = enemy_stat.max_health

    potions_left = potions_count
    potions_used = 0
    potion_timer = potion_cooldown  # 전투 시작 시 즉시 사용 가능 상태

    current_time = 0.0
    time_step = 0.1
    max_duration = 360.0

    player_next_action_time = 0.0
    enemy_next_attack_time = random.uniform(0.5, enemy_stat.attack_interval)

    # 쿨다운 상태 복사
    active_skills = [
        BossSkillPattern(
            skill_name=s.skill_name,
            weight=s.weight,
            cooldown=s.cooldown,
            damage_multiplier=s.damage_multiplier,
            is_unblockable=s.is_unblockable,
            downtime_sec=s.downtime_sec,
            phase=s.phase,
            last_used_time=-999.0,
        )
        for s in (boss_skills or [])
    ]

    total_damage_dealt_by_player = 0.0
    parry_counter_damage_dealt = 0.0
    is_drinking_potion = False
    potion_finish_time = 0.0

    while current_time < max_duration and player_hp > 0 and enemy_hp > 0:
        current_time += time_step
        potion_timer += time_step

        is_phase2 = (enemy_hp / enemy_max_hp) <= 0.50
        current_attack_interval = enemy_stat.attack_interval * (0.75 if is_phase2 else 1.0)

        # 1. 포션 복용 완료 처리
        if is_drinking_potion and current_time >= potion_finish_time:
            is_drinking_potion = False
            heal_amount = player_stat.max_health * potion_heal_ratio
            player_hp = min(player_stat.max_health, player_hp + heal_amount)

        # 2. 포션 사용 결정 (체력 40% 이하이고 쿨다운 완료 시)
        if not is_drinking_potion and potions_left > 0 and potion_timer >= potion_cooldown:
            if player_hp <= player_stat.max_health * 0.40:
                is_drinking_potion = True
                potion_finish_time = current_time + potion_drinking_time
                potions_left -= 1
                potions_used += 1
                potion_timer = 0.0
                # 복용 중에는 플레이어 다음 공격도 지연
                player_next_action_time = max(player_next_action_time, potion_finish_time)

        # 3. 플레이어 공격 사이클 (포션 복용 중이 아닐 때)
        if not is_drinking_potion and current_time >= player_next_action_time:
            # 콤보 선택
            combo_type = random.choices(
                list(cohort.combo_weights.keys()),
                weights=list(cohort.combo_weights.values()),
                k=1,
            )[0]
            stages = COMBO_DEFINITIONS[combo_type]

            cycle_damage = 0.0
            cycle_duration = 0.0

            for anim_time, mult in stages:
                is_crit = random.random() < player_stat.crit_chance
                crit_bonus = player_stat.crit_multiplier if is_crit else 1.0
                raw_dmg = (player_stat.base_attack * mult * crit_bonus) - enemy_stat.defense
                dmg = max(1.0, raw_dmg) * cohort.dps_efficiency
                cycle_damage += dmg
                cycle_duration += anim_time

            enemy_hp -= cycle_damage
            total_damage_dealt_by_player += cycle_damage
            player_next_action_time = current_time + cycle_duration

            if enemy_hp <= 0:
                break

        # 4. 몬스터 / 보스 공격 사이클
        if current_time >= enemy_next_attack_time:
            enemy_next_attack_time = current_time + current_attack_interval + random.uniform(-0.2, 0.2)

            # 스킬 선택 (보스 스킬이 있는 경우)
            chosen_skill = select_boss_skill(active_skills, current_time, is_phase2) if active_skills else None

            if chosen_skill:
                skill_mult = chosen_skill.damage_multiplier
                is_unblockable = chosen_skill.is_unblockable
                downtime = chosen_skill.downtime_sec
            else:
                skill_mult = 1.0
                is_unblockable = False
                downtime = 0.0

            # 텔레포트/버로우 등으로 인한 플레이어 접근 딜로스 발생
            if downtime > 0.0:
                player_next_action_time = max(player_next_action_time, current_time + downtime)

            # 순수 공격 판정
            if skill_mult > 0.0:
                # 포션 복용 중에는 회피/가드 불가 -> 100% 확정 피격
                if is_drinking_potion:
                    is_crit = random.random() < enemy_stat.crit_chance
                    crit_bonus = enemy_stat.crit_multiplier if is_crit else 1.0
                    raw_dmg = (enemy_stat.base_attack * skill_mult * crit_bonus) - player_stat.defense
                    player_hp -= max(1.0, raw_dmg)
                else:
                    # 회피 / 가드 / 패링 판정
                    roll = random.random()

                    if is_unblockable:
                        # 가드 불가 스킬: 순수 회피율만 판정
                        if roll < cohort.dodge_rate:
                            pass  # 회피 성공
                        else:
                            # 피격
                            is_crit = random.random() < enemy_stat.crit_chance
                            crit_bonus = enemy_stat.crit_multiplier if is_crit else 1.0
                            raw_dmg = (enemy_stat.base_attack * skill_mult * crit_bonus) - player_stat.defense
                            player_hp -= max(1.0, raw_dmg)
                    else:
                        # 일반 스킬: 패링 -> 일반 가드 -> 회피 -> 피격 순차 판정
                        if roll < cohort.parry_rate:
                            # [패링 성공!]
                            # 1) 데미지 0
                            # 2) 보스 경직 (다음 공격 지연 +1.0초)
                            enemy_next_attack_time += 1.0
                            # 3) 패링 카운터 폭딜 즉시 발동 (Parry_Attack: 2.0x + 3.0x = 5.0x)
                            parry_stages = COMBO_DEFINITIONS["Parry_Attack"]
                            parry_dmg = 0.0
                            for anim_time, mult in parry_stages:
                                is_crit = random.random() < player_stat.crit_chance
                                crit_bonus = player_stat.crit_multiplier if is_crit else 1.0
                                raw_dmg = (player_stat.base_attack * mult * crit_bonus) - enemy_stat.defense
                                parry_dmg += max(1.0, raw_dmg)

                            enemy_hp -= parry_dmg
                            total_damage_dealt_by_player += parry_dmg
                            parry_counter_damage_dealt += parry_dmg
                            player_next_action_time = current_time + 1.0

                            if enemy_hp <= 0:
                                break

                        elif roll < (cohort.parry_rate + cohort.regular_guard_rate):
                            # [일반 가드 성공] 데미지 80% 감쇄 (20% 칩 데미지 피격)
                            is_crit = random.random() < enemy_stat.crit_chance
                            crit_bonus = enemy_stat.crit_multiplier if is_crit else 1.0
                            raw_dmg = ((enemy_stat.base_attack * skill_mult * crit_bonus) - player_stat.defense) * 0.20
                            player_hp -= max(1.0, raw_dmg)

                        elif roll < (cohort.parry_rate + cohort.regular_guard_rate + cohort.dodge_rate):
                            # [회피 성공] 완전 무효화
                            pass
                        else:
                            # [피격] 풀 데미지
                            is_crit = random.random() < enemy_stat.crit_chance
                            crit_bonus = enemy_stat.crit_multiplier if is_crit else 1.0
                            raw_dmg = (enemy_stat.base_attack * skill_mult * crit_bonus) - player_stat.defense
                            player_hp -= max(1.0, raw_dmg)

                if player_hp <= 0:
                    break

    is_win = (player_hp > 0 and enemy_hp <= 0)
    remain_hp_ratio = (max(0.0, player_hp) / player_stat.max_health) * 100.0
    parry_dmg_ratio = (parry_counter_damage_dealt / total_damage_dealt_by_player * 100.0) if total_damage_dealt_by_player > 0 else 0.0

    return is_win, current_time, potions_used, remain_hp_ratio, parry_dmg_ratio


def run_monte_carlo(
    player_stat: CombatStat,
    enemy_stat: CombatStat,
    cohort: CohortProfile,
    boss_skills: Optional[List[BossSkillPattern]] = None,
    potions_count: int = 0,
    potion_heal_ratio: float = 0.40,
    num_simulations: int = 1000,
) -> Dict[str, float]:
    wins = 0
    ttks = []
    potions = []
    end_hp_ratios = []
    parry_contributions = []

    for _ in range(num_simulations):
        win, duration, pot_used, end_hp, parry_contrib = simulate_single_battle(
            player_stat,
            enemy_stat,
            cohort,
            boss_skills=boss_skills,
            potions_count=potions_count,
            potion_heal_ratio=potion_heal_ratio,
        )
        potions.append(pot_used)
        if win:
            wins += 1
            ttks.append(duration)
            end_hp_ratios.append(end_hp)
            parry_contributions.append(parry_contrib)

    return {
        "win_rate": (wins / num_simulations) * 100.0,
        "avg_ttk": (sum(ttks) / len(ttks)) if ttks else 0.0,
        "avg_potion": (sum(potions) / len(potions)) if potions else 0.0,
        "avg_hp_remain": (sum(end_hp_ratios) / len(end_hp_ratios)) if end_hp_ratios else 0.0,
        "avg_parry_contrib": (sum(parry_contributions) / len(parry_contributions)) if parry_contributions else 0.0,
    }


def main():
    base_dir = os.path.dirname(__file__)
    csv_combat_stats = os.path.join(base_dir, "DT_CombatStats.csv")
    csv_boss_skills = os.path.join(base_dir, "DT_BossSkillPatterns.csv")

    if not os.path.exists(csv_combat_stats):
        print(f"[Error] DT_CombatStats.csv not found: {csv_combat_stats}")
        return

    stats = load_combat_stats(csv_combat_stats)
    boss_skills = load_boss_skill_patterns(csv_boss_skills)

    print("=" * 88)
    print("  [Shards of Veyara] Kalivra-Style Monte Carlo Balance Simulation Engine (v2.0)")
    print(f"  시뮬레이션 반복 횟수: 1,000회 per scenario | 보스 패턴 테이블: {len(boss_skills)}종 연동")
    print("=" * 88)

    # 1. 일반 잡몹 밸런스 검증
    mobs = [
        ("Enemy_Forest_Bee", stats.get("Enemy_Forest_Bee")),
        ("Enemy_BaseMelee", stats.get("Enemy_BaseMelee")),
        ("Enemy_BaseRanged", stats.get("Enemy_BaseRanged")),
        ("Enemy_BaseGuard", stats.get("Enemy_BaseGuard")),
    ]
    p_t0 = stats["Player_Tribute0"]

    print("\n▶ [테스트 1] 일반 필드 몬스터 vs 기본 플레이어 (봉헌 0단계, 포션 0개)")
    print("-" * 88)
    for mob_id, mob_stat in mobs:
        if not mob_stat:
            continue
        print(f"  [{mob_stat.character_name}] (HP: {mob_stat.max_health:.0f}, ATK: {mob_stat.base_attack:.0f}, 목표 TTK: {mob_stat.target_ttk}초)")
        for cohort_key, cohort in COHORTS.items():
            res = run_monte_carlo(p_t0, mob_stat, cohort, potions_count=0, num_simulations=500)
            print(
                f"    - {cohort.name:20s} | 승률: {res['win_rate']:5.1f}% | "
                f"TTK: {res['avg_ttk']:4.1f}s | 잔여HP: {res['avg_hp_remain']:4.1f}%"
            )

    # 2. 보스 4종 전역 밸런싱 검증 (봉헌 단계별 순환 검증)
    boss_targets = [
        ("Boss_Dungeon_SkeletonMage", "던전 보스: 스켈레톤 메이지"),
        ("Boss_BossWorm", "숲/사막 보스: 보스 웜"),
        ("Boss_BlackKnight", "동굴 보스: 블랙 나이트"),
        ("Boss_MagicSwordMan", "도시 보스: 마법검사"),
    ]

    tribute_scenarios = [
        ("봉헌 0단계 (무준비)", stats["Player_Tribute0"], 0, 0.20),
        ("봉헌 3단계 (초기: 하급 포션 4개)", stats["Player_Tribute3"], 4, 0.20),
        ("봉헌 7단계 (중기: 중급 포션 4개)", stats["Player_Tribute7"], 4, 0.40),
        ("봉헌 14단계 (상위: 상급 포션 4개)", stats["Player_Tribute14"], 4, 0.70),
        ("봉헌 20단계 (마스터: 상급 포션 4개)", stats["Player_Tribute20"], 4, 0.70),
    ]

    for boss_id, boss_label in boss_targets:
        boss_stat = stats.get(boss_id)
        if not boss_stat:
            continue
        skills = boss_skills.get(boss_id, [])

        print("\n" + "=" * 88)
        print(f"  [테스트 2] {boss_label} 밸런싱 검증 (HP: {boss_stat.max_health:.0f}, ATK: {boss_stat.base_attack:.0f}, 목표 TTK: {boss_stat.target_ttk}초)")
        print(f"  등록된 패턴 수: {len(skills)}개 (2페이즈 광폭화 포함)")
        print("=" * 88)

        for sc_name, p_stat, pot_cnt, pot_ratio in tribute_scenarios:
            print(f"\n▶ 시나리오: {sc_name} [ATK: {p_stat.base_attack:.0f}, HP: {p_stat.max_health:.0f}]")
            for cohort_key, cohort in COHORTS.items():
                res = run_monte_carlo(
                    p_stat,
                    boss_stat,
                    cohort,
                    boss_skills=skills,
                    potions_count=pot_cnt,
                    potion_heal_ratio=pot_ratio,
                    num_simulations=500,
                )
                print(
                    f"   - {cohort.name:20s} | 승률: {res['win_rate']:5.1f}% | "
                    f"TTK: {res['avg_ttk']:5.1f}s | 포션 소모: {res['avg_potion']:3.1f}병 | "
                    f"잔여 HP: {res['avg_hp_remain']:4.1f}% | 패링 폭딜 기여: {res['avg_parry_contrib']:4.1f}%"
                )

    print("\n" + "=" * 88)
    print("  [시뮬레이터 템플릿 사용 안내]")
    print("  * 보스 스킬 수치 변경: DT_BossSkillPatterns.csv 에서 데미지배율, 쿨타임, 가중치 수정")
    print("  * 몬스터/플레이어 스탯 변경: DT_CombatStats.csv 에서 수치 수정 후 재실행")
    print("=" * 88)


if __name__ == "__main__":
    main()
