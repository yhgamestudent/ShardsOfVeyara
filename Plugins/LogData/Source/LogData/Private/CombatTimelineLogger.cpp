#include "CombatTimelineLogger.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

// 정적 멤버 변수 정의
bool UCombatTimelineLogger::bSessionActive = false;
FString UCombatTimelineLogger::CurrentSessionFilePath = TEXT("");
FString UCombatTimelineLogger::CurrentBossID = TEXT("");
float UCombatTimelineLogger::SessionStartWorldTime = 0.0f;
int32 UCombatTimelineLogger::ActionCounter = 0;
TMap<FString, FActiveSkillTrackingInfo> UCombatTimelineLogger::ActiveSkills;
TMap<FString, FSkillAggregateStats> UCombatTimelineLogger::SkillStats;

static FString GetFormattedCurrentTime()
{
	const FDateTime Now = FDateTime::Now();
	return FString::Printf(TEXT("%04d-%02d-%02d %02d:%02d:%02d.%03d"),
		Now.GetYear(), Now.GetMonth(), Now.GetDay(),
		Now.GetHour(), Now.GetMinute(), Now.GetSecond(),
		Now.GetMillisecond());
}

void UCombatTimelineLogger::StartCombatSession(
	const FString& BossID,
	float StartWorldTime,
	float BossMaxHP,
	float PlayerMaxHP,
	float InitialDistance)
{
	bSessionActive = true;
	ActionCounter = 0;
	ActiveSkills.Empty();
	SkillStats.Empty();

	SessionStartWorldTime = StartWorldTime;
	CurrentBossID = BossID.IsEmpty() ? TEXT("SkeletonMage") : BossID;

	const FDateTime Now = FDateTime::Now();
	const FString TimeStr = FString::Printf(TEXT("%04d%02d%02d_%02d%02d%02d"),
		Now.GetYear(), Now.GetMonth(), Now.GetDay(),
		Now.GetHour(), Now.GetMinute(), Now.GetSecond());

	const FString Directory = FPaths::ProjectSavedDir() / TEXT("LogData") / TEXT("CombatTimeline");
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*Directory))
	{
		PlatformFile.CreateDirectoryTree(*Directory);
	}

	CurrentSessionFilePath = Directory / FString::Printf(TEXT("Timeline_%s_%s.csv"), *CurrentBossID, *TimeStr);

	// 파일 초기화 및 헤더 작성
	const FString Header = FCombatTimelineEvent::GetCSVHeader() + TEXT("\n");
	FFileHelper::SaveStringToFile(
		Header,
		*CurrentSessionFilePath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(),
		FILEWRITE_None // 새 파일 생성
	);

	// BATTLE_START 이벤트 기록
	FCombatTimelineEvent StartEvent;
	StartEvent.Timestamp = GetFormattedCurrentTime();
	StartEvent.CombatTime = 0.0f;
	StartEvent.ActionID = TEXT("-");
	StartEvent.EventType = ECombatEventType::BattleStart;
	StartEvent.SourceActor = TEXT("Player");
	StartEvent.TargetActor = CurrentBossID;
	StartEvent.ActionName = TEXT("Engage");
	StartEvent.Damage = 0.0f;
	StartEvent.Distance = InitialDistance;
	StartEvent.PlayerState = TEXT("Idle");
	StartEvent.BossHP = BossMaxHP;
	StartEvent.BossHPPct = 100.0f;
	StartEvent.PlayerHP = PlayerMaxHP;
	StartEvent.PlayerHPPct = 100.0f;
	StartEvent.Details = TEXT("Combat Initiated");

	WriteEvent(StartEvent);
}

