#include "BaseFlyingPet.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Component/PetTalkComponent.h"
#include "Component/PetGuideComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "DrawDebugHelpers.h"
#include "Header/PetState.h"

ABaseFlyingPet::ABaseFlyingPet()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	RootComponent = CollisionComp;
	// 카메라 채널 무시 설정
	CollisionComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	// 메쉬는 충돌을 처리하지 않음
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FloatingMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovement"));

	// 적 감지 범위 구체 컴포넌트 생성 및 설정
	EnemyDetectSphere = CreateDefaultSubobject<USphereComponent>(TEXT("EnemyDetectSphere"));
	EnemyDetectSphere->SetupAttachment(RootComponent);
	EnemyDetectSphere->ShapeColor = FColor::Green;
	EnemyDetectSphere->SetSphereRadius(EnemyDetectRange);
	EnemyDetectSphere->SetVisibility(false);
	EnemyDetectSphere->SetHiddenInGame(false);
	// 퀘스트/월드 트리거 볼륨과의 불필요한 오버랩을 방지하고 적 캐릭터(Pawn)만 감지하도록 설정
	EnemyDetectSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	EnemyDetectSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 아이템 감지 범위 구체 컴포넌트 생성 및 설정
	ItemDetectSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ItemDetectSphere"));
	ItemDetectSphere->SetupAttachment(RootComponent);
	ItemDetectSphere->ShapeColor = FColor::Blue;
	ItemDetectSphere->SetSphereRadius(ItemDetectRange);
	ItemDetectSphere->SetVisibility(false);
	ItemDetectSphere->SetHiddenInGame(false);
	// 월드 트리거 오버랩 방지 및 아이템(WorldDynamic)만 감지하도록 설정
	ItemDetectSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	ItemDetectSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);

	// 아이템 감지 핑 생성 위치 컴포넌트
	ItemDetectPingSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ItemDetectPingSpawnPoint"));
	ItemDetectPingSpawnPoint->SetupAttachment(RootComponent);
	
	// 펫 대화 컴포넌트 생성
	PetTalkComp = CreateDefaultSubobject<UPetTalkComponent>(TEXT("PetTalkComp"));
	
	// 펫 길안내/이정표 컴포넌트 생성
	PetGuideComp = CreateDefaultSubobject<UPetGuideComponent>(TEXT("PetGuideComp"));

	// 퀘스트 길안내 설정 기본값 초기화
	QuestGuideInterpSpeed = 0.5f;
	QuestGuideGroundHeight = 100.0f;

	// 펫 태그 추가
	Tags.Add("Pet");
}

void ABaseFlyingPet::BeginPlay()
{
	Super::BeginPlay();
	
	if (PetTalkComp)
	{
		// 대화가 끝나면 내 클래스의 EndConversation 함수를 실행해라! 라고 등록
		PetTalkComp->OnConversationEnded.AddDynamic(this, &ABaseFlyingPet::EndConversation);
	}

	// [추가] 외부에서 OnPetConversationStart를 Broadcast하면 자동으로 StartConversation이 실행되도록 연결
	OnPetConversationStart.AddDynamic(this, &ABaseFlyingPet::StartBigConversation);
	
	// 일정 주기마다 주변 적 감지 함수 호출 설정
	GetWorld()->GetTimerManager().SetTimer(
		EnemyDetectTimerHandle, 
		this, 
		&ABaseFlyingPet::CheckSurroundingEnemy, 
		EnemyDetectInterval, 
		true // 반복 여부: true
	);
	
	GetWorld()->GetTimerManager().SetTimer(
		LineOfSightTimerHandle, 
		this, 
		&ABaseFlyingPet::CheckLineOfSightToTarget, 
		2.0f, // 2초 간격
		true  // 반복 여부
	);
	
	// 3초 마다 아이템 감지
	GetWorld()->GetTimerManager().SetTimer(
		ItemDetectTimerHandle, 
		this, 
		&ABaseFlyingPet::CheckSurroundingItems, 
		3.0f, 
		true
	);
}

void ABaseFlyingPet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PollInit(DeltaTime);

	if (TargetActor && bIsFolloingTarget)
	{
		FollowingTarget(DeltaTime);
	}
}

void ABaseFlyingPet::PollInit(float DeltaTime)
{
	if (bTargetInitalize == false)
	{
		MasterCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		TargetActor = MasterCharacter;
       
		if (MasterCharacter) 
		{
			// ✅ 인터페이스를 통해 함수 호출
			if (MasterCharacter->GetClass()->ImplementsInterface(UPetConversationInterface::StaticClass()))
			{
				// Execute_ 함수명을 사용하여 안전하게 호출합니다.
				IPetConversationInterface::Execute_SetMyPet(MasterCharacter, this);
            
				bTargetInitalize = true;
				bIsFolloingTarget = true;
			}
		}
	}
}

void ABaseFlyingPet::SetFreeRoaming(bool bNewState)
{
	bIsFolloingTarget = bNewState;
}

void ABaseFlyingPet::SetTargetActor(AActor* NewTarget)
{
	if (NewTarget)
	{
		TargetActor = NewTarget;
		bIsFolloingTarget = true;
		ResetNavPath();
	}
}

