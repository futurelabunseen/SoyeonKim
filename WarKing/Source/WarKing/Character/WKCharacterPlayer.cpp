// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/WKCharacterPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "WarKing.h"
#include "AbilitySystemComponent.h"
#include "UI/WKGASWidgetComponent.h"
#include "UI/WKGASHpBarUserWidget.h"
#include "Game/WKGameState.h"
#include "Game/WKGameMode.h"
#include "Tag/WKGameplayTag.h"
#include "GameplayTagContainer.h"
#include "Attribute/WKCharacterAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Player/WKPlayerController.h"
#include "Player/WKGASPlayerState.h"
#include "Actor/PlayerStart/WKPlayerStart.h"

AWKCharacterPlayer::AWKCharacterPlayer()
{
	// player가 빙의할 때 playerState에서 생성된 ASC값을 대입할 것임
	ASC = nullptr;

	// Camera
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Input
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> InputMappingContextRef(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/WarKing/Input/IMC_Default.IMC_Default'"));
	if (nullptr != InputMappingContextRef.Object)
	{
		DefaultMappingContext = InputMappingContextRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionMoveRef(TEXT("/Script/EnhancedInput.InputAction'/Game/WarKing/Input/Actions/IA_Move.IA_Move'"));
	if (nullptr != InputActionMoveRef.Object)
	{
		MoveAction = InputActionMoveRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionJumpRef(TEXT("/Script/EnhancedInput.InputAction'/Game/WarKing/Input/Actions/IA_Jump.IA_Jump'"));
	if (nullptr != InputActionJumpRef.Object)
	{
		JumpAction = InputActionJumpRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionLookRef(TEXT("/Script/EnhancedInput.InputAction'/Game/WarKing/Input/Actions/IA_Look.IA_Look'"));
	if (nullptr != InputActionLookRef.Object)
	{
		LookAction = InputActionLookRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionAttackRef(TEXT("/Script/EnhancedInput.InputAction'/Game/WarKing/Input/Actions/IA_Attack.IA_Attack'"));
	if (nullptr != InputActionAttackRef.Object)
	{
		AttackAction = InputActionAttackRef.Object;
	}
}

void AWKCharacterPlayer::BeginPlay()
{
	Super::BeginPlay();

	WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("BeginPlay"));
}

UAbilitySystemComponent* AWKCharacterPlayer::GetAbilitySystemComponent() const
{
	return ASC;
}

void AWKCharacterPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("Begin"));
	
	WKPlayerState = GetPlayerState<AWKGASPlayerState>();

	InitGASSetting();
	SetTeamColor(GetTeam());

	ASC->RegisterGameplayTagEvent(WKTAG_CHARACTER_STATE_DEBUFF_STUN,
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::StunTagChanged);

	if (WKPlayerState && !WKPlayerState->GetInitializedValue())
	{
		SetGASGiveAbility();
		SetHUD();
		SetInitEffects(); 
		ConsoleCommandSetting();

		WKPlayerState->SetInitializedValue(true);
	}
	isDead = false;
	SetNickNameWidget();
	AWKPlayerController* PlayerController = Cast<AWKPlayerController>(GetController());
	if (PlayerController)
	{
		PlayerController->OnRespawnState(false);
	}
	RespawnMontagePlay();
	WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("End"));
}

void AWKCharacterPlayer::OnRep_Owner()
{
	WK_LOG(LogWKNetwork, Log, TEXT("%s %s"), *GetName(), TEXT("Begin"));

	Super::OnRep_Owner();

	ConsoleCommandSetting();

	WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("End"));
}

void AWKCharacterPlayer::OnRep_PlayerState()
{
	// 클라 전용 
	Super::OnRep_PlayerState();
	WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("OnRep_PlayerState"));
	WKPlayerState = GetPlayerState<AWKGASPlayerState>();
	InitGASSetting();
	SetTeamColor(GetTeam());
	isDead = false;

	if (WKPlayerState && !WKPlayerState->GetInitializedValue())
	{
		SetHUD();

		WKPlayerState->SetInitializedValue(true);
	}
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			SetNickNameWidget();
		});

	AWKPlayerController* PlayerController = Cast<AWKPlayerController>(GetController());
	if (PlayerController)
	{
		PlayerController->OnRespawnState(false);
	}
	RespawnMontagePlay();
}

void AWKCharacterPlayer::OnRep_Controller()
{
	WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("OnRep_Controller"));

	// OnRep_PlayerState()에서 놓칠 수 있어, 방어코드 작성
	if (WKPlayerState)
	{
		WKPlayerState->GetAbilitySystemComponent()->RefreshAbilityActorInfo();
	}
}

