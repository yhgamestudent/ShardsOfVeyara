#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TutorialTypes.generated.h"

/**
 * 튜토리얼에서 플레이어가 수행해야 하는 행동(Action-Gating)의 유형입니다.
 */
UENUM(BlueprintType)
enum class ETutorialActionType : uint8
{
	None UMETA(DisplayName = "None"),

	// 마을 시퀀스 (Tutorial_Village)
	Move UMETA(DisplayName = "Move (WASD)"),
	Roll UMETA(DisplayName = "Roll (Space)"),
	ReachArea UMETA(DisplayName = "Reach Area"),
	Interact UMETA(DisplayName = "Interact (E)"),
	WeedHarvest UMETA(DisplayName = "Harvest Weed (E)"),
	PlantSeed UMETA(DisplayName = "Plant Seed"),
	EnterPortal UMETA(DisplayName = "Enter Portal"),

	// 하늘섬 시퀀스 (Tutorial_Sky_Island)
	EquipWeapon UMETA(DisplayName = "Equip Weapon"),
	AttackCombo UMETA(DisplayName = "Attack 3-Combo"),
	LockOn UMETA(DisplayName = "Lock-On"),
	Guard UMETA(DisplayName = "Guard / Block"),
	RollCancel UMETA(DisplayName = "Roll Cancel"),
	JumpPlatform UMETA(DisplayName = "Jump Platform"),
	DungeonGate UMETA(DisplayName = "Dungeon Gate"),

	// 거점 시퀀스 (Farm_Sky_Island)
	TributeAltar UMETA(DisplayName = "Tribute Altar"),
	AlchemyTable UMETA(DisplayName = "Alchemy Table"),
	OrbAltar UMETA(DisplayName = "Orb Altar"),
	PortalExit UMETA(DisplayName = "Portal Exit"),

	// 범용 커스텀 액션
	Custom UMETA(DisplayName = "Custom Action")
};

/**
 * 튜토리얼 스텝 내에서 달성해야 하는 개별 세부 목표입니다.
 * 한 스텝 내에 여러 목표를 배치하여 병렬(동시) 진행을 지원합니다.
 */
USTRUCT(BlueprintType)
struct FTutorialObjective
{
	GENERATED_BODY()

public:
	/** 세부 목표 설명 텍스트 (예: "작물 심기", "작물 수확". 진행도 UI에 따로 표시될 때 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FText ObjectiveDescription;

	/** 검증할 플레이어 행동 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	ETutorialActionType ActionType = ETutorialActionType::None;

	/** 필요 행동 횟수 (기본 1회) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	int32 RequiredActionCount = 1;

	/** 커스텀 액션 식별용 태그 (ActionType == Custom일 때 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName CustomActionTag = NAME_None;

	/** 이 세부 목표 지점을 안내할 웨이포인트 태그 (선택 사항, 없으면 스텝의 TargetWaypointTag 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName TargetWaypointTag = NAME_None;

	/** 맵 전환(EnterPortal/레벨 이동) 시 목표가 되는 레벨/맵 이름 (선택 사항, 비워두면 임의의 맵 이동 시 완료) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName TargetLevelName = NAME_None;
};

/**
 * 튜토리얼의 각 단계(스텝) 정보를 담는 데이터 테이블 행 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FTutorialStepData : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** 스텝 고유 식별자 (예: VIL_01, SKY_04, HUB_01) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName StepID;

	/** UI 상단에 표시될 퀘스트 가이드 목표 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FText QuestGuideText;

	/** 화면 중앙/하단에 노출될 세부 조작 키 힌트 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FText ControlHintText;

	/** 이 스텝을 통과하기 위해 검증해야 하는 플레이어 행동 (단일 목표 시 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	ETutorialActionType ActionType = ETutorialActionType::None;

	/** 목표 달성을 위해 필요한 행동 횟수 (기본 1회, 잡초 2개면 2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	int32 RequiredActionCount = 1;

	/** 지점 도달(ReachArea) 판정 수평 반경 (cm 단위, 0 이하면 서브시스템 기본값 150cm 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	float ReachAreaRadius = 0.0f;

	/** 펫이 날아가서 대기하고 마커가 가리킬 월드 내 액터의 Tag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName TargetWaypointTag;

	/** 맵 전환(EnterPortal/레벨 이동) 시 목표가 되는 레벨/맵 이름 (선택 사항, 비워두면 임의의 맵 이동 시 완료) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName TargetLevelName = NAME_None;

	/** 스텝 시작 시 펫 또는 캐릭터가 출력할 대화 ID (None이면 대화 생략) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName DialogueID;

	/** 마을 맵 연출처럼 펫 메쉬를 숨기고 머리 위 마커만 띄울지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	bool bHidePetMesh = false;

	/** 펫이 목표 지점(웨이포인트 액터)으로 직접 날아갈지 여부 (false면 플레이어 곁을 유지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	bool bSendPetToWaypoint = true;

	/** 목표 지점으로 펫을 순간이동시킬지 여부 (점프맵 등 장거리 이동 시) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	bool bTeleportPet = false;

	/** 해당 스텝 통과 전까지 길을 막고 있다가 완료 시 열릴 문/장벽 액터의 Tag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName GateActorTag;

	/** 스텝 완료 시 보상으로 지급할 아이템 ID (선택 사항) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName RewardItemID;

	/** 보상 아이템 지급 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	int32 RewardItemCount = 0;

	/** 커스텀 액션 식별용 태그 (ActionType == Custom일 때 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName CustomActionTag;

	/**
	 * 병렬로 동시에 진행할 세부 목표 목록입니다.
	 * 비어 있으면 기존의 단일 ActionType / RequiredActionCount 방식을 사용합니다.
	 * 채워져 있으면 이 목록의 모든 목표가 완료되어야 해당 스텝이 클리어됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	TArray<FTutorialObjective> SubObjectives;
};
