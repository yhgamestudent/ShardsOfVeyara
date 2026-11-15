// Fill out your copyright notice in the Description page of Project Settings.

#include "SOVShortcutInteractComponent.h"
#include "Character/AGSDCharacter.h"
#include "AGSDPlayerController.h"
#include "SOVGameInstance.h"
#include "Character/Components/AGSDInteractionComponent.h"
#include "Inventory/AGSDInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SOVShortcutRequirementWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/ShapeComponent.h"
#include "Engine/DataTable.h"
#include "Struct_ItemData.h"

// ── ASOVInteractionProxyActor 구현 ──

void ASOVInteractionProxyActor::Interact_Implementation(AAGSDCharacter* player)
{
	if (OwnerComponent)
	{
		OwnerComponent->TryUnlockShortcut(player);
	}
}

void ASOVInteractionProxyActor::ShowWidget_Implementation(ACharacter* player)
{
	if (OwnerComponent)
	{
		OwnerComponent->HandleShowWidget(player);
	}
}

bool ASOVInteractionProxyActor::CanInteract_Implementation(AAGSDCharacter* player)
{
	if (OwnerComponent)
	{
		return OwnerComponent->HandleCanInteract(player);
	}
	return false;
}

FString ASOVInteractionProxyActor::GetInteractionActionType_Implementation(AAGSDCharacter* player)
{
	return TEXT("Shortcut");
}

// ── USOVShortcutInteractComponent 구현 ──

USOVShortcutInteractComponent::USOVShortcutInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // 평소에는 틱을 꺼두어 성능 최적화

	// 기본 구체 트리거 반경 및 콜리전 프리셋 설정
	InitSphereRadius(250.0f);
	SetCollisionProfileName(TEXT("Trigger"));
	SetGenerateOverlapEvents(true);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 에디터 뷰포트에서 시각적으로 잘 식별되도록 와이어프레임 색상 설정
	ShapeColor = FColor(70, 230, 130, 255);

	// 부모 액터에서 자동으로 찾아 실행할 기본 함수명 목록 (이동 발판 및 도어 공용)
	AutoInvokeFunctionNames.Add(FName("DoorOpen"));
	AutoInvokeFunctionNames.Add(FName("ActivatePlatform"));
	AutoInvokeFunctionNames.Add(FName("StartMoving"));
	AutoInvokeFunctionNames.Add(FName("Activate"));
	AutoInvokeFunctionNames.Add(FName("Move"));
	AutoInvokeFunctionNames.Add(FName("Open"));

	// 액터 머리 위 3D/스크린 공간 위젯 컴포넌트 생성 및 기본값 설정 (몬스터 체력바 동일 구조)
	WorldWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WorldWidgetComponent"));
	WorldWidgetComponent->SetupAttachment(this);
	WorldWidgetComponent->SetRelativeLocation(WidgetRelativeOffset);
	WorldWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen); // 항상 카메라를 정면으로 바라봄
	WorldWidgetComponent->SetDrawSize(WidgetDrawSize);
	WorldWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WorldWidgetComponent->SetHiddenInGame(true);
	WorldWidgetComponent->SetVisibility(false);
}

void USOVShortcutInteractComponent::BeginPlay()
{
	Super::BeginPlay();

	// 맵 진입 시 이미 숏컷이 열려있는지 확인
	if (IsShortcutAlreadyUnlocked())
	{
		UE_LOG(LogTemp, Log, TEXT("[USOVShortcutInteractComponent] Shortcut [%s] is already unlocked on map load. Triggering open state."), *ShortcutTag.ToString());

		// 이미 개방된 상태 이벤트 브로드캐스트
		OnShortcutAlreadyUnlocked.Broadcast(true);

		// 부모 액터에 활성화 함수(ActivatePlatform, DoorOpen 등)가 있다면 자동 호출
		if (bAutoTriggerDoorOpen)
		{
			InvokeParentDoorOpen();
		}

		// 이미 열려있으므로 스피어 콜리전 비활성화 및 컴포넌트 파괴
		DisableAndDestroySphereCollision();
		return;
	}

	// 닫혀있는 상태라면 위젯 컴포넌트 클래스 및 초기 위치 동기화
	if (WorldWidgetComponent)
	{
		if (RequirementWidgetClass)
		{
			WorldWidgetComponent->SetWidgetClass(RequirementWidgetClass);
		}
		WorldWidgetComponent->SetRelativeLocation(WidgetRelativeOffset);
		WorldWidgetComponent->SetDrawSize(WidgetDrawSize);
		WorldWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		WorldWidgetComponent->InitWidget();
		WorldWidgetComponent->SetHiddenInGame(true);
		WorldWidgetComponent->SetVisibility(false);
	}

	// 닫혀있는 상태라면 오버랩 델리게이트 바인딩
	OnComponentBeginOverlap.AddDynamic(this, &USOVShortcutInteractComponent::OnOverlapBegin);
	OnComponentEndOverlap.AddDynamic(this, &USOVShortcutInteractComponent::OnOverlapEnd);
}

void USOVShortcutInteractComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CurrentOverlappingPlayer)
	{
		HideInteractionUI(CurrentOverlappingPlayer);
		CurrentOverlappingPlayer = nullptr;
	}

	if (ProxyActor)
	{
		ProxyActor->Destroy();
		ProxyActor = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void USOVShortcutInteractComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 플레이어가 범위 내에 머무르고 있을 때 실시간으로 인벤토리 변동 감지 및 UI 갱신
	if (CurrentOverlappingPlayer)
	{
		RefreshInteractionUI(CurrentOverlappingPlayer);
	}
}

bool USOVShortcutInteractComponent::IsShortcutAlreadyUnlocked() const
{
	if (const USOVGameInstance* GI = Cast<USOVGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		return GI->IsShortcutOpened(ShortcutTag);
	}
	return false;
}

AActor* USOVShortcutInteractComponent::GetOrCreateInteractableActor()
{
	AActor* OwnerActor = GetOwner();

	// 1. 부모 액터가 이미 IInteraction 인터페이스를 구현하고 있다면 부모 액터를 그대로 사용
	if (OwnerActor && OwnerActor->Implements<UInteraction>())
	{
		return OwnerActor;
	}

	// 2. 부모 액터가 인터페이스를 구현하지 않은 일반 액터(이동 발판 등)라면 자체 프록시 액터를 생성하여 제공
	if (!ProxyActor && GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerActor;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ProxyActor = GetWorld()->SpawnActor<ASOVInteractionProxyActor>(
			ASOVInteractionProxyActor::StaticClass(),
			GetComponentLocation(),
			GetComponentRotation(),
			SpawnParams
		);

		if (ProxyActor)
		{
			ProxyActor->OwnerComponent = this;
			ProxyActor->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			UE_LOG(LogTemp, Log, TEXT("[USOVShortcutInteractComponent] Spawned internal interaction proxy actor for non-interface owner [%s]."),
				OwnerActor ? *OwnerActor->GetName() : TEXT("None"));
		}
	}

	return ProxyActor ? ProxyActor.Get() : OwnerActor;
}

int32 USOVShortcutInteractComponent::FindRequiredItemSlot(AAGSDCharacter* Player, int32& OutQuantity) const
{
	OutQuantity = 0;
	if (!Player || !Player->InventoryComponent)
	{
		return -1;
	}

	const TArray<FStruct_InventorySlotData>& AllSlots = Player->InventoryComponent->GetAllSlots();
	for (int32 i = 0; i < AllSlots.Num(); ++i)
	{
		const FStruct_InventorySlotData& Slot = AllSlots[i];
		if (!Slot.IsEmpty && Slot.ItemData.ItemID.Equals(RequiredItemID, ESearchCase::IgnoreCase))
		{
			OutQuantity += Slot.ItemData.CurrentQuantity;
			return i;
		}
	}

	return -1;
}

int32 USOVShortcutInteractComponent::GetPlayerHoldingItemCount(AAGSDCharacter* Player) const
{
	if (!Player) return 0;

	int32 TotalCount = 0;
	if (Player->InventoryComponent)
	{
		const TArray<FStruct_InventorySlotData>& AllSlots = Player->InventoryComponent->GetAllSlots();
		for (const FStruct_InventorySlotData& Slot : AllSlots)
		{
			if (!Slot.IsEmpty && Slot.ItemData.ItemID.Equals(RequiredItemID, ESearchCase::IgnoreCase))
			{
				TotalCount += Slot.ItemData.CurrentQuantity;
			}
		}
	}

	// 손에 차원 조각(EHS_Shard)을 들고 있는 경우 최소 1개 인정 (인벤토리에 수량이 잡히지 않은 특수 상태 대비)
	if (TotalCount == 0 && bAllowHoldingShard && Player->HoldingState == EHoldingState::EHS_Shard)
	{
		TotalCount = 1;
	}

	return TotalCount;
}