void AWKCharacterPlayer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InitializeInput();
}

void AWKCharacterPlayer::SetupGASInputComponent()
{
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

	if (IsValid(EnhancedInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::GASInputPressed, 0);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::GASInputReleased, 0);

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ThisClass::GASInputPressed, 1);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::GASInputReleased, 1);

		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &ThisClass::GASInputPressed, WKTAG_CHARACTER_ACTION_ATTACK);

		// Blade Skill Q BlockAttack
		EnhancedInputComponent->BindAction(SkillAction1, ETriggerEvent::Triggered, this, &ThisClass::GASInputPressed, WKTAG_CHARACTER_ACTION_SKILL_BLOCKATTACK);

		// Blade Skill E FlamingSword
		EnhancedInputComponent->BindAction(SkillAction2, ETriggerEvent::Triggered, this, &ThisClass::GASInputPressed, WKTAG_CHARACTER_ACTION_SKILL_FLAMINGSWORD);

		// Blade Skill R AOE
		EnhancedInputComponent->BindAction(SkillActionUlt, ETriggerEvent::Triggered, this, &ThisClass::GASInputPressed, WKTAG_CHARACTER_ACTION_SKILL_AOE);
	}	
}

void AWKCharacterPlayer::InitializeInput()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerController->InputComponent);

		if (EnhancedInputComponent)
		{
			InputSubsystem->ClearAllMappings();
			InputSubsystem->AddMappingContext(DefaultMappingContext, 0);

			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
			
			SetupGASInputComponent();
		}
	}
}

void AWKCharacterPlayer::GASInputPressed(int32 InputId)
{
	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromInputID(InputId);

	if (Spec)
	{
		Spec->InputPressed = true;
		if (Spec->IsActive())
		{
			ASC->AbilitySpecInputPressed(*Spec);
		}
		else
		{
			ASC->TryActivateAbility(Spec->Handle);
		}
	}
}

void AWKCharacterPlayer::GASInputPressed(const FGameplayTag InputTag)
{
	if (ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_DEBUFF_STUN) || 
			ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_ISDEAD))
		return;

	FGameplayTagContainer Container;
	Container.AddTag(InputTag);
	ASC->TryActivateAbilitiesByTag(Container);
}

void AWKCharacterPlayer::GASInputReleased(int32 InputId)
{
	if (ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_DEBUFF_STUN) ||
		ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_ISDEAD))
		return;

	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromInputID(InputId);

	if (Spec)
	{
		Spec->InputPressed = false;
		if (Spec->IsActive())
		{
			ASC->AbilitySpecInputReleased(*Spec);
		}
	}
}

void AWKCharacterPlayer::InitGASSetting()
{
	WKPlayerState = GetPlayerState<AWKGASPlayerState>();
	WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("GASAbilitySetting"));

	if (WKPlayerState)
	{
		ASC = WKPlayerState->GetAbilitySystemComponent();

		ASC->InitAbilityActorInfo(WKPlayerState, this);	

		// 서버 캐릭터만 Remove...
		if (ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_ISDEAD))
		{
			ASC->RemoveLooseGameplayTag(WKTAG_CHARACTER_STATE_ISDEAD);
		}
		WKPlayerState->OnOutOfHealth.AddDynamic(this, &ThisClass::OnOutOfHealth);
	}

	// Widget 초기화 작업
	HpBar->InitGASWidget();
}

void AWKCharacterPlayer::SetGASGiveAbility()
{	
	for (const auto& StartAbility : StartAbilities)
	{
		FGameplayAbilitySpec StartSpec(StartAbility);
		ASC->GiveAbility(StartSpec);
	}

	for (const auto& StartInputAbility : StartInputAbilities)
	{
		FGameplayAbilitySpec StartSpec(StartInputAbility.Value);
		StartSpec.InputID = StartInputAbility.Key;
		ASC->GiveAbility(StartSpec);
	}
}

void AWKCharacterPlayer::SetHUD()
{		
	// HUD Set
	UWKCharacterAttributeSet* CurrentAttributeSet = WKPlayerState->GetAttributeSet();
	AWKGameState* CurrentGameState = Cast<AWKGameState>(GetWorld()->GetGameState());

	if (AWKPlayerController* WKPlayerController = Cast<AWKPlayerController>(GetController()))
	{
		if (CurrentAttributeSet && CurrentGameState)
		{
			WKPlayerController->InitOverlay(ASC, CurrentAttributeSet,
				CurrentGameState->GetAbilitySystemComponent(), CurrentGameState->GetAttributeSet());
		}
	}
}