FString UCombatTimelineLogger::LogSkillWindup(
	const FString& ActionName,
	float CurrentWorldTime,
	float Distance,
	const FString& PlayerState,
	float BossHP,
	float BossMaxHP,
	float PlayerHP,
	float PlayerMaxHP)
{
	if (!bSessionActive) return TEXT("");

	ActionCounter++;
	const FString ActionID = FString::Printf(TEXT("%s_#%d"), *ActionName, ActionCounter);

	FActiveSkillTrackingInfo Info;
	Info.ActionID = ActionID;
	Info.ActionName = ActionName;
	Info.WindupStartTime = CurrentWorldTime;
	Info.InitialPlayerDistance = Distance;
	ActiveSkills.Add(ActionID, Info);

	FSkillAggregateStats& Stats = SkillStats.FindOrAdd(ActionName);
	Stats.SkillName = ActionName;
	Stats.CastCount++;

	FCombatTimelineEvent Event;
	Event.Timestamp = GetFormattedCurrentTime();
	Event.CombatTime = FMath::Max(0.0f, CurrentWorldTime - SessionStartWorldTime);
	Event.ActionID = ActionID;
	Event.EventType = ECombatEventType::SkillWindup;
	Event.SourceActor = CurrentBossID;
	Event.TargetActor = TEXT("Player");
	Event.ActionName = ActionName;
	Event.Damage = 0.0f;
	Event.Distance = Distance;
	Event.PlayerState = PlayerState;
	Event.BossHP = BossHP;
	Event.BossHPPct = (BossMaxHP > 0.f) ? (BossHP / BossMaxHP * 100.f) : 0.f;
	Event.PlayerHP = PlayerHP;
	Event.PlayerHPPct = (PlayerMaxHP > 0.f) ? (PlayerHP / PlayerMaxHP * 100.f) : 0.f;
	Event.Details = TEXT("Windup Started");

	WriteEvent(Event);
	return ActionID;
}

void UCombatTimelineLogger::LogSkillRelease(
	const FString& ActionID,
	float CurrentWorldTime,
	float Distance,
	const FString& PlayerState,
	float BossHP,
	float BossMaxHP,
	float PlayerHP,
	float PlayerMaxHP,
	const FString& AdditionalDetails)
{
	if (!bSessionActive) return;

	float WindupDuration = 0.0f;
	FString ActionName = ActionID;

	if (FActiveSkillTrackingInfo* Info = ActiveSkills.Find(ActionID))
	{
		WindupDuration = FMath::Max(0.0f, CurrentWorldTime - Info->WindupStartTime);
		Info->ReleaseTime = CurrentWorldTime;
		ActionName = Info->ActionName;

		if (FSkillAggregateStats* Stats = SkillStats.Find(ActionName))
		{
			Stats->TotalWindupTime += WindupDuration;
			Stats->WindupSamples++;
		}
	}

	FCombatTimelineEvent Event;
	Event.Timestamp = GetFormattedCurrentTime();
	Event.CombatTime = FMath::Max(0.0f, CurrentWorldTime - SessionStartWorldTime);
	Event.ActionID = ActionID;
	Event.EventType = ECombatEventType::SkillRelease;
	Event.SourceActor = CurrentBossID;
	Event.TargetActor = TEXT("Player");
	Event.ActionName = ActionName;
	Event.Damage = 0.0f;
	Event.Distance = Distance;
	Event.PlayerState = PlayerState;
	Event.BossHP = BossHP;
	Event.BossHPPct = (BossMaxHP > 0.f) ? (BossHP / BossMaxHP * 100.f) : 0.f;
	Event.PlayerHP = PlayerHP;
	Event.PlayerHPPct = (PlayerMaxHP > 0.f) ? (PlayerHP / PlayerMaxHP * 100.f) : 0.f;

	FString DetailStr = FString::Printf(TEXT("Windup: %.2fs"), WindupDuration);
	if (!AdditionalDetails.IsEmpty())
	{
		DetailStr += FString::Printf(TEXT(" | %s"), *AdditionalDetails);
	}
	Event.Details = DetailStr;

	WriteEvent(Event);
}