void ABaseFlyingPet::ReturnToPlayer()
{
	if (MasterCharacter)
	{
		TargetActor = MasterCharacter;
	}
	else
	{
		MasterCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
		TargetActor = MasterCharacter;
	}
	bIsFolloingTarget = true;
	ResetNavPath();
}

void ABaseFlyingPet::TeleportToTargetActor(AActor* GoalActor)
{
	if (!GoalActor)
	{
		return;
	}

	FVector StartLoc = GetActorLocation();

	// 1. 출발 위치에 순간이동 이펙트 스폰
	if (TeleportEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TeleportEffect, StartLoc, FRotator::ZeroRotator);
	}

	// 2. 목표 액터 기준 위치 계산 (뒤/오른쪽 오프셋 및 지면 1m 높이 보정)
	FVector GoalLoc = GoalActor->GetActorLocation();
	FVector GoalForward = GoalActor->GetActorForwardVector();
	FVector GoalRight = GoalActor->GetActorRightVector();

	FVector FinalLocation = GoalLoc
		- (GoalForward * FollowSettings.Distance)
		+ (GoalRight * FollowSettings.SideOffset)
		+ FVector(0.f, 0.f, FollowSettings.UpOffset);

	// 지면 레이캐스트로 땅 위 정확히 1m(QuestGuideGroundHeight) 높이 보정
	FHitResult FloorHit;
	FVector TraceStart = FVector(FinalLocation.X, FinalLocation.Y, GoalLoc.Z + 300.0f);
	FVector TraceEnd = FVector(FinalLocation.X, FinalLocation.Y, GoalLoc.Z - 600.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GoalActor);
	if (MasterCharacter)
	{
		QueryParams.AddIgnoredActor(MasterCharacter);
	}

	if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams) ||
		GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		FinalLocation.Z = FloorHit.ImpactPoint.Z + QuestGuideGroundHeight;
	}
	else
	{
		FinalLocation.Z = GoalLoc.Z + QuestGuideGroundHeight;
	}

	// 3. 다가오는 플레이어를 마주보는 회전 계산
	FRotator TargetRot = GoalActor->GetActorRotation();
	if (MasterCharacter)
	{
		TargetRot = UKismetMathLibrary::FindLookAtRotation(FinalLocation, MasterCharacter->GetActorLocation());
		TargetRot.Pitch = FMath::Clamp(TargetRot.Pitch, -20.0f, 20.0f);
		TargetRot.Roll = 0.0f;
	}

	// 4. 물리 텔레포트 적용
	SetActorLocationAndRotation(FinalLocation, TargetRot, false, nullptr, ETeleportType::TeleportPhysics);

	// 5. 도착 위치에 순간이동 이펙트 스폰
	if (TeleportEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TeleportEffect, FinalLocation, FRotator::ZeroRotator);
	}

	// 6. 타겟 액터 전환 (도착 후에도 위치 유지)
	SetTargetActor(GoalActor);
	ResetNavPath();

	UE_LOG(LogTemp, Log, TEXT("[BaseFlyingPet] Teleported to Target Actor: %s"), *GoalActor->GetName());
}

void ABaseFlyingPet::ResetNavPath()
{
	NavPathPoints.Empty();
	CurrentNavPathIndex = 0;
	LastNavTargetLocation = FVector::ZeroVector;
	NavPathUpdateTimer = 0.0f;
	StuckTimer = 0.0f;
	TemporaryStuckLiftZ = 0.0f;
	CurrentObstacleAltitudeOffset = 0.0f;
	LastStuckCheckLocation = GetActorLocation();
}

float ABaseFlyingPet::CalculateObstacleClearanceZ(const FVector& StartLoc, const FVector& EndLoc)
{
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (TargetActor) QueryParams.AddIgnoredActor(TargetActor);
	if (MasterCharacter) QueryParams.AddIgnoredActor(MasterCharacter);

	// 출발지에서 목적지 방향으로 장애물(WorldStatic) 검사
	if (GetWorld()->LineTraceSingleByChannel(Hit, StartLoc, EndLoc, ECC_WorldStatic, QueryParams))
	{
		// 장애물 꼭대기 높이를 찾기 위해 위에서 아래로 레이캐스트
		FHitResult TopHit;
		float MaxZ = FMath::Max(StartLoc.Z, EndLoc.Z) + 800.0f;
		FVector SearchHigh = FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, MaxZ);
		FVector SearchLow = Hit.ImpactPoint;

		if (GetWorld()->LineTraceSingleByChannel(TopHit, SearchHigh, SearchLow, ECC_WorldStatic, QueryParams))
		{
			// 장애물 꼭대기 표면보다 130cm 위로 상승 유도
			float DesiredClearanceZ = TopHit.ImpactPoint.Z + 130.0f;
			if (DesiredClearanceZ > StartLoc.Z)
			{
				return DesiredClearanceZ - StartLoc.Z;
			}
		}
	}
	return 0.0f;
}

