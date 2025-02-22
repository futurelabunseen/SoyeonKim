﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/WKGameState.h"

#include "AbilitySystemComponent.h"
#include "Attribute/WKGameAttributeSet.h"
#include "Game/WKGameMode.h"
#include "WarKing.h"
#include "Kismet/GameplayStatics.h" 
#include "Sound/SoundCue.h" 
#include "Tag/WKGameplayTag.h"

AWKGameState::AWKGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AttributeSet = CreateDefaultSubobject<UWKGameAttributeSet>(TEXT("GameAttributeSet"));

	ASC->RegisterGameplayTagEvent(WKTAG_GAME_CONTROL_DOMINATE_BLUETEAM,
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::DominateTagChanged);
	ASC->RegisterGameplayTagEvent(WKTAG_GAME_CONTROL_DOMINATE_REDTEAM,
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::DominateTagChanged);

	NetUpdateFrequency = 100.f;
}

void AWKGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (ensure(ASC))
	{
		ASC->InitAbilityActorInfo(this, this);
	
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
}

UAbilitySystemComponent* AWKGameState::GetAbilitySystemComponent() const
{
	return ASC;
}

UAttributeSet* AWKGameState::GetAttributeSet() const
{
	return AttributeSet;
}

float AWKGameState::GetBlueTeamScore() const
{
	if (AttributeSet)
	{
		return AttributeSet->GetControlScoreBlue();
	}
	return 0.0f;
}

float AWKGameState::GetRedTeamScore() const
{
	if (AttributeSet)
	{
		return AttributeSet->GetControlScoreRed();
	}
	return 0.0f;
}

void AWKGameState::PlaySound()
{
	if (SoundCue)
	{
		// 이 액터의 위치에서 사운드 재생
		UGameplayStatics::PlaySound2D(this, SoundCue);
	}
}

void AWKGameState::MulticastPlaySound_Implementation()
{
	PlaySound();
}

void AWKGameState::HandleBeginPlay()
{
	WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("Begin"));

	Super::HandleBeginPlay();

	WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("End"));
}

void AWKGameState::OnRep_ReplicatedHasBegunPlay()
{
	WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("Begin"));

	Super::OnRep_ReplicatedHasBegunPlay();

	WK_LOG(LogWKNetwork, Log, TEXT("%s"), TEXT("End"));
}

void AWKGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AWKGameState::DominateTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		MulticastPlaySound();
	}
}