void AWKCharacterPlayer::SetNickNameWidget()
{
	if (WKPlayerState)
	{
		UWKGASHpBarUserWidget* WKHpBar = Cast<UWKGASHpBarUserWidget>(HpBar->GetWidget());

		if (WKHpBar)
		{
			WKHpBar->SetNickName(WKPlayerState->GetPlayerName());
			WKHpBar->SetWidgetTeamColor(GetTeam());
		}
	}
}

void AWKCharacterPlayer::ConsoleCommandSetting()
{
	//APlayerController* PlayerController = CastChecked<APlayerController>(GetOwner());

	//if (ensure(PlayerController))
	//{
	//	PlayerController->ConsoleCommand(TEXT("showdebug abilitysystem"));
	//}
}

void AWKCharacterPlayer::SetInitEffects()
{		
	// Init Effect Setting
	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (TSubclassOf<UGameplayEffect> GameplayEffect : StartEffects)
	{
		FGameplayEffectSpecHandle NewHandle = ASC->MakeOutgoingSpec(GameplayEffect, 1.f, EffectContext);
		if (NewHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveGEHandle =
				ASC->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), ASC.Get());
		}
	}
}

void AWKCharacterPlayer::SetPlayerDefaults()
{
	if (IsValid(ASC))
	{
		SetSpawnPoint();
		WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("SetPlayerDefaults"));
		UWKCharacterAttributeSet* CurrentAttributeSet = WKPlayerState->GetAttributeSet();
		CurrentAttributeSet->ResetAttributeSetData();
	}
}

bool AWKCharacterPlayer::GetIsFalling()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		return Movement->IsFalling();
	}

	return false;
}

void AWKCharacterPlayer::ClientSetStopSprint_Implementation()
{
	Sprint(false);

	if (ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_ISSPRINTING))
	{
		ASC->RemoveLooseGameplayTag(WKTAG_CHARACTER_STATE_ISSPRINTING);
	}
}

void AWKCharacterPlayer::ServerSetStopSprint()
{
	Sprint(false);

	if (!IsLocallyControlled())
	{
		ClientSetStopSprint();
	}
	
	if (ASC && SprintCostEffect)
	{
		ASC->RemoveActiveGameplayEffectBySourceEffect(SprintCostEffect, ASC);
	
		FGameplayTagContainer AbilityTagsToCancel;
		AbilityTagsToCancel.AddTag(WKTAG_CHARACTER_STATE_ISSPRINTING);
		FGameplayTagContainer AbilityTagsToIgnore;

		ASC->CancelAbilities(&AbilityTagsToCancel, &AbilityTagsToIgnore);
		if (ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_ISSPRINTING))
		{
			ASC->RemoveLooseGameplayTag(WKTAG_CHARACTER_STATE_ISSPRINTING);
		}
	}
}

void AWKCharacterPlayer::ServerLeaveGame_Implementation()
{
}

bool AWKCharacterPlayer::HasGameplayTag(FGameplayTag Tag) const
{
	if (IsValid(ASC))
		return ASC->HasMatchingGameplayTag(Tag);
	else
		return false;
}

void AWKCharacterPlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.X);
	AddMovementInput(RightDirection, MovementVector.Y);
}

void AWKCharacterPlayer::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AWKCharacterPlayer::Sprint(bool IsSprint)
{
	if (IsSprint)
	{
		GetCharacterMovement()->MaxWalkSpeed = 800.f;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = 400.0;
	}
}

bool AWKCharacterPlayer::GetIsMoving()
{
	return GetVelocity().Size() > 0.f && GetCharacterMovement()->IsMovingOnGround();
}

void AWKCharacterPlayer::SetSpawnPoint()
{
	if (HasAuthority() && WKPlayerState && WKPlayerState->GetTeam() != WKTAG_GAME_TEAM_NONE)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, AWKPlayerStart::StaticClass(), PlayerStarts);
		TArray<AWKPlayerStart*> TeamPlayerStarts;

		for (auto Start : PlayerStarts)
		{
			AWKPlayerStart* TeamStart = Cast<AWKPlayerStart>(Start);
			if (TeamStart && TeamStart->TeamTag == WKPlayerState->GetTeam())
			{
				TeamPlayerStarts.Add(TeamStart);
			}
		}

		if (TeamPlayerStarts.Num() > 0)
		{
			AWKPlayerStart* ChosenPlayerStart = TeamPlayerStarts[FMath::RandRange(0, TeamPlayerStarts.Num() - 1)];
	
			FRotator Rotator = ChosenPlayerStart->GetActorRotation();
			Rotator.Roll = 0;
			SetActorLocationAndRotation(
				ChosenPlayerStart->GetActorLocation(),
				Rotator);

			WK_LOG(LogWKNetwork, Log, TEXT("SetSpawnPoint : %f, %f, %f"), Rotator.Roll, Rotator.Pitch, Rotator.Yaw);
		}
	}
}