bool ABaseFlyingPet::UpdateNavPath(const FVector& TargetDesiredLocation)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseFlyingPet] NavigationSystem not found in World!"));
		return false;
	}

	FVector StartLocation = GetActorLocation();
	FVector EndLocation = TargetDesiredLocation;

	// 바닥 내비메시 상의 정확한 지점을 찾기 위해 수직 레이캐스트로 지면 탐색
	FHitResult StartHit, EndHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (TargetActor) QueryParams.AddIgnoredActor(TargetActor);
	if (MasterCharacter) QueryParams.AddIgnoredActor(MasterCharacter);

	if (GetWorld()->LineTraceSingleByChannel(StartHit, StartLocation + FVector(0.f, 0.f, 200.f), StartLocation - FVector(0.f, 0.f, 2000.f), ECC_WorldStatic, QueryParams))
	{
		StartLocation = StartHit.ImpactPoint;
	}

	if (GetWorld()->LineTraceSingleByChannel(EndHit, EndLocation + FVector(0.f, 0.f, 200.f), EndLocation - FVector(0.f, 0.f, 2000.f), ECC_WorldStatic, QueryParams))
	{
		EndLocation = EndHit.ImpactPoint;
	}

	// [정밀 보정] 내비메시 표면으로 정확히 프로젝션(스냅)하여 탐색 실패 방지
	FNavLocation ProjectedStart, ProjectedEnd;
	const FVector Extent(500.f, 500.f, 1000.f);
	if (NavSys->ProjectPointToNavigation(StartLocation, ProjectedStart, Extent))
	{
		StartLocation = ProjectedStart.Location;
	}
	if (NavSys->ProjectPointToNavigation(EndLocation, ProjectedEnd, Extent))
	{
		EndLocation = ProjectedEnd.Location;
	}

	// [핵심] PathfindingAgent로 펫(this) 대신 MasterCharacter(플레이어) 또는 nullptr을 넘겨 지면 표준 NavMesh 사용
	AActor* AgentActor = MasterCharacter ? MasterCharacter : nullptr;
	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), StartLocation, EndLocation, AgentActor);

	if (!NavPath || !NavPath->IsValid() || NavPath->PathPoints.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BaseFlyingPet] NavPath Search FAILED (Points: %d)! Fallback to direct fly."),
			NavPath ? NavPath->PathPoints.Num() : 0);
		return false;
	}

	NavPathPoints.Empty();
	float TargetHeight = (MasterCharacter && TargetActor != MasterCharacter) ? QuestGuideGroundHeight : FollowSettings.UpOffset;
	if (TargetHeight <= 10.0f)
	{
		TargetHeight = 100.0f;
	}

	for (const FVector& Pt : NavPath->PathPoints)
	{
		// 지면 경로점의 Z축을 공중 비행 높이만큼 띄워줌
		NavPathPoints.Add(Pt + FVector(0.0f, 0.0f, TargetHeight));
	}

	// 0번 포인트는 시작 지점 근처이므로 1번 경유지부터 추적
	CurrentNavPathIndex = 1;
	LastNavTargetLocation = TargetDesiredLocation;

	UE_LOG(LogTemp, Log, TEXT("[BaseFlyingPet] NavPath Found! Total Waypoints: %d"), NavPathPoints.Num());

	// 디버그 경로선 시각화 (선명한 초록색 선 및 노란색 경유점 구체)
	if (bDebugMode)
	{
		for (int32 i = 0; i < NavPathPoints.Num() - 1; ++i)
		{
			DrawDebugLine(GetWorld(), NavPathPoints[i], NavPathPoints[i + 1], FColor::Green, false, 5.0f, 0, 4.0f);
			DrawDebugSphere(GetWorld(), NavPathPoints[i], 20.0f, 12, FColor::Yellow, false, 5.0f);
		}
		if (NavPathPoints.Num() > 0)
		{
			DrawDebugSphere(GetWorld(), NavPathPoints.Last(), 30.0f, 12, FColor::Cyan, false, 5.0f);
		}
	}

	return true;
}

FVector ABaseFlyingPet::GetNextNavWaypoint(const FVector& FinalDesiredLocation)
{
	if (!bUseNavMeshFlight || NavPathPoints.Num() == 0 || CurrentNavPathIndex >= NavPathPoints.Num())
	{
		return FinalDesiredLocation;
	}

	FVector CurrentLocation = GetActorLocation();

	while (CurrentNavPathIndex < NavPathPoints.Num())
	{
		FVector Waypoint = NavPathPoints[CurrentNavPathIndex];
		float Dist2D = FVector::Dist2D(CurrentLocation, Waypoint);

		// 마지막 웨이포인트가 아닌 경우 거리 체크로 다음으로 진행
		if (CurrentNavPathIndex < NavPathPoints.Num() - 1)
		{
			if (Dist2D <= NavWaypointAcceptanceRadius)
			{
				CurrentNavPathIndex++;
				continue;
			}

			// 지름길(Corner Shortcut) 검사: 다다음 경로점까지 벽 없이 뚫려있다면 바로 건너뛰어 자연스러운 곡선 유영
			if (CurrentNavPathIndex + 1 < NavPathPoints.Num())
			{
				FHitResult Hit;
				FCollisionQueryParams Params;
				Params.AddIgnoredActor(this);
				if (TargetActor) Params.AddIgnoredActor(TargetActor);
				if (MasterCharacter) Params.AddIgnoredActor(MasterCharacter);

				FVector NextNextPt = NavPathPoints[CurrentNavPathIndex + 1];
				if (!GetWorld()->LineTraceSingleByChannel(Hit, CurrentLocation, NextNextPt, ECC_WorldStatic, Params))
				{
					CurrentNavPathIndex++;
					continue;
				}
			}
		}
		break;
	}

	if (CurrentNavPathIndex < NavPathPoints.Num())
	{
		FVector SelectedPt = NavPathPoints[CurrentNavPathIndex];
		// 마지막 경유지일 때는 원래 목적지의 정확한 높이를 사용
		if (CurrentNavPathIndex == NavPathPoints.Num() - 1)
		{
			SelectedPt.Z = FinalDesiredLocation.Z;
		}
		return SelectedPt;
	}

	return FinalDesiredLocation;
}