void UCombatTimelineLogger::LogSkillRecovery(
	const FString& ActionID,
	float CurrentWorldTime,
	float BossHP,
	float BossMaxHP,
	float PlayerHP,
	float PlayerMaxHP)
{
	if (!bSessionActive) return;

	float PostDelay = 0.0f;
	float TotalCast = 0.0f;
	FString ActionName = ActionID;

	if (FActiveSkillTrackingInfo* Info = ActiveSkills.Find(ActionID))
	{
		ActionName = Info->ActionName;
		if (Info->ReleaseTime > 0.f)
		{
			PostDelay = FMath::Max(0.0f, CurrentWorldTime - Info->ReleaseTime);
		}
		TotalCast = FMath::Max(0.0f, CurrentWorldTime - Info->WindupStartTime);

		if (FSkillAggregateStats* Stats = SkillStats.Find(ActionName))
		{
			Stats->TotalRecoveryTime += PostDelay;
			Stats->RecoverySamples++;
		}
	}

	FCombatTimelineEvent Event;
	Event.Timestamp = GetFormattedCurrentTime();
	Event.CombatTime = FMath::Max(0.0f, CurrentWorldTime - SessionStartWorldTime);
	Event.ActionID = ActionID;
	Event.EventType = ECombatEventType::SkillRecovery;
	Event.SourceActor = CurrentBossID;
	Event.TargetActor = TEXT("None");
	Event.ActionName = ActionName;
	Event.Damage = 0.0f;
	Event.Distance = 0.0f;
	Event.PlayerState = TEXT("-");
	Event.BossHP = BossHP;
	Event.BossHPPct = (BossMaxHP > 0.f) ? (BossHP / BossMaxHP * 100.f) : 0.f;
	Event.PlayerHP = PlayerHP;
	Event.PlayerHPPct = (PlayerMaxHP > 0.f) ? (PlayerHP / PlayerMaxHP * 100.f) : 0.f;
	Event.Details = FString::Printf(TEXT("PostDelay: %.2fs | Total: %.2fs"), PostDelay, TotalCast);

	WriteEvent(Event);
}

void UCombatTimelineLogger::LogHitSuccess(
	const FString& ActionID,
	const FString& ActionName,
	float Damage,
	float CurrentWorldTime,
	float Distance,
	const FString& PlayerState,
	float BossHP,
	float BossMaxHP,
	float PlayerHP,
	float PlayerMaxHP)
{
	if (!bSessionActive) return;

	float TravelTime = 0.0f;
	if (FActiveSkillTrackingInfo* Info = ActiveSkills.Find(ActionID))
	{
		if (Info->ReleaseTime > 0.f)
		{
			TravelTime = FMath::Max(0.0f, CurrentWorldTime - Info->ReleaseTime);
		}
	}

	FSkillAggregateStats& Stats = SkillStats.FindOrAdd(ActionName);
	Stats.SkillName = ActionName;
	Stats.HitCount++;
	Stats.TotalDamageDealt += Damage;
	if (TravelTime > 0.f)
	{
		Stats.TotalTravelTime += TravelTime;
		Stats.TravelSamples++;
	}
	int32& StateCount = Stats.VictimStateCounts.FindOrAdd(PlayerState);
	StateCount++;

	FCombatTimelineEvent Event;
	Event.Timestamp = GetFormattedCurrentTime();
	Event.CombatTime = FMath::Max(0.0f, CurrentWorldTime - SessionStartWorldTime);
	Event.ActionID = ActionID.IsEmpty() ? ActionName : ActionID;
	Event.EventType = ECombatEventType::HitSuccess;
	Event.SourceActor = ActionName.StartsWith(TEXT("Minion_")) ? TEXT("Minion") : CurrentBossID;
	Event.TargetActor = TEXT("Player");
	Event.ActionName = ActionName;
	Event.Damage = Damage;
	Event.Distance = Distance;
	Event.PlayerState = PlayerState;
	Event.BossHP = BossHP;
	Event.BossHPPct = (BossMaxHP > 0.f) ? (BossHP / BossMaxHP * 100.f) : 0.f;
	Event.PlayerHP = PlayerHP;
	Event.PlayerHPPct = (PlayerMaxHP > 0.f) ? (PlayerHP / PlayerMaxHP * 100.f) : 0.f;
	Event.Details = FString::Printf(TEXT("Hit Player (Travel: %.2fs)"), TravelTime);

	WriteEvent(Event);
}