UTexture2D* USOVShortcutInteractComponent::GetRequirementItemIcon(AAGSDCharacter* Player) const
{
	// 1. 에디터에서 직접 오버라이드한 아이콘이 있다면 우선 반환
	if (OverrideItemIcon)
	{
		return OverrideItemIcon;
	}

	// 2. 플레이어 인벤토리의 ItemDataTable에서 RequiredItemID를 검색하여 아이콘 추출
	if (Player && Player->InventoryComponent)
	{
		if (UDataTable* ItemDT = Player->InventoryComponent->GetItemDataTable())
		{
			if (FStruct_ItemData* Row = ItemDT->FindRow<FStruct_ItemData>(FName(*RequiredItemID), TEXT("GetRequirementItemIcon")))
			{
				return Row->ItemIcon;
			}
		}
	}

	return nullptr;
}

bool USOVShortcutInteractComponent::CanUnlockShortcut(AAGSDCharacter* Player) const
{
	if (!Player || IsShortcutAlreadyUnlocked())
	{
		return false;
	}

	return GetPlayerHoldingItemCount(Player) >= RequiredItemAmount;
}

bool USOVShortcutInteractComponent::TryUnlockShortcut(AAGSDCharacter* Player)
{
	if (!Player)
	{
		return false;
	}

	// 숏컷 개방 조건 검사
	if (!CanUnlockShortcut(Player))
	{
		RefreshInteractionUI(Player);
		UE_LOG(LogTemp, Warning, TEXT("[USOVShortcutInteractComponent] TryUnlockShortcut failed: Player has %d / %d of [%s]."),
			GetPlayerHoldingItemCount(Player), RequiredItemAmount, *ShortcutTag.ToString());
		return false;
	}

	// 1. 아이템 차감 처리 (필요 수량만큼 인벤토리 슬롯 순회 차감)
	if (bConsumeItemOnUnlock)
	{
		int32 RemainingToConsume = RequiredItemAmount;

		if (Player->InventoryComponent)
		{
			const TArray<FStruct_InventorySlotData>& AllSlots = Player->InventoryComponent->GetAllSlots();
			for (int32 i = 0; i < AllSlots.Num() && RemainingToConsume > 0; ++i)
			{
				if (!AllSlots[i].IsEmpty && AllSlots[i].ItemData.ItemID.Equals(RequiredItemID, ESearchCase::IgnoreCase))
				{
					const int32 AmountInSlot = AllSlots[i].ItemData.CurrentQuantity;
					const int32 AmountToDeduct = FMath::Min(RemainingToConsume, AmountInSlot);

					Player->InventoryComponent->RemoveItem(i, AmountToDeduct);
					RemainingToConsume -= AmountToDeduct;
				}
			}
			Player->InventoryComponent->SaveToGameInstance();
			UE_LOG(LogTemp, Log, TEXT("[USOVShortcutInteractComponent] Consumed %d of [%s] from inventory."),
				RequiredItemAmount - RemainingToConsume, *RequiredItemID);
		}

		if (RemainingToConsume > 0 && bAllowHoldingShard && Player->HoldingState == EHoldingState::EHS_Shard)
		{
			Player->SubItemAmount();
			UE_LOG(LogTemp, Log, TEXT("[USOVShortcutInteractComponent] Consumed held Shard via SubItemAmount."));
		}
	}

	// 2. GameInstance 및 세이브파일에 숏컷 영구 개방 기록
	if (USOVGameInstance* GI = Cast<USOVGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->SetShortcutState(ShortcutTag, true, bAutoSaveOnUnlock);
	}

	// 3. 상호작용 UI 숨김
	HideInteractionUI(Player);

	// 4. 플레이어 상호작용 대상 목록에서 제거
	if (UAGSDInteractionComponent* InteractionComp = Player->GetInteractionComponent())
	{
		AActor* TargetActor = GetOrCreateInteractableActor();
		InteractionComp->RemoveInteractableActor(TargetActor);
	}

	// 5. 이벤트 디스패처 브로드캐스트 (블루프린트에서 타임라인 Play 등에 바로 연결 가능)
	OnShortcutUnlocked.Broadcast(Player);

	// 6. 부모 액터에 활성화 함수(ActivatePlatform, DoorOpen 등)가 있다면 자동 호출
	if (bAutoTriggerDoorOpen)
	{
		InvokeParentDoorOpen();
	}

	// 7. 스피어 콜리전 및 컴포넌트 완전 제거
	DisableAndDestroySphereCollision();

	UE_LOG(LogTemp, Log, TEXT("[USOVShortcutInteractComponent] Shortcut [%s] successfully unlocked!"), *ShortcutTag.ToString());
	return true;
}