void ABaseFlyingPet::ProcessStuckRecovery(float DeltaTime, const FVector& FinalDesiredLocation)
{
	FVector CurrentLoc = GetActorLocation();
	float DistToFinal = FVector::Dist(CurrentLoc, FinalDesiredLocation);

	// 최종 목적지 반경에 거의 도달했다면 끼임 체크 해제
	if (DistToFinal <= 120.0f)
	{
		StuckTimer = 0.0f;
		TemporaryStuckLiftZ = FMath::FInterpTo(TemporaryStuckLiftZ, 0.0f, DeltaTime, 3.0f);
		LastStuckCheckLocation = CurrentLoc;
		return;
	}

	// 직전 체크 위치와의 이동 거리
	float MoveDist = FVector::Dist(CurrentLoc, LastStuckCheckLocation);
	if (MoveDist < 15.0f) // 거의 전진하지 못하고 벽/장애물에 걸림
	{
		StuckTimer += DeltaTime;

		// 1초 이상 정체: 공중으로 살짝 상승하여 낮은 난간/울타리 극복 시도
		if (StuckTimer >= 1.0f && StuckTimer < StuckTeleportThresholdTime)
		{
			TemporaryStuckLiftZ = FMath::FInterpTo(TemporaryStuckLiftZ, StuckAvoidanceHeight, DeltaTime, 2.0f);
		}
		// 3초 이상 완전 끼임: 다음 웨이포인트나 목표 지점으로 안전 탈출 텔레포트
		else if (StuckTimer >= StuckTeleportThresholdTime)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BaseFlyingPet] Stuck detected for %.1fs! Escaping via recovery teleport."), StuckTimer);

			FVector EscapeLoc = FinalDesiredLocation;
			if (NavPathPoints.Num() > 0 && CurrentNavPathIndex < NavPathPoints.Num())
			{
				EscapeLoc = NavPathPoints[CurrentNavPathIndex];
			}

			SetActorLocation(EscapeLoc, false, nullptr, ETeleportType::TeleportPhysics);
			if (TeleportEffect)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TeleportEffect, EscapeLoc, FRotator::ZeroRotator);
			}

			StuckTimer = 0.0f;
			TemporaryStuckLiftZ = 0.0f;
			CurrentNavPathIndex++;
			LastStuckCheckLocation = EscapeLoc;
		}
	}
	else
	{
		// 정상 이동 중이면 타이머 리셋
		StuckTimer = 0.0f;
		TemporaryStuckLiftZ = FMath::FInterpTo(TemporaryStuckLiftZ, 0.0f, DeltaTime, 3.0f);
		LastStuckCheckLocation = CurrentLoc;
	}
}

