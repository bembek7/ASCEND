// Fill out your copyright notice in the Description page of Project Settings.


#include "ASCEND/Components/AbilityManager.h"
#include "ASCEND/Actors/Characters/PlayerCharacter.h"
#include <Kismet/GameplayStatics.h>

UAbilityManager::UAbilityManager()
{
	PrimaryComponentTick.bCanEverTick = true;
}

int32 UAbilityManager::IndexByName(TArray<AAbilityBase*>& CorrespondingArray, AAbilityBase* Ability)
{
	int32 i = 0;
	for (AAbilityBase* AbilityInArray : CorrespondingArray)
	{
		if (AbilityInArray->AbilityName == Ability->AbilityName)
		{
			return i;
		}
		i++;
	}
	return INDEX_NONE;
}

int32 UAbilityManager::GetAbilityIndexInArray(AAbilityBase* Ability)
{
	return IndexByName(FindCorrespondingArray(Ability), Ability);
	//return FindCorrespondingArray(Ability).IndexOfByKey(Ability);
}

void UAbilityManager::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<APlayerCharacter>(GetOwner());
}

TArray<AAbilityBase*>& UAbilityManager::FindCorrespondingArray(AAbilityBase* Ability)
{
	if(Ability->Hand == EAbilityHand::Left)
	{
		return LeftHandAbilitiesArray;
	}
	if(Ability->Hand == EAbilityHand::Right)
	{
		return RightHandAbilitiesArray;
	}
	UE_LOG(LogTemp, Error, TEXT("Invalid ability hand"))
	return LeftHandAbilitiesArray;
}

void UAbilityManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!LeftHandAbilitiesArray.IsEmpty())
	{
		for(AAbilityBase* Ability : LeftHandAbilitiesArray)
		{
			FString Name = Ability->AbilityName.ToString();
			int32 Index = LeftHandAbilitiesArray.Find(Ability);
			bool IsActive = Ability->bIsPicked;
			FString Active;
			if(IsActive)
			{
				Active = "Active";
			} else
			{
				Active = "Not active";
			}
			UE_LOG(LogTemp, Warning, TEXT("%s %s %s"), *Name, *FString::FromInt(Index), *Active);
		}
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("Array is empty"));
	}
}

void UAbilityManager::ActivatePickedLeftHandAbility()
{
	if(LeftHandAbilitiesArray.IsEmpty())
	{
		return;
	}
	
	for(AAbilityBase* Ability : LeftHandAbilitiesArray)
	{
		if(Ability->bIsPicked && Ability->CanActivateAbility())
		{
			ActivateAbility(Ability);
			return;
		}
	}
}

void UAbilityManager::ActivatePickedRightHandAbility()
{
	if(RightHandAbilitiesArray.IsEmpty())
	{
		return;
	}
	
	for(AAbilityBase* Ability : RightHandAbilitiesArray)
	{
		if(Ability->bIsPicked && Ability->CanActivateAbility())
		{
			ActivateAbility(Ability);
			return;
		}
	}
}

void UAbilityManager::ActivateAbility(AAbilityBase* Ability)
{
	Ability->UseAbility();
	
	if (Ability->Hand == EAbilityHand::Left)
	{
		if(LeftAbilityUsed.IsBound())
		{
			LeftAbilityUsed.Broadcast(Ability);
		}	
	} else if(Ability->Hand == EAbilityHand::Right)
	{
		if(RightAbilityUsed.IsBound())
		{
			RightAbilityUsed.Broadcast(Ability);
		}	
	}
	
	if(Ability->ChargesLeft == 0)
	{
		TArray<AAbilityBase*>& CorrespondingArray = FindCorrespondingArray(Ability);
		GetNextAbilityInArray(Ability)->bIsPicked = true;
		CorrespondingArray.Remove(Ability);
		if (Ability->Hand == EAbilityHand::Left)
		{
			if (LeftAbilityRemovedDelegate.IsBound())
			{
				LeftAbilityRemovedDelegate.Broadcast(Ability);
			}
		}
		else if (Ability->Hand == EAbilityHand::Right)
		{
			if (RightAbilityRemovedDelegate.IsBound())
			{
				RightAbilityRemovedDelegate.Broadcast(Ability);
			}
		}
		Ability->Destroy();
	}
}