void USOVShortcutInteractComponent::RefreshInteractionUI(AAGSDCharacter* Player)
{
	if (!Player) return;

	AAGSDPlayerController* PC = Cast<AAGSDPlayerController>(Player->GetController());
	if (!PC) return;

	if (IsShortcutAlreadyUnlocked())
	{
		HideInteractionUI(Player);
		return;
	}

	const int32 CurrentAmount = GetPlayerHoldingItemCount(Player);
	const bool bCanUnlockNow = (CurrentAmount >= RequiredItemAmount);

	// 1. 액터 머리 위 위젯 컴포넌트(WorldWidgetComponent) 표시 및 수량/아이콘 갱신
	if (WorldWidgetComponent)
	{
		if (!WorldWidgetComponent->GetWidgetClass() && RequirementWidgetClass)
		{
			WorldWidgetComponent->SetWidgetClass(RequirementWidgetClass);
			WorldWidgetComponent->InitWidget();
		}

		WorldWidgetComponent->SetHiddenInGame(false);
		WorldWidgetComponent->SetVisibility(true);

		if (USOVShortcutRequirementWidget* ReqWidget = Cast<USOVShortcutRequirementWidget>(WorldWidgetComponent->GetUserWidgetObject()))
		{
			UTexture2D* IconTexture = GetRequirementItemIcon(Player);
			ReqWidget->UpdateRequirement(IconTexture, CurrentAmount, RequiredItemAmount);
		}
	}

	// 2. 수량이 3개 충족되었을 때(bCanUnlockNow) 화면 하단 ShowInteractionWidget 활성화!
	if (bCanUnlockNow && bShowInteractionWidgetWhenReady)
	{
		PC->ShowInteractionWidget(CanUnlockPromptText);
	}
	else if (!bCanUnlockNow && bShowScreenPromptTextWhenLocked)
	{
		const FText AmountSuffix = FText::Format(FText::FromString(TEXT(" ({0}/{1})")),
			FText::AsNumber(CurrentAmount), FText::AsNumber(RequiredItemAmount));
		const FText FullText = FText::Join(FText::GetEmpty(), NeedCrystalPromptText, AmountSuffix);
		PC->ShowInteractionWidget(FullText);
	}
	else
	{
		PC->HideInteractionWidget();
	}

	// 3. 외곽선 테두리(CustomDepth) 하이라이트 제어
	if (bHighlightOwnerWhenReady)
	{
		SetOwnerMeshHighlight(bCanUnlockNow);
	}
}

void USOVShortcutInteractComponent::HideInteractionUI(AAGSDCharacter* Player)
{
	// 1. 액터 머리 위 위젯 숨김
	if (WorldWidgetComponent)
	{
		WorldWidgetComponent->SetHiddenInGame(true);
		WorldWidgetComponent->SetVisibility(false);
	}

	// 2. 기본 상호작용 위젯 숨김
	if (Player)
	{
		if (AAGSDPlayerController* PC = Cast<AAGSDPlayerController>(Player->GetController()))
		{
			PC->HideInteractionWidget();
		}
	}

	// 3. 외곽선 테두리 끄기
	SetOwnerMeshHighlight(false);
}

void USOVShortcutInteractComponent::SetOwnerMeshHighlight(bool bActive)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	// 부모 액터(및 자식 액터들)의 모든 프리미티브 컴포넌트를 탐색
	TArray<UPrimitiveComponent*> Comps;
	OwnerActor->GetComponents(Comps, true);
	for (UPrimitiveComponent* Comp : Comps)
	{
		if (!Comp || Comp == this) continue;

		// 위젯 컴포넌트나 충돌 볼륨(스피어, 캡슐, 박스 등)은 제외하고 실제 메시 컴포넌트에만 외곽선 적용
		if (Comp->IsA<UWidgetComponent>() || Comp->IsA<UShapeComponent>())
		{
			continue;
		}

		Comp->SetRenderCustomDepth(bActive);
	}
}