void ABaseFlyingPet::FollowingTarget(float DeltaTime)
{
	if (!TargetActor) return;

	// 상태(PetState)에 따른 설정값 선택 
	FPetPositionSettings CurrentSettings;
	switch (PetState)
	{
	case EPetState::EPS_Conversation:
		CurrentSettings = ConversationSettings;
		break;
	case EPetState::EPS_Follow:
		CurrentSettings = FollowSettings;
		break;
	default:
		CurrentSettings = FollowSettings;
		break;
	}

	FVector CurrentLocation = GetActorLocation();
	FVector TargetLocation = TargetActor->GetActorLocation();

	// --- 1. 목표 위치 계산 ---
	FVector TargetForward = TargetActor->GetActorForwardVector();
	FVector TargetRight = TargetActor->GetActorRightVector();

	// 공식 == 타겟 위치 - (앞방향 * 거리) + (오른쪽방향 * 좌우오프셋) + (위방향 * 높이오프셋)
	FVector DesiredLocation = TargetLocation
		- (TargetForward * CurrentSettings.Distance)    // 거리
		+ (TargetRight * CurrentSettings.SideOffset)    // 좌우
		+ FVector(0.0f, 0.0f, CurrentSettings.UpOffset); // 높이

	// [지면 높이 보정] 퀘스트 액터로 이동했을 때: 실제 땅(지면)으로부터 정확히 1m(QuestGuideGroundHeight) 띄우도록 레이캐스트 보정
	if (MasterCharacter && TargetActor != MasterCharacter)
	{
		FHitResult FloorHit;
		FVector TraceStart = FVector(DesiredLocation.X, DesiredLocation.Y, TargetLocation.Z + 300.0f);
		FVector TraceEnd = FVector(DesiredLocation.X, DesiredLocation.Y, TargetLocation.Z - 600.0f);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(TargetActor);
		if (MasterCharacter)
		{
			QueryParams.AddIgnoredActor(MasterCharacter);
		}

		if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams) ||
			GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
		{
			DesiredLocation.Z = FloorHit.ImpactPoint.Z + QuestGuideGroundHeight;
		}
		else
		{
			DesiredLocation.Z = TargetLocation.Z + QuestGuideGroundHeight;
		}
	}

	// [내비메시 경로 갱신]
	if (bUseNavMeshFlight)
	{
		NavPathUpdateTimer += DeltaTime;
		bool bNeedRepath = (NavPathPoints.Num() == 0) ||
						   (NavPathUpdateTimer >= NavPathUpdateInterval && FVector::Dist(DesiredLocation, LastNavTargetLocation) > 150.0f);

		if (bNeedRepath)
		{
			NavPathUpdateTimer = 0.0f;
			UpdateNavPath(DesiredLocation);
		}
	}

	// [끼임 감지 및 단계별 탈출 처리]
	ProcessStuckRecovery(DeltaTime, DesiredLocation);

	// 실제 비행할 목표 경유점 (내비메시 경로가 있으면 다음 웨이포인트, 없으면 DesiredLocation)
	FVector MoveTargetLocation = DesiredLocation;
	if (bUseNavMeshFlight && NavPathPoints.Num() > 0)
	{
		MoveTargetLocation = GetNextNavWaypoint(DesiredLocation);
	}

	// [3D 비행 장애물 자동 극복]: 전방에 돌담, 울타리, 단차가 가로막고 있으면 장애물 꼭대기 높이를 측정해 고도를 상승!
	float NeededClearance = CalculateObstacleClearanceZ(CurrentLocation, MoveTargetLocation);
	if (NeededClearance > CurrentObstacleAltitudeOffset)
	{
		CurrentObstacleAltitudeOffset = FMath::FInterpTo(CurrentObstacleAltitudeOffset, NeededClearance, DeltaTime, 4.0f);
	}
	else
	{
		CurrentObstacleAltitudeOffset = FMath::FInterpTo(CurrentObstacleAltitudeOffset, 0.0f, DeltaTime, 2.0f);
	}
	MoveTargetLocation.Z += (CurrentObstacleAltitudeOffset + TemporaryStuckLiftZ);

	// 이동 거리 및 속도 계산
	float DistanceToWaypoint = FVector::Dist(CurrentLocation, MoveTargetLocation);
	float DistanceToFinalTarget = FVector::Dist(CurrentLocation, DesiredLocation);
	float FinalInterpSpeed = MoveInterpSpeed;
	float CurrentMaxSpeed = MaxFollowFlightSpeed;

	if (MasterCharacter && TargetActor != MasterCharacter)
	{
		// [퀘스트 액터 안내 비행]: 급발진 없이 차분하고 여유로운 유영 속도 유지
		FinalInterpSpeed = QuestGuideInterpSpeed;
		CurrentMaxSpeed = MaxGuideFlightSpeed;
	}
	else
	{
		// [평소 플레이어 추적]: 플레이어와 거리가 멀어졌을 때만 완만하게 가속
		float MoveSpeedMultiplier = FMath::Clamp(1.0f + (DistanceToWaypoint * 0.005f), 1.0f, 2.5f);
		FinalInterpSpeed = MoveInterpSpeed * MoveSpeedMultiplier;
		CurrentMaxSpeed = MaxFollowFlightSpeed;
	}
    
	// 보간된 새로운 위치 계산
	FVector NewLocation = FMath::VInterpTo(CurrentLocation, MoveTargetLocation, DeltaTime, FinalInterpSpeed);

	// [속도 제한] 프레임당 최대 이동 거리(MaxStep)를 제한하여 순간 급발진을 완전히 방지하고 일정한 유영 속도 유지
	FVector MoveDelta = NewLocation - CurrentLocation;
	float MaxStep = CurrentMaxSpeed * DeltaTime;
	if (MoveDelta.SizeSquared() > (MaxStep * MaxStep))
	{
		NewLocation = CurrentLocation + (MoveDelta.GetSafeNormal() * MaxStep);
	}

	// bSweep을 true로 설정하여 충돌 감지 활성화
	FHitResult Hit;
	bool bMoved = SetActorLocation(NewLocation, true, &Hit);

	// 만약 벽에 부딪혔다면 (충돌이 발생했다면)
	if (Hit.IsValidBlockingHit())
	{
		// 1. 벽을 타고 미끄러지는 이동(Slide)
		FVector RemainingDelta = NewLocation - Hit.Location;
		FVector SlideDelta = FVector::VectorPlaneProject(RemainingDelta, Hit.Normal);
		AddActorWorldOffset(SlideDelta, true);

		// 2. [비행 펫 특권]: 벽이나 돌담 난간에 부딪혔을 때 수직 상승하여 장애물 턱을 사뿐히 타넘음!
		AddActorWorldOffset(FVector(0.0f, 0.0f, 300.0f * DeltaTime), true);
	}
    
	// 속도 계산 로직 유지
	if (DeltaTime > KINDA_SMALL_NUMBER)
	{
		CurrentVelocity = (GetActorLocation() - CurrentLocation) / DeltaTime;
	}
	
	// --- 2. 회전 계산 (상태에 따른 분기) ---
	FRotator TargetRotation;

	if (PetState == EPetState::EPS_Conversation)
	{
		// [대화 상태] 펫이 캐릭터의 눈높이를 바라보도록 회전 (LookAt)
		FVector LookTarget = TargetLocation + FVector(0.f, 0.f, 60.f);
		TargetRotation = UKismetMathLibrary::FindLookAtRotation(CurrentLocation, LookTarget);
		TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch, -20.0f, 20.0f);
		TargetRotation.Roll = 0.0f;
	}
	else if (MasterCharacter && TargetActor != MasterCharacter)
	{
		// [퀘스트 액터 안내 상태]
		bool bArrivedAtQuest = (PetGuideComp && PetGuideComp->HasArrived()) || (DistanceToFinalTarget <= 150.0f);
		if (!bArrivedAtQuest)
		{
			// 비행 이동 중: 진행 방향을 바라봄
			FVector MoveDir = (MoveTargetLocation - CurrentLocation).GetSafeNormal();
			TargetRotation = MoveDir.IsNearlyZero() ? TargetActor->GetActorRotation() : MoveDir.Rotation();
		}
		else
		{
			// 퀘스트 지점 도착 후: 다가오는 플레이어(나)를 마주봄 (LookAt Player)
			FVector LookTarget = MasterCharacter->GetActorLocation() + FVector(0.f, 0.f, 60.f);
			TargetRotation = UKismetMathLibrary::FindLookAtRotation(CurrentLocation, LookTarget);
			TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch, -20.0f, 20.0f);
			TargetRotation.Roll = 0.0f;
		}
	}
	else
	{
		// [평소 플레이어 동행 상태]
		// 펫이 항상 나(플레이어 캐릭터)를 마주보도록 회전 (LookAt Player)
		if (MasterCharacter)
		{
			FVector LookTarget = MasterCharacter->GetActorLocation() + FVector(0.f, 0.f, 60.f);
			TargetRotation = UKismetMathLibrary::FindLookAtRotation(CurrentLocation, LookTarget);
			TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch, -20.0f, 20.0f);
			TargetRotation.Roll = 0.0f;
		}
		else
		{
			TargetRotation = TargetActor->GetActorRotation();
		}
	}

	// 회전 보간: 자연스럽고 기민하게 회전 (기본 속도 6.0f)
	FRotator CurrentRotation = GetActorRotation();
	float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetRotation.Yaw));
	float RotSpeedMultiplier = FMath::Clamp(1.0f + (DeltaYaw * 0.05f), 1.0f, 3.0f);
	float FinalRotationSpeed = 6.0f * RotSpeedMultiplier;

	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, FinalRotationSpeed);
	SetActorRotation(NewRotation);
}