void AWKCharacterPlayer::OnOutOfHealth()
{
	SetDead();
}

void AWKCharacterPlayer::SetDead()
{
	Super::SetDead();

	AWKPlayerController* PlayerController = Cast<AWKPlayerController>(GetController());
	if (PlayerController)
	{
		DisableInput(PlayerController);
		PlayerController->OnRespawnState(true);
	}

	RemoveAttackTag();

	CancelAbilities();

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&AWKCharacterPlayer::ElimTimerFinished,
		ElimDelay
	);
}

void AWKCharacterPlayer::StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bool bIsCheckStun = NewCount > 0;

	if (!ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_ISDEAD))
	{
		// Muticast RPC 호출
		MulticastSetStun(bIsCheckStun);
		WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("StunTagChanged"));
	}
}

void AWKCharacterPlayer::CancelAbilities()
{
	FGameplayTagContainer AbilityTagsToCancel;
	AbilityTagsToCancel.AddTag(FGameplayTag::RequestGameplayTag(FName("Character.Action")));
	AbilityTagsToCancel.AddTag(FGameplayTag::RequestGameplayTag(FName("Character.State")));

	FGameplayTagContainer AbilityTagsToIgnore;
	AbilityTagsToIgnore.AddTag(FGameplayTag::RequestGameplayTag(FName("Character.State.IsDead")));

	ASC->CancelAbilities(&AbilityTagsToCancel, &AbilityTagsToIgnore);

	if (ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_ACTION_SKILL_FLAMINGSWORD))
	{
		ASC->RemoveLooseGameplayTag(WKTAG_CHARACTER_ACTION_SKILL_FLAMINGSWORD);
		ASC->RemoveReplicatedLooseGameplayTag(WKTAG_CHARACTER_ACTION_SKILL_FLAMINGSWORD);
	}
}

void AWKCharacterPlayer::RemoveAttackTag()
{
	if (!ASC) return;

	if (CancelTags.IsEmpty()) return;

	for (const FGameplayTag& CancelTag : CancelTags)
	{
		if (ASC->HasMatchingGameplayTag(CancelTag))
		{
			ASC->RemoveLooseGameplayTag(CancelTag);
			ASC->RemoveReplicatedLooseGameplayTag(CancelTag);
		}
	}
}

void AWKCharacterPlayer::MulticastSetStun_Implementation(bool bIsStun)
{
	if (ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_ISDEAD))
		return;

	// Server 수행
	SetStun(bIsStun);

	if (bIsStun)
	{
		CancelAbilities();
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
		if (!ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_DEBUFF_STUN))
		{
			if (!ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_DEBUFF_STUNCOOLDOWN))
			{
				ASC->AddLooseGameplayTag(WKTAG_CHARACTER_STATE_DEBUFF_STUNCOOLDOWN);
			}
		}
	}
	else
	{
		GetWorldTimerManager().SetTimer(StunTimer, FTimerDelegate::CreateLambda(
			[this]()->void
			{
				if (!isDead)
				{
					if (!ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_DEBUFF_STUN))
					{
						// Daad상태 추가
						GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
					}

					if (ASC->HasMatchingGameplayTag(WKTAG_CHARACTER_STATE_DEBUFF_STUNCOOLDOWN))
					{
						ASC->RemoveLooseGameplayTag(WKTAG_CHARACTER_STATE_DEBUFF_STUNCOOLDOWN);
					}
				}
			}
		), StunCooldownTime, false);
	}
}

void AWKCharacterPlayer::RespawnMontagePlay()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->StopAllMontages(0.0f);
	AnimInstance->Montage_Play(RespawnMontage, 1.0f);
}

void AWKCharacterPlayer::ElimTimerFinished()
{
	AWKPlayerController* PlayerController = Cast<AWKPlayerController>(GetController());

	AWKGameMode* WKGameMode = GetWorld()->GetAuthGameMode<AWKGameMode>();


	if (WKGameMode)
	{
		WKGameMode->RequestRespawn(this, Controller);
	}
}
