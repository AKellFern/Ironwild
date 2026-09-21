#include "RuntimeDiagnosticsSubsystem.h"
#include "SurvivalDiagnostics.h"
#include "Animation/AnimClassInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimNode_StateMachine.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EnhancedPlayerInput.h"
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"

namespace RD
{
using FObject = TSharedRef<FJsonObject>;
FString Json(const FObject& Object)
{
    FString Text;
    auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text);
    FJsonSerializer::Serialize(Object, Writer);
    return Text;
}
UWorld* World(int32 Instance)
{
    if (!GEngine) return nullptr;
    if (Instance < 0 && GEditor && GEditor->PlayWorld) return GEditor->PlayWorld;
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
        if (Context.WorldType == EWorldType::PIE && (Instance < 0 || Context.PIEInstance == Instance)) return Context.World();
    return nullptr;
}
void Vector(const FObject& Object, const FString& Name, const FVector& Value)
{
    auto V = MakeShared<FJsonObject>();
    V->SetNumberField(TEXT("x"), Value.X); V->SetNumberField(TEXT("y"), Value.Y); V->SetNumberField(TEXT("z"), Value.Z);
    Object->SetObjectField(Name, V);
}
FObject Observe(ACharacter* Character)
{
    auto Root = MakeShared<FJsonObject>();
    Root->SetNumberField(TEXT("schema_version"), 1);
    Root->SetNumberField(TEXT("monotonic_seconds"), FPlatformTime::Seconds());
    Root->SetStringField(TEXT("frame"), LexToString(GFrameCounter));
    if (!IsValid(Character) || !Character->GetWorld() || Character->GetWorld()->WorldType != EWorldType::PIE)
    {
        Root->SetBoolField(TEXT("ok"), false);
        Root->SetStringField(TEXT("error"), TEXT("No character in selected PIE world/player index"));
        return Root;
    }
    Root->SetBoolField(TEXT("ok"), true);
    Root->SetStringField(TEXT("character"), Character->GetPathName());
    Root->SetStringField(TEXT("world"), Character->GetWorld()->GetPathName());
    Root->SetNumberField(TEXT("pie_seconds"), Character->GetWorld()->GetTimeSeconds());
    auto Input = MakeShared<FJsonObject>();
    APlayerController* PC = Cast<APlayerController>(Character->GetController());
    Input->SetBoolField(TEXT("available"), PC && PC->IsLocalController());
    const TPair<const TCHAR*, FKey> Keys[] = {
        {TEXT("W"), EKeys::W}, {TEXT("A"), EKeys::A}, {TEXT("S"), EKeys::S}, {TEXT("D"), EKeys::D},
        {TEXT("SpaceBar"), EKeys::SpaceBar}, {TEXT("LeftControl"), EKeys::LeftControl},
        {TEXT("C"), EKeys::C}, {TEXT("LeftShift"), EKeys::LeftShift}};
    auto Raw = MakeShared<FJsonObject>();
    for (const auto& Key : Keys)
    {
        if (PC && PC->IsLocalController()) Raw->SetBoolField(Key.Key, PC->IsInputKeyDown(Key.Value));
        else Raw->SetField(Key.Key, MakeShared<FJsonValueNull>());
    }
    Input->SetObjectField(TEXT("keys"), Raw);
    TArray<TSharedPtr<FJsonValue>> Actions;
    auto Semantic = MakeShared<FJsonObject>();
    for (const TCHAR* Name : {TEXT("Jump"), TEXT("Crouch"), TEXT("Sprint")}) Semantic->SetField(Name, MakeShared<FJsonValueNull>());
    if (auto* Enhanced = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr)
    {
        TSet<const UInputAction*> Seen;
        for (const FEnhancedActionKeyMapping& Mapping : Enhanced->GetEnhancedActionMappingsView())
        {
            const UInputAction* Action = Mapping.Action;
            if (!Action || Seen.Contains(Action)) continue;
            Seen.Add(Action);
            auto Item = MakeShared<FJsonObject>();
            Item->SetStringField(TEXT("action"), Action->GetPathName());
            if (const FInputActionInstance* Data = Enhanced->FindActionInstanceData(Action))
            {
                const auto Value = Data->GetValue();
                const bool Active = Value.GetMagnitudeSq() > SMALL_NUMBER;
                Item->SetStringField(TEXT("value"), Value.ToString());
                Item->SetBoolField(TEXT("nonzero"), Active);
                Item->SetStringField(TEXT("trigger_event"), StaticEnum<ETriggerEvent>()->GetNameStringByValue(int64(Data->GetTriggerEvent())));
                Item->SetNumberField(TEXT("elapsed_seconds"), Data->GetElapsedTime());
                for (const TCHAR* Name : {TEXT("Jump"), TEXT("Crouch"), TEXT("Sprint")})
                    if (Action->GetName().Contains(Name))
                    {
                        bool Previous = false; Semantic->TryGetBoolField(Name, Previous);
                        Semantic->SetBoolField(Name, Previous || Active);
                    }
            }
            else Item->SetField(TEXT("value"), MakeShared<FJsonValueNull>());
            Actions.Add(MakeShared<FJsonValueObject>(Item));
        }
    }
    Input->SetObjectField(TEXT("semantic_actions_by_name"), Semantic);
    Input->SetArrayField(TEXT("enhanced_actions"), Actions);
    Root->SetObjectField(TEXT("input"), Input);

    auto Movement = MakeShared<FJsonObject>();
    Vector(Movement, TEXT("location"), Character->GetActorLocation());
    Vector(Movement, TEXT("velocity"), Character->GetVelocity());
    Movement->SetNumberField(TEXT("horizontal_speed"), Character->GetVelocity().Size2D());
    Movement->SetNumberField(TEXT("velocity_z"), Character->GetVelocity().Z);
    Movement->SetBoolField(TEXT("is_crouched"), Character->bIsCrouched);
    if (const auto* M = Character->GetCharacterMovement())
    {
        Vector(Movement, TEXT("acceleration"), M->GetCurrentAcceleration());
        Movement->SetBoolField(TEXT("is_falling"), M->IsFalling());
        Movement->SetBoolField(TEXT("moving_on_ground"), M->IsMovingOnGround());
        Movement->SetStringField(TEXT("movement_mode"), StaticEnum<EMovementMode>()->GetNameStringByValue(M->MovementMode));
        Movement->SetNumberField(TEXT("custom_movement_mode"), M->CustomMovementMode);
        Movement->SetBoolField(TEXT("floor_blocking_hit"), M->CurrentFloor.bBlockingHit);
        Movement->SetBoolField(TEXT("floor_walkable"), M->CurrentFloor.IsWalkableFloor());
        Movement->SetNumberField(TEXT("floor_distance"), M->CurrentFloor.FloorDist);
        Vector(Movement, TEXT("floor_normal"), M->CurrentFloor.HitResult.ImpactNormal);
    }
    Root->SetObjectField(TEXT("movement"), Movement);

    TArray<TSharedPtr<FJsonValue>> Meshes;
    TInlineComponentArray<USkeletalMeshComponent*> Components(Character);
    for (USkeletalMeshComponent* Mesh : Components)
    {
        auto Anim = MakeShared<FJsonObject>();
        Anim->SetStringField(TEXT("mesh"), Mesh->GetPathName());
        UAnimInstance* Instance = Mesh->GetAnimInstance();
        Anim->SetStringField(TEXT("instance"), GetPathNameSafe(Instance));
        Anim->SetStringField(TEXT("class"), Instance ? Instance->GetClass()->GetPathName() : TEXT("None"));
        const bool Safe = Instance && !Mesh->IsRunningParallelEvaluation();
        Anim->SetStringField(TEXT("status"), !Instance ? TEXT("no_anim_instance") : Safe ? TEXT("available") : TEXT("parallel_evaluation_in_flight"));
        if (Safe)
        {
            TArray<TSharedPtr<FJsonValue>> Players, Machines;
            if (const auto* Interface = IAnimClassInterface::GetFromClass(Instance->GetClass()))
            {
                for (FStructProperty* Property : Interface->GetAnimNodeProperties())
                {
                    if (Property->Struct->IsChildOf(FAnimNode_SequencePlayerBase::StaticStruct()))
                    {
                        const auto* Node = Property->ContainerPtrToValuePtr<FAnimNode_SequencePlayerBase>(Instance);
                        auto P = MakeShared<FJsonObject>();
                        UAnimSequenceBase* Sequence = Node->GetSequence();
                        P->SetStringField(TEXT("node"), Property->GetName());
                        P->SetStringField(TEXT("sequence"), GetPathNameSafe(Sequence));
                        P->SetNumberField(TEXT("time_seconds"), Node->GetAccumulatedTime());
                        if (Property->Struct == FAnimNode_SequencePlayer::StaticStruct())
                            P->SetNumberField(TEXT("start_position"), Property->ContainerPtrToValuePtr<FAnimNode_SequencePlayer>(Instance)->GetStartPosition());
                        else P->SetField(TEXT("start_position"), MakeShared<FJsonValueNull>());
                        P->SetNumberField(TEXT("cached_blend_weight"), Node->GetCachedBlendWeight());
                        P->SetNumberField(TEXT("length_seconds"), Sequence ? Sequence->GetPlayLength() : 0);
                        if (Sequence && Sequence->GetPlayLength() > 0)
                            P->SetNumberField(TEXT("normalized_time"), Node->GetAccumulatedTime() / Sequence->GetPlayLength());
                        else P->SetField(TEXT("normalized_time"), MakeShared<FJsonValueNull>());
                        Players.Add(MakeShared<FJsonValueObject>(P));
                    }
                    else if (Property->Struct->IsChildOf(FAnimNode_StateMachine::StaticStruct()))
                    {
                        const auto* Node = Property->ContainerPtrToValuePtr<FAnimNode_StateMachine>(Instance);
                        auto S = MakeShared<FJsonObject>();
                        S->SetStringField(TEXT("node"), Property->GetName());
                        S->SetNumberField(TEXT("machine_index"), Node->StateMachineIndexInClass);
                        S->SetStringField(TEXT("state"), Node->GetCurrentStateName().ToString());
                        S->SetNumberField(TEXT("elapsed_seconds"), Node->GetCurrentStateElapsedTime());
                        Machines.Add(MakeShared<FJsonValueObject>(S));
                    }
                }
            }
            Anim->SetArrayField(TEXT("sequence_players"), Players);
            Anim->SetArrayField(TEXT("state_machines"), Machines);
            Anim->SetObjectField(TEXT("survival"), RuntimeDiagnostics::ObserveSurvival(Instance));
        }
        Meshes.Add(MakeShared<FJsonValueObject>(Anim));
    }
    Root->SetArrayField(TEXT("animation"), Meshes);
    return Root;
}
FString Quote(FString Value) { Value.ReplaceInline(TEXT("\""), TEXT("\"\"")); return TEXT("\"") + Value + TEXT("\""); }
void Write(FArchive& Archive, const FString& Text)
{
    FTCHARToUTF8 Bytes(*Text); Archive.Serialize((void*)Bytes.Get(), Bytes.Length());
}
}

void URuntimeDiagnosticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PostTickHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &ThisClass::PostTick);
    EndPIEHandle = FEditorDelegates::EndPIE.AddUObject(this, &ThisClass::EndPIE);
    WatchdogHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::Watchdog));
}
void URuntimeDiagnosticsSubsystem::Deinitialize()
{
    StopCapture();
    FWorldDelegates::OnWorldPostActorTick.Remove(PostTickHandle);
    FEditorDelegates::EndPIE.Remove(EndPIEHandle);
    FTSTicker::GetCoreTicker().RemoveTicker(WatchdogHandle);
    SetOverlay(false);
    Super::Deinitialize();
}
bool URuntimeDiagnosticsSubsystem::IsPIERunning() const { return RD::World(-1) != nullptr; }
FString URuntimeDiagnosticsSubsystem::ListPIEWorlds() const
{
    auto Root = MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Worlds;
    if (GEngine) for (const FWorldContext& C : GEngine->GetWorldContexts()) if (C.WorldType == EWorldType::PIE && C.World())
    {
        auto W = MakeShared<FJsonObject>(); W->SetNumberField(TEXT("pie_instance"), C.PIEInstance);
        W->SetStringField(TEXT("world"), C.World()->GetPathName());
        W->SetNumberField(TEXT("net_mode"), int32(C.World()->GetNetMode())); Worlds.Add(MakeShared<FJsonValueObject>(W));
    }
    Root->SetArrayField(TEXT("worlds"), Worlds); return RD::Json(Root);
}
ACharacter* URuntimeDiagnosticsSubsystem::GetPlayerCharacter(int32 PlayerIndex, int32 PIEInstance) const
{
    UWorld* W = RD::World(PIEInstance);
    return W && PlayerIndex >= 0 ? UGameplayStatics::GetPlayerCharacter(W, PlayerIndex) : nullptr;
}
FString URuntimeDiagnosticsSubsystem::Snapshot(int32 PlayerIndex, int32 PIEInstance) const
{
    return SnapshotCharacter(GetPlayerCharacter(PlayerIndex, PIEInstance));
}
FString URuntimeDiagnosticsSubsystem::SnapshotCharacter(ACharacter* Character) const
{
    check(IsInGameThread()); return RD::Json(RD::Observe(Character));
}
FString URuntimeDiagnosticsSubsystem::StartCapture(float DurationSeconds, int32 PlayerIndex, int32 PIEInstance)
{
    check(IsInGameThread());
    if (Writer) return TEXT("ERROR: capture already running; stop it first");
    if (!FMath::IsFinite(DurationSeconds) || DurationSeconds <= 0 || DurationSeconds > 300) return TEXT("ERROR: duration must be in (0, 300]");
    ACharacter* Character = GetPlayerCharacter(PlayerIndex, PIEInstance);
    if (!Character) return TEXT("ERROR: no player character in selected PIE world");
    const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("RuntimeDiagnostics"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Path = Directory / (TEXT("Capture_") + FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S")) + TEXT("_") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".csv"));
    Writer.Reset(IFileManager::Get().CreateFileWriter(*Path, FILEWRITE_NoReplaceExisting));
    if (!Writer) return TEXT("ERROR: cannot create CSV");
    LastCapturePath = Path; CaptureCharacter = Character; Samples = 0; LastStopReason.Reset();
    Deadline = FPlatformTime::Seconds() + DurationSeconds;
    RD::Write(*Writer, TEXT("frame,monotonic_seconds,pie_seconds,delta_seconds,horizontal_speed,velocity_z,is_falling,is_crouched,input_json,movement_json,animation_json,snapshot_json\n"));
    return Path;
}
FString URuntimeDiagnosticsSubsystem::StopCapture()
{
    if (Writer)
    {
        Writer->Flush();
        const bool Failed = Writer->IsError();
        Writer.Reset();
        LastStopReason = Failed ? TEXT("write_error") : TEXT("stopped");
    }
    CaptureCharacter.Reset(); return LastCapturePath;
}
FString URuntimeDiagnosticsSubsystem::CaptureStatus() const
{
    auto R = MakeShared<FJsonObject>(); R->SetBoolField(TEXT("recording"), Writer.IsValid());
    R->SetStringField(TEXT("path"), LastCapturePath); R->SetNumberField(TEXT("samples"), Samples);
    R->SetStringField(TEXT("stop_reason"), LastStopReason); return RD::Json(R);
}
void URuntimeDiagnosticsSubsystem::SetOverlay(bool Enabled, int32 PlayerIndex, int32 PIEInstance)
{
    OverlayCharacter = Enabled ? GetPlayerCharacter(PlayerIndex, PIEInstance) : nullptr;
    if (!Enabled && GEngine) GEngine->RemoveOnScreenDebugMessage(0x52444941);
}
void URuntimeDiagnosticsSubsystem::EndPIE(bool)
{
    StopCapture(); SetOverlay(false);
    if (!LastCapturePath.IsEmpty()) LastStopReason = TEXT("end_pie");
}
bool URuntimeDiagnosticsSubsystem::Watchdog(float)
{
    if (Writer && (!CaptureCharacter.IsValid() || FPlatformTime::Seconds() >= Deadline))
    {
        const FString Reason = CaptureCharacter.IsValid() ? TEXT("duration_complete") : TEXT("character_destroyed");
        StopCapture(); LastStopReason = Reason;
    }
    return true;
}
void URuntimeDiagnosticsSubsystem::PostTick(UWorld* World, ELevelTick, float DeltaSeconds)
{
    Watchdog(0);
    if (Writer && CaptureCharacter.IsValid() && CaptureCharacter->GetWorld() == World)
    {
        auto R = RD::Observe(CaptureCharacter.Get());
        const auto M = R->GetObjectField(TEXT("movement"));
        auto Animation = MakeShared<FJsonObject>(); Animation->SetArrayField(TEXT("meshes"), R->GetArrayField(TEXT("animation")));
        const FString Row = FString::Printf(TEXT("%s,%.9f,%.6f,%.6f,%.6f,%.6f,%d,%d,%s,%s,%s,%s\n"),
            *R->GetStringField(TEXT("frame")), R->GetNumberField(TEXT("monotonic_seconds")), R->GetNumberField(TEXT("pie_seconds")), DeltaSeconds,
            M->GetNumberField(TEXT("horizontal_speed")), M->GetNumberField(TEXT("velocity_z")), M->GetBoolField(TEXT("is_falling")), M->GetBoolField(TEXT("is_crouched")),
            *RD::Quote(RD::Json(R->GetObjectField(TEXT("input")).ToSharedRef())), *RD::Quote(RD::Json(M.ToSharedRef())),
            *RD::Quote(RD::Json(Animation)), *RD::Quote(RD::Json(R)));
        RD::Write(*Writer, Row); ++Samples;
        if (Writer->IsError()) { StopCapture(); LastStopReason = TEXT("write_error"); }
    }
    ACharacter* C = OverlayCharacter.Get();
    if (GEngine && C && C->GetWorld() == World)
    {
        auto* PC = Cast<APlayerController>(C->GetController());
        FString Keys;
        if (PC) for (FKey Key : {EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::SpaceBar, EKeys::LeftControl, EKeys::LeftShift})
            if (PC->IsInputKeyDown(Key)) Keys += Key.ToString() + TEXT(" ");
        GEngine->AddOnScreenDebugMessage(0x52444941, 0.15f, FColor::Green,
            FString::Printf(TEXT("RD frame=%llu speed=%.1f vz=%.1f falling=%d crouched=%d\nKeys: %s"), GFrameCounter,
                C->GetVelocity().Size2D(), C->GetVelocity().Z, C->GetCharacterMovement()->IsFalling(), C->bIsCrouched, *Keys));
    }
}
