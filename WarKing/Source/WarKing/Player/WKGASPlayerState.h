// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "WKGASPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class WARKING_API AWKGASPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AWKGASPlayerState();

	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	class UWKCharacterAttributeSet* GetAttributeSet() const;
	class UWKCharacterSkillAttributeSet* GetSkillAttributeSet() const;

protected:

	UPROPERTY(EditAnywhere, Category = GAS)
	TObjectPtr<class UAbilitySystemComponent> ASC;

	UPROPERTY()
	TObjectPtr<class UWKCharacterAttributeSet> AttributeSet;

	UPROPERTY()
	TObjectPtr<class UWKCharacterSkillAttributeSet> SkillAttributeSet;

	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//virtual void HealthChanged(const FOnAttributeChangeData& Data);
	//virtual void MaxHealthChanged(const FOnAttributeChangeData& Data);
};
