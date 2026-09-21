#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "EditorSubsystem.h"
#include "RuntimeDiagnosticsSubsystem.generated.h"

class ACharacter;
class FArchive;

/** Editor-only, game-thread observation. No actor injection or gameplay inheritance required. */
UCLASS()
class RUNTIMEDIAGNOSTICS_API URuntimeDiagnosticsSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Runtime Diagnostics")
    bool IsPIERunning() const;

    /** JSON world list with PIE instance IDs; use these IDs for multi-client sessions. */
    UFUNCTION(BlueprintCallable, Category="Runtime Diagnostics")
    FString ListPIEWorlds() const;

    /** -1 selects the editor PlayWorld; PlayerIndex is local to that world. */
    UFUNCTION(BlueprintCallable, Category="Runtime Diagnostics")
    ACharacter* GetPlayerCharacter(int32 PlayerIndex = 0, int32 PIEInstance = -1) const;

    UFUNCTION(BlueprintCallable, Category="Runtime Diagnostics")
    FString Snapshot(int32 PlayerIndex = 0, int32 PIEInstance = -1) const;

    /** Also supports a specific possessed or unpossessed PIE character. */
    UFUNCTION(BlueprintCallable, Category="Runtime Diagnostics")
    FString SnapshotCharacter(ACharacter* Character) const;

    /** Returns absolute CSV path, or ERROR: reason. Duration must be in (0, 300] seconds. */
    UFUNCTION(BlueprintCallable, Category="Runtime Diagnostics")
    FString StartCapture(float DurationSeconds = 10.0f, int32 PlayerIndex = 0, int32 PIEInstance = -1);

    UFUNCTION(BlueprintCallable, Category="Runtime Diagnostics")
    FString StopCapture();

    UFUNCTION(BlueprintCallable, Category="Runtime Diagnostics")
    FString CaptureStatus() const;

    UFUNCTION(BlueprintCallable, Category="Runtime Diagnostics")
    void SetOverlay(bool Enabled, int32 PlayerIndex = 0, int32 PIEInstance = -1);

private:
    void PostTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);
    bool Watchdog(float DeltaSeconds);
    void EndPIE(bool Simulating);
    FDelegateHandle PostTickHandle, EndPIEHandle;
    FTSTicker::FDelegateHandle WatchdogHandle;
    TWeakObjectPtr<ACharacter> CaptureCharacter, OverlayCharacter;
    TUniquePtr<FArchive> Writer;
    FString LastCapturePath, LastStopReason;
    double Deadline = 0;
    int32 Samples = 0;
};