void UAbilityManager::PickAbility(AAbilityBase* Ability)
{
	if(!Ability)
	{
		return;
	}
	if (Ability->Hand == EAbilityHand::Left)
	{
		if (LeftHandAbilityDelegate.IsBound())
		{
			LeftHandAbilityDelegate.Broadcast(Ability);
		}
	}
	else if (Ability->Hand == EAbilityHand::Right)
	{
		if (RightHandAbilityDelegate.IsBound())
		{
			RightHandAbilityDelegate.Broadcast(Ability);
		}
	}
	if (!Ability->bPickIt)
	{
		return;
	}
	Ability->bActive = false;
	TArray<AAbilityBase*>& CorrespondingArray = FindCorrespondingArray(Ability);
	for (AAbilityBase* AbilityInArray : CorrespondingArray)
	{
		AbilityInArray->bIsPicked = false;
	}
	Ability->bIsPicked = true;
	UE_LOG(LogTemp, Warning, TEXT("True"));
	GEngine->AddOnScreenDebugMessage(0, 1.f, FColor::Cyan, Ability->AbilityName.ToString() + " picked up");
}

void UAbilityManager::CycleThroughLeftHand()
{
	int32 NumberOfItemsInArray = LeftHandAbilitiesArray.Num();
	if(NumberOfItemsInArray <= 1)
	{
		GEngine->AddOnScreenDebugMessage(0, 1.f, FColor::Red, "You have no other abilities");
		return;
	}
	for (AAbilityBase* AbilityInArray : LeftHandAbilitiesArray)
	{
		if (AbilityInArray->bIsPicked)
		{
			AbilityInArray->bIsPicked = false;
			AAbilityBase* NextAbility = GetNextAbilityInArray(AbilityInArray);
			NextAbility->bIsPicked = true;
			GEngine->AddOnScreenDebugMessage(0, 1.f, FColor::Cyan, NextAbility->AbilityName.ToString() + " picked up");
			return;
		}
	}
}

void UAbilityManager::CycleThroughRightHand()
{
	int32 NumberOfItemsInArray = RightHandAbilitiesArray.Num();
	if(NumberOfItemsInArray <= 1)
	{
		GEngine->AddOnScreenDebugMessage(0, 1.f, FColor::Red, "You have no other abilities");
		return;
	}
	for (AAbilityBase* AbilityInArray : RightHandAbilitiesArray)
	{
		if(AbilityInArray->bIsPicked)
		{
			AbilityInArray->bIsPicked = false;
			AAbilityBase* NextAbility = GetNextAbilityInArray(AbilityInArray);
			NextAbility->bIsPicked = true;
			GEngine->AddOnScreenDebugMessage(0, 1.f, FColor::Cyan, NextAbility->AbilityName.ToString() + " picked up");
			return;
		}
	}
}

AAbilityBase* UAbilityManager::GetNextAbilityInArray(AAbilityBase* CurrentAbility)
{
	
	TArray<AAbilityBase*>& CorrespondingArray = FindCorrespondingArray(CurrentAbility);
	int32 NumberOfItemsInArray = CorrespondingArray.Num() - 1;
	int32 CurrentAbilityArrayIndex = CorrespondingArray.Find(CurrentAbility);
	if(CurrentAbilityArrayIndex == NumberOfItemsInArray)
	{
		return CorrespondingArray[0];
	}
	if(CurrentAbilityArrayIndex < NumberOfItemsInArray)
	{
		return CorrespondingArray[CurrentAbilityArrayIndex + 1];
	}
	return nullptr;
}