void UCombatTimelineLogger::LogHitMissed(
	const FString& ActionID,
	const FString& ActionName,
	float CurrentWorldTime,
	float Distance,
	const FString& Reason)
{
	if (!bSessionActive) return;

	FSkillAggregateStats& Stats = SkillStats.FindOrAdd(ActionName);
	Stats.SkillName = ActionName;
	Stats.MissCount++;

	FCombatTimelineEvent Event;
	Event.Timestamp = GetFormattedCurrentTime();
	Event.CombatTime = FMath::Max(0.0f, CurrentWorldTime - SessionStartWorldTime);
	Event.ActionID = ActionID.IsEmpty() ? ActionName : ActionID;
	Event.EventType = ECombatEventType::HitMissed;
	Event.SourceActor = ActionName.StartsWith(TEXT("Minion_")) ? TEXT("Minion") : CurrentBossID;
	Event.TargetActor = TEXT("Player");
	Event.ActionName = ActionName;
	Event.Damage = 0.0f;
	Event.Distance = Distance;
	Event.PlayerState = TEXT("-");
	Event.BossHP = 0.0f;
	Event.BossHPPct = 0.0f;
	Event.PlayerHP = 0.0f;
	Event.PlayerHPPct = 0.0f;
	Event.Details = Reason;

	WriteEvent(Event);
}

void UCombatTimelineLogger::LogHitGuarded(
	const FString& ActionID,
	const FString& ActionName,
	float MitigatedDamage,
	float CurrentWorldTime,
	float Distance,
	float BossHP,
	float BossMaxHP,
	float PlayerHP,
	float PlayerMaxHP)
{
	if (!bSessionActive) return;

	FSkillAggregateStats& Stats = SkillStats.FindOrAdd(ActionName);
	Stats.SkillName = ActionName;
	Stats.GuardCount++;

	FCombatTimelineEvent Event;
	Event.Timestamp = GetFormattedCurrentTime();
	Event.CombatTime = FMath::Max(0.0f, CurrentWorldTime - SessionStartWorldTime);
	Event.ActionID = ActionID.IsEmpty() ? ActionName : ActionID;
	Event.EventType = ECombatEventType::HitGuarded;
	Event.SourceActor = ActionName.StartsWith(TEXT("Minion_")) ? TEXT("Minion") : CurrentBossID;
	Event.TargetActor = TEXT("Player");
	Event.ActionName = ActionName;
	Event.Damage = MitigatedDamage;
	Event.Distance = Distance;
	Event.PlayerState = TEXT("Guarding");
	Event.BossHP = BossHP;
	Event.BossHPPct = (BossMaxHP > 0.f) ? (BossHP / BossMaxHP * 100.f) : 0.f;
	Event.PlayerHP = PlayerHP;
	Event.PlayerHPPct = (PlayerMaxHP > 0.f) ? (PlayerHP / PlayerMaxHP * 100.f) : 0.f;
	Event.Details = FString::Printf(TEXT("Normal Guard (Mitigated: %.1f)"), MitigatedDamage);

	WriteEvent(Event);
}

void UCombatTimelineLogger::LogHitJustGuard(
	const FString& ActionID,
	const FString& ActionName,
	float CurrentWorldTime,
	float Distance,
	float BossHP,
	float BossMaxHP,
	float PlayerHP,
	float PlayerMaxHP)
{
	if (!bSessionActive) return;

	FSkillAggregateStats& Stats = SkillStats.FindOrAdd(ActionName);
	Stats.SkillName = ActionName;
	Stats.JustGuardCount++;

	FCombatTimelineEvent Event;
	Event.Timestamp = GetFormattedCurrentTime();
	Event.CombatTime = FMath::Max(0.0f, CurrentWorldTime - SessionStartWorldTime);
	Event.ActionID = ActionID.IsEmpty() ? ActionName : ActionID;
	Event.EventType = ECombatEventType::HitJustGuard;
	Event.SourceActor = ActionName.StartsWith(TEXT("Minion_")) ? TEXT("Minion") : CurrentBossID;
	Event.TargetActor = TEXT("Player");
	Event.ActionName = ActionName;
	Event.Damage = 0.0f;
	Event.Distance = Distance;
	Event.PlayerState = TEXT("JustGuarding");
	Event.BossHP = BossHP;
	Event.BossHPPct = (BossMaxHP > 0.f) ? (BossHP / BossMaxHP * 100.f) : 0.f;
	Event.PlayerHP = PlayerHP;
	Event.PlayerHPPct = (PlayerMaxHP > 0.f) ? (PlayerHP / PlayerMaxHP * 100.f) : 0.f;
	Event.Details = TEXT("Just Guard Perfect Parry!");

	WriteEvent(Event);
}

