#include "SurvivalDiagnostics.h"
#include "Animation/AnimInstance.h"
#include "UObject/UnrealType.h"

TSharedRef<FJsonObject> RuntimeDiagnostics::ObserveSurvival(UAnimInstance* Instance)
{
    auto Result = MakeShared<FJsonObject>();
    const bool Matches = Instance && Instance->GetClass()->GetName().StartsWith(TEXT("ABP_Survival_MotionMatching"));
    Result->SetBoolField(TEXT("adapter_matches"), Matches);
    if (!Matches) return Result;
    // Preserve actual property names and values, including maps, enums, and sequence object paths.
    for (TFieldIterator<FProperty> It(Instance->GetClass()); It; ++It)
    {
        FProperty* P = *It;
        const FString Name = P->GetName();
        if (!Name.StartsWith(TEXT("JC_")) && Name != TEXT("VerticalVelocity") &&
            Name != TEXT("isInAir") && Name != TEXT("isCrouched")) continue;
        const void* Value = P->ContainerPtrToValuePtr<void>(Instance);
        if (auto* B = CastField<FBoolProperty>(P)) Result->SetBoolField(Name, B->GetPropertyValue(Value));
        else if (auto* N = CastField<FNumericProperty>(P))
            Result->SetNumberField(Name, N->IsFloatingPoint() ? N->GetFloatingPointPropertyValue(Value) : double(N->GetSignedIntPropertyValue(Value)));
        else if (auto* O = CastField<FObjectPropertyBase>(P))
        {
            UObject* Object = O->GetObjectPropertyValue(Value);
            if (Object) Result->SetStringField(Name, Object->GetPathName());
            else Result->SetField(Name, MakeShared<FJsonValueNull>());
        }
        else
        {
            FString Text;
            P->ExportTextItem_Direct(Text, Value, nullptr, Instance, PPF_None);
            Result->SetStringField(Name, Text);
        }
    }
    const int32 Machine = Instance->GetStateMachineIndex(TEXT("SM_Airborne"));
    if (Machine != INDEX_NONE) Result->SetStringField(TEXT("airborne_state"), Instance->GetCurrentStateName(Machine).ToString());
    else Result->SetField(TEXT("airborne_state"), MakeShared<FJsonValueNull>());
    return Result;
}
