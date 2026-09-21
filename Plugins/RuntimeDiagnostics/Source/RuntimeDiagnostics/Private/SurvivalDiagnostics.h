#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UAnimInstance;
namespace RuntimeDiagnostics
{
    // Optional reflection adapter. No dependency on game modules or assets.
    TSharedRef<FJsonObject> ObserveSurvival(UAnimInstance* Instance);
}
