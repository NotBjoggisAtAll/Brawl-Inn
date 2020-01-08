// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HealthWidget_B.generated.h"

/**
 * 
 */
UCLASS()
class BRAWLINN_API UHealthWidget_B : public UUserWidget
{
	GENERATED_BODY()
protected:

		virtual bool Initialize() override;
	
};