void ABaseFlyingPet::CheckLineOfSightToTarget()
{
	if (!TargetActor) return;

	// 퀘스트 액터를 안내 중이거나 퀘스트 액터에 붙어있을 때는 장애물 시야 검사(순간이동)를 건너뜀
	if ((PetGuideComp && PetGuideComp->IsGuiding()) || (MasterCharacter && TargetActor != MasterCharacter))
	{
		LineOfSightBlockedCount = 0;
		return;
	}

	FHitResult Hit;
	FVector Start = GetActorLocation();
	FVector End = TargetActor->GetActorLocation();
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(TargetActor);

	// LineTrace 등을 사용하여 시야 방해물이 있는지 확인하는 로직이 들어갈 자리입니다.
	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	
	if ( bHit == true )
	{
		LineOfSightBlockedCount++;
		
		if (LineOfSightBlockedCount >= 2)
		{
			FVector TargetLocation = TargetActor->GetActorLocation();
			FVector TargetForward = TargetActor->GetActorForwardVector();
			FVector TargetRight = TargetActor->GetActorRightVector();

			// 1. 가고 싶은 이상적인 위치 (FollowSettings 기준)
			FVector DesiredLocation = TargetLocation
				- (TargetForward * FollowSettings.Distance)
				+ (TargetRight * FollowSettings.SideOffset)
				+ FVector(0.0f, 0.0f, FollowSettings.UpOffset);

			// 2. 캐릭터에서 목표 지점으로 레이를 쏴서 "실제로 갈 수 있는 끝점" 찾기
			FHitResult SweepHit;
			FVector TraceStart = TargetLocation + FVector(0.0f, 0.0f, FollowSettings.UpOffset);
			FVector FinalLocation = DesiredLocation;

			if (GetWorld()->LineTraceSingleByChannel(SweepHit, TraceStart, DesiredLocation, ECC_Visibility, Params))
			{
				// 벽에 부딪혔다면, 그 충돌 지점에서 살짝 안쪽으로 들어온 위치를 최종 목적지로 설정
				FinalLocation = SweepHit.Location + (SweepHit.Normal * 20.0f);
			}

			// 3. 순간이동 실행 (이때는 이미 안전한 위치를 계산했으므로 bSweep은 false여도 됨)
			SetActorLocation(FinalLocation, false, nullptr, ETeleportType::TeleportPhysics);
			
			// 로그 확인 (선택 사항)
			UE_LOG(LogTemp, Warning, TEXT("Pet Teleported: Line of sight blocked by %s"), *Hit.GetActor()->GetName());
			
			LineOfSightBlockedCount = 0; // 순간이동 후 카운트 초기화
			// 순간이동 이펙트 재생
			if (TeleportEffect)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TeleportEffect, 
					FinalLocation, FRotator::ZeroRotator);
			}
		}
	}
	else LineOfSightBlockedCount = 0; // 시야가 확보된 경우 카운트 초기화
}