void UCombatTimelineLogger::LogBossDamaged(
	const FString& AttackName,
	float Damage,
	float CurrentWorldTime,
	float Distance,
	float BossHP,
	float BossMaxHP,
	float PlayerHP,
	float PlayerMaxHP,
	const FString& PlayerState)
{
	if (!bSessionActive) return;

	FCombatTimelineEvent Event;
	Event.Timestamp = GetFormattedCurrentTime();
	Event.CombatTime = FMath::Max(0.0f, CurrentWorldTime - SessionStartWorldTime);
	Event.ActionID = TEXT("-");
	Event.EventType = ECombatEventType::BossDamaged;
	Event.SourceActor = TEXT("Player");
	Event.TargetActor = CurrentBossID;
	Event.ActionName = AttackName;
	Event.Damage = Damage;
	Event.Distance = Distance;
	Event.PlayerState = PlayerState;
	Event.BossHP = BossHP;
	Event.BossHPPct = (BossMaxHP > 0.f) ? (BossHP / BossMaxHP * 100.f) : 0.f;
	Event.PlayerHP = PlayerHP;
	Event.PlayerHPPct = (PlayerMaxHP > 0.f) ? (PlayerHP / PlayerMaxHP * 100.f) : 0.f;
	Event.Details = TEXT("Boss Damaged by Player");

	WriteEvent(Event);
}

void UCombatTimelineLogger::LogPhaseTransition(
	const FString& PhaseName,
	float CurrentWorldTime,
	float BossHP,
	float BossMaxHP,
	float PlayerHP,
	float PlayerMaxHP,
	const FString& Details)
{
	if (!bSessionActive) return;

	FCombatTimelineEvent Event;
	Event.Timestamp = GetFormattedCurrentTime();
	Event.CombatTime = FMath::Max(0.0f, CurrentWorldTime - SessionStartWorldTime);
	Event.ActionID = TEXT("-");
	Event.EventType = ECombatEventType::PhaseTransition;
	Event.SourceActor = CurrentBossID;
	Event.TargetActor = TEXT("None");
	Event.ActionName = PhaseName;
	Event.Damage = 0.0f;
	Event.Distance = 0.0f;
	Event.PlayerState = TEXT("-");
	Event.BossHP = BossHP;
	Event.BossHPPct = (BossMaxHP > 0.f) ? (BossHP / BossMaxHP * 100.f) : 0.f;
	Event.PlayerHP = PlayerHP;
	Event.PlayerHPPct = (PlayerMaxHP > 0.f) ? (PlayerHP / PlayerMaxHP * 100.f) : 0.f;
	Event.Details = Details.IsEmpty() ? TEXT("Phase Transition") : Details;

	WriteEvent(Event);
}

