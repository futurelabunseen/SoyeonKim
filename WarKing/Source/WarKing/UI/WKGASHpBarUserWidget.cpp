// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WKGASHpBarUserWidget.h"
#include "AbilitySystemComponent.h"
#include "Attribute/WKCharacterAttributeSet.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Tag/WKGameplayTag.h"
#include "WarKing.h"

void UWKGASHpBarUserWidget::SetAbilitySystemComponent(AActor* InOwner)
{
	Super::SetAbilitySystemComponent(InOwner);
	
	if (ASC)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UWKCharacterAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ThisClass::OnHealthChanged);
		ASC->GetGameplayAttributeValueChangeDelegate(UWKCharacterAttributeSet::GetMaxHealthAttribute())
			.AddUObject(this, &ThisClass::OnMaxHealthChanged);

		// RegisterGameplayTagEvent : 해당 태그가 부착, 떨어질때 지정한 함수 호출 델리게이트
		//ASC->RegisterGameplayTagEvent(WKTAG_CHARACTER_STATE_INVISIBLE, EGameplayTagEventType::NewOrRemoved)
		//	.AddUObject(this, &ThisClass::OnInvinsibleTagChanged);

		//PbHpBar->SetFillColorAndOpacity(FLinearColor::Red);

		const UWKCharacterAttributeSet* CurrentAttributeSet = ASC->GetSet<UWKCharacterAttributeSet>();
		if (CurrentAttributeSet)
		{
			CurrentHealth = CurrentAttributeSet->GetHealth();
			CurrentMaxHealth = CurrentAttributeSet->GetMaxHealth();

			if (CurrentMaxHealth > 0.0f)
			{
				UpdateHpBar();
			}
		}
	}
	else
	{
		UE_LOG(LogWKNetwork, Log, TEXT("[%s][%s]NO ASC"), InOwner->HasAuthority()? TEXT("SERVER") : TEXT("CLIENT"), *InOwner->GetName());
	}

}
void UWKGASHpBarUserWidget::OnHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	CurrentHealth = ChangeData.NewValue;
	UpdateHpBar();
}

void UWKGASHpBarUserWidget::OnMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	CurrentMaxHealth = ChangeData.NewValue;
	UpdateHpBar();
}

void UWKGASHpBarUserWidget::UpdateHpBar()
{
	if(ASC)
	{
		UE_LOG(LogWKNetwork, Log, TEXT("[%s][%s]UpdateHpBar"), ASC->GetOwnerActor()->HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"), *ASC->GetOwnerActor()->GetName());
	}	
	else
	{
		UE_LOG(LogWKNetwork, Log, TEXT("[No ASC][%s]UpdateHpBar"), *ASC->GetOwnerActor()->GetName());
	}
		
	if (PbHpBar)
	{
		PbHpBar->SetPercent(CurrentHealth / CurrentMaxHealth);
	}

	if (TxtHpStat)
	{
		TxtHpStat->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f/%.0f"), CurrentHealth, CurrentMaxHealth)));
	}
}

void UWKGASHpBarUserWidget::SetWidgetTeamColor(const FGameplayTag Team)
{
	if (NickNameText && PbHpBar)
	{
		if (Team == WKTAG_GAME_TEAM_RED)
		{
			NickNameText->SetColorAndOpacity(RedTeamColor);
			PbHpBar->SetFillColorAndOpacity(RedTeamColor);
		}
		else if (Team == WKTAG_GAME_TEAM_BLUE)
		{
			NickNameText->SetColorAndOpacity(BlueTeamColor);
			PbHpBar->SetFillColorAndOpacity(BlueTeamColor);
		}
	}
}

void UWKGASHpBarUserWidget::SetNickName(const FString NameText)
{
	if (NickNameText)
	{
		NickNameText->SetText(FText::FromString(NameText));
	}
}
