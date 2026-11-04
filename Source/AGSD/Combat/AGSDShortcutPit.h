// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction.h"
#include "AGSDShortcutPit.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class USceneComponent;
class AAGSDCharacter;
class USOVShortcutInteractComponent;

/**
 * 동굴 숏컷 구덩이 액터 클래스 (SOLID 원칙 준수)
 * - 단일 책임 원칙(SRP):
 *   * 숏컷 해금 조건, 아이템 검사/차감, 머리 위 3D UI, 세이브 저장은 USOVShortcutInteractComponent에 전담.
 *   * 구덩이 외형, 반대편 구덩이 연결(LinkedPit) 및 텔레포트 연출/이동은 AAGSDShortcutPit가 전담.
 * - 개방-폐쇄 원칙(OCP):
 *   * USOVShortcutInteractComponent 및 알림/UI 텍스트, 도착 높이 오프셋을 디테일 패널에서 자유롭게 커스텀 가능.
 * - 리스코프 치환 원칙(LSP):
 *   * IInteraction 인터페이스를 완벽 준수하여 플레이어 인터랙션 시스템과 100% 호환.
 * - 인터페이스 분리 원칙(ISP):
 *   * 필요한 IInteraction 인터페이스 메서드만 정밀 오버라이드.
 * - 의존 역전 원칙(DIP):
 *   * 구체적인 인벤토리 구현 대신 USOVShortcutInteractComponent 및 IInteraction 계약에 의존.
 */
UCLASS(Blueprintable, ClassGroup = (Custom))
class AGSD_API AAGSDShortcutPit : public AActor, public IInteraction
{
	GENERATED_BODY()
	
public:	
	AAGSDShortcutPit();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ── 컴포넌트 ──

	/** 루트 씬 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 구덩이 스태틱 메시 컴포넌트 (기본: LP_Landscape/Hole) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PitMesh;

	/** 플레이어 텔레포트 접근 감지 및 상호작용(E키) 트리거 스피어 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;

	/** 반대편 구덩이에서 이 구덩이로 텔레포트해 왔을 때 플레이어가 서게 될 도착 지점 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> ArrivalPoint;

	/** 숏컷 해금(차원 결정 검사/소모, 머리 위 3D 요구 위젯, 세이브 연동) 전담 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USOVShortcutInteractComponent> ShortcutInteractComp;

	// ── 숏컷 설정 ──

	/** 보스 방 앞 구덩이 여부 (true면 이상한 차원 결정 바치기 가능, false면 입구 전용 구덩이) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Settings")
	bool bIsBossRoomPit = true;

	/** 연결할 반대편 구덩이 액터 레퍼런스 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Shortcut|Settings")
	TObjectPtr<AAGSDShortcutPit> LinkedPit;

	/** 텔레포트 카메라 암전/페이드 지속 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Teleport")
	float TeleportFadeDuration = 0.5f;

	/** 텔레포트 착지 시 바닥 콜리전 겹침을 방지하기 위한 높이 여유 오프셋 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|Teleport")
	float ArrivalZOffset = 30.0f;

	// ── UI 텍스트 설정 ──

	/** 해금 완료 후 텔레포트 상호작용 UI 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	FText TeleportPromptText = FText::FromString(TEXT("구덩이 들어가기"));

	/** 해금 전 입구 구덩이 접근 시 띄울 중앙 경고 알림 텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcut|UI")
	FText NotUnlockedNoticeText = FText::FromString(TEXT("아직 해금되지 않은 구덩이입니다."));

	// ── 내부 상태 ──

	/** 현재 숏컷이 열려있는지 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shortcut|State")
	bool bIsUnlocked = false;

	/** 현재 텔레포트 시퀀스가 진행 중인지 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shortcut|State")
	bool bIsTeleporting = false;

	/** 입구 구덩이 경고창 1회 출력 플래그 */
	bool bHasShownNotice = false;

public:
	// ── IInteraction 인터페이스 구현 ──
	virtual void ShowWidget_Implementation(ACharacter* player) override;
	virtual bool CanInteract_Implementation(AAGSDCharacter* player) override;
	virtual void Interact_Implementation(AAGSDCharacter* player) override;
	virtual FString GetInteractionActionType_Implementation(AAGSDCharacter* player) override { return TEXT("ShortcutPit"); }

	// ── 외부 호출 및 헬퍼 함수 ──

	/** 숏컷 해금 상태 설정 */
	UFUNCTION(BlueprintCallable, Category = "Shortcut")
	void SetShortcutUnlocked(bool bUnlocked);

	/** 숏컷 해금 여부 반환 */
	UFUNCTION(BlueprintPure, Category = "Shortcut")
	bool IsShortcutUnlocked() const { return bIsUnlocked; }

	/** 도착 지점 월드 위치 반환 */
	UFUNCTION(BlueprintPure, Category = "Shortcut")
	FVector GetArrivalLocation() const;

	/** 도착 지점 월드 회전 반환 */
	UFUNCTION(BlueprintPure, Category = "Shortcut")
	FRotator GetArrivalRotation() const;

	/** 플레이어가 소지한 요구 아이템 총 수량 반환 (ShortcutInteractComp에 위임) */
	UFUNCTION(BlueprintPure, Category = "Shortcut")
	int32 GetPlayerItemCount(AAGSDCharacter* Player) const;

	/** 숏컷 전담 컴포넌트 반환 */
	UFUNCTION(BlueprintPure, Category = "Shortcut")
	USOVShortcutInteractComponent* GetShortcutInteractComponent() const { return ShortcutInteractComp; }

protected:
	/** 오버랩 시작 시 플레이어 감지 및 알림/인터랙션 처리 */
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 오버랩 종료 시 플레이어 인터랙션 컴포넌트에서 해제 및 알림 리셋 */
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** ShortcutInteractComp의 해금 성공 콜백 */
	UFUNCTION()
	void OnCompShortcutUnlocked(AAGSDCharacter* UnlockingPlayer);

	/** ShortcutInteractComp의 기해금 상태 콜백 */
	UFUNCTION()
	void OnCompShortcutAlreadyUnlocked(bool bWasOpened);

	/** GameInstance의 숏컷 상태 변경 델리게이트 수신 콜백 */
	UFUNCTION()
	void OnShortcutStateChangedCallback(FName ChangedTag, bool bOpened);

	/** 텔레포트 시퀀스 시작 (카메라 페이드 아웃) */
	void StartTeleport(AAGSDCharacter* Player);

	/** 텔레포트 실제 위치 이동 및 페이드 인 */
	void PerformTeleport();

	// ── 블루프린트 연출 이벤트 훅 ──

	/** 숏컷이 활성화(해금)되었을 때 블루프린트에서 VFX/사운드를 재생할 수 있는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Shortcut|Events")
	void OnShortcutUnlocked_BP();

	/** 텔레포트가 시작될 때 호출되는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Shortcut|Events")
	void OnTeleportStarted_BP(AAGSDCharacter* Player);

	/** 텔레포트가 완료되었을 때 호출되는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Shortcut|Events")
	void OnTeleportFinished_BP(AAGSDCharacter* Player);

private:
	/** 텔레포트 진행 중 플레이어 약참조 */
	TWeakObjectPtr<AAGSDCharacter> TeleportingPlayer;

	/** 텔레포트 페이드 지연 타이머 핸들 */
	FTimerHandle TeleportTimerHandle;
};