void UCombatTimelineLogger::EndCombatSession(
	const FString& Result,
	float EndWorldTime,
	float BossHP,
	float BossMaxHP,
	float PlayerHP,
	float PlayerMaxHP)
{
	if (!bSessionActive) return;

	const float TotalDuration = FMath::Max(0.0f, EndWorldTime - SessionStartWorldTime);

	// BATTLE_END 이벤트 기록
	FCombatTimelineEvent EndEvent;
	EndEvent.Timestamp = GetFormattedCurrentTime();
	EndEvent.CombatTime = TotalDuration;
	EndEvent.ActionID = TEXT("-");
	EndEvent.EventType = ECombatEventType::BattleEnd;
	EndEvent.SourceActor = CurrentBossID;
	EndEvent.TargetActor = TEXT("Player");
	EndEvent.ActionName = Result;
	EndEvent.Damage = 0.0f;
	EndEvent.Distance = 0.0f;
	EndEvent.PlayerState = TEXT("-");
	EndEvent.BossHP = BossHP;
	EndEvent.BossHPPct = (BossMaxHP > 0.f) ? (BossHP / BossMaxHP * 100.f) : 0.f;
	EndEvent.PlayerHP = PlayerHP;
	EndEvent.PlayerHPPct = (PlayerMaxHP > 0.f) ? (PlayerHP / PlayerMaxHP * 100.f) : 0.f;
	EndEvent.Details = FString::Printf(TEXT("Result: %s | Total Duration: %.2fs"), *Result, TotalDuration);

	WriteEvent(EndEvent);

	// 스킬별 KPI 종합 통계 요약표 생성
	FString Summary = TEXT("\n");
	Summary += TEXT("# =========================================================================================\n");
	Summary += TEXT("# COMBAT BALANCE KPI SUMMARY (스킬별 밸런스 핵심 분석 지표)\n");
	Summary += TEXT("# =========================================================================================\n");
	Summary += TEXT("# SkillName,CastCount,Attempts,Hits,Misses,Guards,JustGuards,HitRate(%),AvgWindup(s),AvgTravel(s),AvgRecovery(s),TotalDmgDealt,PrimaryVictimState\n");

	for (const auto& Pair : SkillStats)
	{
		const FSkillAggregateStats& S = Pair.Value;
		const int32 TotalAttempts = S.HitCount + S.MissCount + S.GuardCount + S.JustGuardCount;
		const float HitRate = (TotalAttempts > 0) ? ((float)S.HitCount / (float)TotalAttempts * 100.0f) : 0.0f;
		const float AvgWindup = (S.WindupSamples > 0) ? (S.TotalWindupTime / (float)S.WindupSamples) : 0.0f;
		const float AvgTravel = (S.TravelSamples > 0) ? (S.TotalTravelTime / (float)S.TravelSamples) : 0.0f;
		const float AvgRecovery = (S.RecoverySamples > 0) ? (S.TotalRecoveryTime / (float)S.RecoverySamples) : 0.0f;

		FString TopState = TEXT("-");
		int32 TopCount = 0;
		for (const auto& StatePair : S.VictimStateCounts)
		{
			if (StatePair.Value > TopCount)
			{
				TopCount = StatePair.Value;
				TopState = StatePair.Key;
			}
		}
		const FString PrimaryStateStr = (TopCount > 0) ? FString::Printf(TEXT("%s (%d/%d)"), *TopState, TopCount, S.HitCount) : TEXT("-");

		Summary += FString::Printf(TEXT("# %s,%d,%d,%d,%d,%d,%d,%.1f%%,%.2fs,%.2fs,%.2fs,%.1f,%s\n"),
			*S.SkillName,
			S.CastCount,
			TotalAttempts,
			S.HitCount,
			S.MissCount,
			S.GuardCount,
			S.JustGuardCount,
			HitRate,
			AvgWindup,
			AvgTravel,
			AvgRecovery,
			S.TotalDamageDealt,
			*PrimaryStateStr);
	}

	Summary += TEXT("# =========================================================================================\n");
	AppendRowToFile(Summary);

	bSessionActive = false;
	CurrentSessionFilePath.Empty();
}

bool UCombatTimelineLogger::IsSessionActive()
{
	return bSessionActive;
}

FString UCombatTimelineLogger::GetLatestActionID(const FString& ActionName)
{
	// 가장 최근에 등록된 해당 스킬의 ActionID 검색
	for (auto It = ActiveSkills.CreateConstIterator(); It; ++It)
	{
		if (It.Value().ActionName == ActionName)
		{
			return It.Key();
		}
	}
	return ActionName;
}

void UCombatTimelineLogger::AppendRowToFile(const FString& RowText)
{
	if (CurrentSessionFilePath.IsEmpty()) return;

	FFileHelper::SaveStringToFile(
		RowText,
		*CurrentSessionFilePath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(),
		FILEWRITE_Append
	);
}

void UCombatTimelineLogger::WriteEvent(const FCombatTimelineEvent& Event)
{
	AppendRowToFile(Event.ToCSVRow() + TEXT("\n"));
}