void USOVShortcutInteractComponent::DisableAndDestroySphereCollision()
{
	// 0. 외곽선 테두리 끄기
	SetOwnerMeshHighlight(false);

	// 1. 현재 오버랩 중인 플레이어가 있다면 상호작용 장부, 하이라이트, UI를 완벽히 정리
	if (CurrentOverlappingPlayer)
	{
		if (UAGSDInteractionComponent* InteractionComp = CurrentOverlappingPlayer->GetInteractionComponent())
		{
			AActor* TargetActor = GetOrCreateInteractableActor();
			InteractionComp->RemoveInteractableActor(TargetActor);
			InteractionComp->SetHighLight(TargetActor, false);
		}
		HideInteractionUI(CurrentOverlappingPlayer);
		CurrentOverlappingPlayer = nullptr;
	}

	SetComponentTickEnabled(false);

	// 2. 물리 엔진 상의 모든 콜리전 및 씬 쿼리 즉시 차단
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetGenerateOverlapEvents(false);
	SetHiddenInGame(true);
	SetVisibility(false);

	// 3. 프록시 액터 정리
	if (ProxyActor)
	{
		ProxyActor->Destroy();
		ProxyActor = nullptr;
	}

	// 4. 컴포넌트 자체를 액터 및 월드에서 완전히 파괴 (설정된 경우)
	if (WorldWidgetComponent)
	{
		WorldWidgetComponent->SetHiddenInGame(true);
		WorldWidgetComponent->SetVisibility(false);
		if (bDestroyComponentOnUnlock)
		{
			WorldWidgetComponent->DestroyComponent();
		}
	}

	if (bDestroyComponentOnUnlock)
	{
		UE_LOG(LogTemp, Log, TEXT("[USOVShortcutInteractComponent] Destroying sphere collision component for [%s]."), *ShortcutTag.ToString());
		DestroyComponent();
	}
}

void USOVShortcutInteractComponent::HandleShowWidget(ACharacter* InPlayer)
{
	if (AAGSDCharacter* Player = Cast<AAGSDCharacter>(InPlayer))
	{
		RefreshInteractionUI(Player);
	}
}

bool USOVShortcutInteractComponent::HandleCanInteract(AAGSDCharacter* InPlayer)
{
	return CanUnlockShortcut(InPlayer);
}

void USOVShortcutInteractComponent::HandleInteract(AAGSDCharacter* InPlayer)
{
	TryUnlockShortcut(InPlayer);
}

void USOVShortcutInteractComponent::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                                   bool bFromSweep, const FHitResult& SweepResult)
{
	AAGSDCharacter* Player = Cast<AAGSDCharacter>(OtherActor);
	if (!Player) return;

	CurrentOverlappingPlayer = Player;

	// 상호작용 대상 액터(부모 액터 또는 내부 프록시 액터)를 플레이어 인터랙션 컴포넌트에 등록
	AActor* TargetActor = GetOrCreateInteractableActor();
	if (UAGSDInteractionComponent* InteractionComp = Player->GetInteractionComponent())
	{
		InteractionComp->AddInteractableActor(TargetActor);
	}

	// UI 프롬프트 표시
	RefreshInteractionUI(Player);

	// 플레이어가 범위 안에 있는 동안 실시간 수량 갱신을 위해 틱 활성화
	SetComponentTickEnabled(true);

	// 근접 이벤트 브로드캐스트
	OnPlayerProximityChanged.Broadcast(Player, true);
}

void USOVShortcutInteractComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AAGSDCharacter* Player = Cast<AAGSDCharacter>(OtherActor);
	if (!Player) return;

	// 플레이어의 InteractionComponent에서 대상 액터 제거
	AActor* TargetActor = GetOrCreateInteractableActor();
	if (UAGSDInteractionComponent* InteractionComp = Player->GetInteractionComponent())
	{
		InteractionComp->RemoveInteractableActor(TargetActor);
	}

	// UI 프롬프트 숨김
	HideInteractionUI(Player);

	// 범위에서 벗어나면 틱 비활성화 (성능 최적화)
	SetComponentTickEnabled(false);

	if (CurrentOverlappingPlayer == Player)
	{
		CurrentOverlappingPlayer = nullptr;
	}

	// 근접 이벤트 브로드캐스트
	OnPlayerProximityChanged.Broadcast(Player, false);
}

void USOVShortcutInteractComponent::InvokeParentDoorOpen()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return;

	// AutoInvokeFunctionNames에 등록된 함수명들을 순서대로 탐색하여 존재하는 첫 번째 함수 실행
	for (const FName& FuncName : AutoInvokeFunctionNames)
	{
		UFunction* TargetFunc = OwnerActor->FindFunction(FuncName);
		if (TargetFunc && TargetFunc->NumParms == 0)
		{
			OwnerActor->ProcessEvent(TargetFunc, nullptr);
			UE_LOG(LogTemp, Log, TEXT("[USOVShortcutInteractComponent] Automatically invoked function [%s] on [%s]."),
				*FuncName.ToString(), *OwnerActor->GetName());
			return;
		}
	}
}
