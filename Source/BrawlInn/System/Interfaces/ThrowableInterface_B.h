// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ThrowableInterface_B.generated.h"

// This class does not need to be modified.

class APlayerCharacter_B;

UINTERFACE(MinimalAPI)
class UThrowableInterface_B : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class BRAWLINN_API IThrowableInterface_B
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Broadcast")
	void PickedUp(APlayerCharacter_B* Player);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Broadcast")
	void Dropped();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Broadcast")
	bool IsHeld() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Broadcast")
	void Use();
};	