void ABaseFlyingPet::CheckSurroundingEnemy()
{
	// 현재 상태가 대화 중이면 적 감지 무시
	if (!EnemyDetectSphere || PetState == EPetState::EPS_Conversation) return;
	
	TArray<AActor*> OverlappingActors;
	// 현재 스피어 안에 있는 모든 액터를 가져옵니다. (필터링할 클래스가 있다면 두 번째 인자에 넣음)
	EnemyDetectSphere->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());

	bool bEnemyFound = false;

	for (AActor* Actor : OverlappingActors)
	{
		// 나 자신은 제외
		if (Actor == this) continue;

		// 적 태그 확인 (혹은 인터페이스나 클래스 캐스팅 확인)
		if (Actor->ActorHasTag("Enemy"))
		{
			// 배틀 상태가 아니면 타겟과의 시야 체크를 해서 벽으로 가려진 적은 무시하도록 (배틀 상태에서는 일단 감지된 적은 모두 적으로 간주)
			if ( PetState != EPetState::EPS_Battle && TraceCharacterToTarget( Actor ) == false ) continue;
			
			// 미믹 몬스터 또는 버로우 몬스터는 무시
			if (Actor->ActorHasTag("Mimic") || Actor->ActorHasTag("Burrow") 
				|| Actor->ActorHasTag("Trans")) continue;
			
			// Boss 태그 
			if (Actor->ActorHasTag("Boss"))
			{
				if (bBossBattleMode == false)
				{
					bBossBattleMode = true;
					EnemyDetectSphere->SetSphereRadius(30000.f);
					
					HealCharacter(); // 보스전 시작 시 캐릭터 힐링
					
					UE_LOG(LogTemp, Warning, TEXT("BossBattleMode Detected"));
				}
				bEnemyFound = true; // ◀ 보스도 적으로 간주하여 상태 업데이트 로직으로 넘어가게 함
				break; // 보스를 찾았으니 반복문 탈출
			}
			
			bEnemyFound = true;
			break; // 한 명이라도 있으면 배틀 모드이므로 더 검사할 필요 없음
		}
	}

	// 상태 업데이트
	if (bEnemyFound == true )
	{
		if (PetState != EPetState::EPS_Battle)
		{
			// 평상시 상태였는데 주변 적이 있다는 의미
			PetState = EPetState::EPS_Battle;
			//bIsFolloingTarget = false; // 배틀 모드 진입 시 자유 이동 모드로 전환
			
			if ( bBossBattleMode == true && PetTalkComp ) PetTalkComp->Travel_FollowToBattle( true );
			else if ( bBossBattleMode == false && PetTalkComp ) PetTalkComp->Travel_FollowToBattle( false );
		}
	}
	else if ( bEnemyFound == false && bBossBattleMode == true ) // 보스전 모드 해제 조건
	{
		PetState = EPetState::EPS_Follow;
		if ( PetTalkComp ) PetTalkComp->Travel_BattleToFollow( true );
		UE_LOG(LogTemp, Warning, TEXT("BossBattleMode Ended - EnemyDetectRange Restore"));
		bBossBattleMode = false;
		//bIsFolloingTarget = true; // 다시 따라다니기 모드로 전환
		EnemyDetectSphere->SetSphereRadius(EnemyDetectRange); // 원래 범위으로 복귀	
	}
	else
	{
		if (PetState == EPetState::EPS_Battle) 
		{
			// 배틀 모드에서 벗어나 평상시 상태로 복귀
			PetState = EPetState::EPS_Follow;
			if ( PetTalkComp ) PetTalkComp->Travel_BattleToFollow( false );
			//bIsFolloingTarget = true; // 다시 따라다니기 모드로 전환
		}
	}
}

void ABaseFlyingPet::HealCharacter()
{
	AActor* HealTarget = MasterCharacter ? Cast<AActor>(MasterCharacter) : TargetActor;
	if (HealTarget)
	{
		// 플레이어한테 - 대미지 주기 
		UGameplayStatics::ApplyDamage(HealTarget, -HealAmount, 
			nullptr, this, nullptr);
		
		// 2. 나이아가라 이펙트를 그 자리(Location)에 생성
		if (HealEffect)
		{
			// SpawnSystemAtLocation을 사용하면 부착되지 않고 월드 좌표에 고정되어 실행됩니다.
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				HealEffect,
				HealTarget->GetActorLocation(), // 플레이어의 현재 위치(고정값)
				FRotator::ZeroRotator,
				FVector(2.f),                    // 스케일
				true                         // 자동 파괴
			);
		}
	}
}

bool ABaseFlyingPet::TraceCharacterToTarget(AActor* Target)
{
	AActor* BaseActor = MasterCharacter ? Cast<AActor>(MasterCharacter) : TargetActor;
	if (!BaseActor || !Target) return false;

	FHitResult Hit;
	FVector Start = BaseActor->GetActorLocation() + (FVector::UpVector * 50.f);
	FVector End = Target->GetActorLocation() + (FVector::UpVector * 50.f); 
	
	// 캐릭터의 눈높이 정도를 고려한다면 높이 오프셋을 추가할 수 있습니다.
	// Start.Z += 50.f; 
	// End.Z += 50.f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);          // 펫 무시
	Params.AddIgnoredActor(BaseActor);     // 플레이어 무시
	Params.AddIgnoredActor(Target);        // 타겟 무시 (장애물만 체크하기 위함)

	// ECC_Visibility 채널을 사용하여 가시성을 가로막는 물체(벽 등)가 있는지 검사
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, 
		Start, 
		End, 
		ECC_Visibility, 
		Params
	);
	
	if (bHit)
	{
		// 무엇에 부딪혔는지 로그 출력
		UE_LOG(LogTemp, Warning, TEXT("Trace Blocked by: %s"), *Hit.GetActor()->GetName());
		//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 0, 2.0f);
	}

	// 무언가에 부딪혔다면 시야가 가려진 것이므로 false 리턴
	return !bHit;
}

void ABaseFlyingPet::CheckSurroundingItems()
{
	// 따라다니기 상태가 아니면 무시
	if (PetState != EPetState::EPS_Follow || !ItemDetectSphere) return;

	TArray<AActor*> OverlappingActors;
	// 범위 내의 모든 액터를 가져옴
	ItemDetectSphere->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor || Actor == this) continue;

		// 태그 확인 (Item 또는 Mimic)
		bool bIsItem = Actor->ActorHasTag("Item");
		bool bIsMimic = Actor->ActorHasTag("Mimic");

		if (bIsItem || bIsMimic)
		{
			// 1. 벽 체크 (플레이어와 아이템 사이 시야 확인)
			if (TraceCharacterToTarget(Actor) == false) continue;

			// 2. 대화 컴포넌트로 전달 (이미 대화한 아이템 히스토리 처리는 PetTalkComp 내부에서 수행됨)
			if (PetTalkComp)
			{
				PetTalkComp->Travel_ItemDetect(Actor, ItemDetectPingSpawnPoint->GetComponentLocation());
			}
		}
	}
}

void ABaseFlyingPet::TriggerPetBigConversation_Implementation(FName DialogueID)
{
	StartBigConversation(DialogueID);
}

void ABaseFlyingPet::TriggerPetSmallConversation_Implementation(FName DialogueID)
{
	StartSmallConversation(DialogueID);
}

void ABaseFlyingPet::SetPetState_Implementation(EPetState NewState)
{
	PetState = NewState;
}

void ABaseFlyingPet::PlayPetMontageFromConversation_Implementation(UAnimMontage* MontageToPlay)
{
	// MeshComp와 몽타주가 유효한지 확인
	if (MeshComp && MontageToPlay)
	{
		// 스켈레톤 일치 여부 확인
		// 몽타주가 사용하는 스켈레톤과 현재 펫 메쉬의 스켈레톤이 다르면 재생하지 않도록 방어 코드 추가
		USkeletalMesh* CurrentMeshAsset = MeshComp->GetSkeletalMeshAsset();
		if (CurrentMeshAsset)
		{
			USkeleton* MeshSkeleton = CurrentMeshAsset->GetSkeleton();
			USkeleton* MontageSkeleton = MontageToPlay->GetSkeleton();

			if (MeshSkeleton != MontageSkeleton)
			{
				// 스켈레톤이 다르면 경고 로그를 남기고 함수 종료
				UE_LOG(LogTemp, Error, TEXT(" NotMatch Skeleton - Montage cannot be played on this pet. "));
				return;
			}
		}
		
		// pawn은 애님 인스턴스를 직접 가져와서 재생해야 함
		UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(MontageToPlay);
		}
	}
}

void ABaseFlyingPet::StartBigConversation( FName DialogueID )
{
	// 2. 대화 컴포넌트에 실제 대화 시작 요청
	if (PetTalkComp)
	{
		PetTalkComp->ResetConversationLogScrollBox();
		PetTalkComp->StartConversation(DialogueID);
	}
}

void ABaseFlyingPet::EndConversation()
{
	// 따라다니기 상태로 복귀
	PetState = EPetState::EPS_Follow;
}

void ABaseFlyingPet::StartSmallConversation(FName DialogueID)
{
	if (PetTalkComp)
	{
		PetTalkComp->Travel_StartSmallConversation(DialogueID);
	}
}

#if WITH_EDITOR
void ABaseFlyingPet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 변경된 프로퍼티의 이름을 가져옵니다.
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// 디버그 모드에 따라 어택, 디텍트, 체이스 범위 구체의 가시성을 설정합니다.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABaseFlyingPet, bDebugMode))
	{
		if ( bDebugMode == true )
		{
			if ( EnemyDetectSphere ) EnemyDetectSphere->SetVisibility(true);
			if ( ItemDetectSphere ) ItemDetectSphere->SetVisibility(true);
		}
		else
		{
			if ( EnemyDetectSphere ) EnemyDetectSphere->SetVisibility(false);
			if ( ItemDetectSphere ) ItemDetectSphere->SetVisibility(false);
		}
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABaseFlyingPet, EnemyDetectRange))
	{
		if (EnemyDetectSphere)
		{
			EnemyDetectSphere->SetSphereRadius(EnemyDetectRange);
		}
	}
	if ( PropertyName == GET_MEMBER_NAME_CHECKED(ABaseFlyingPet, ItemDetectRange) )
	{
		if ( ItemDetectSphere )
		{
			ItemDetectSphere->SetSphereRadius(ItemDetectRange);
		}
	}
}
#endif