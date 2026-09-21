#pragma once
#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "RuntimeDiagnosticsToolset.generated.h"

class ACharacter;

/** Read-only live PIE observation; recording and overlay are opt-in diagnostic operations. */
UCLASS()
class URuntimeDiagnosticsToolset : public UToolsetDefinition
{
    GENERATED_BODY()
public:
    /** Whether an in-process PIE world exists. */
    UFUNCTION(meta=(AICallable))
    static bool IsPIERunning();
    /** JSON list of PIE worlds and instance IDs. */
    UFUNCTION(meta=(AICallable))
    static FString ListPIEWorlds();
    /** Resolve character in selected PIE instance; -1 uses editor PlayWorld. */
    UFUNCTION(meta=(AICallable))
    static ACharacter* GetPlayerCharacter(int32 PlayerIndex = 0, int32 PIEInstance = -1);
    /** JSON snapshot of input, movement, animation nodes/timing and optional game adapter. */
    UFUNCTION(meta=(AICallable))
    static FString Snapshot(int32 PlayerIndex = 0, int32 PIEInstance = -1);
    /** Snapshot an explicit PIE character reference. */
    UFUNCTION(meta=(AICallable))
    static FString SnapshotCharacter(ACharacter* Character);
    /** Start per-frame CSV for (0,300] wall-clock seconds. Returns absolute path or ERROR. */
    UFUNCTION(meta=(AICallable))
    static FString StartCapture(float DurationSeconds = 10.0f, int32 PlayerIndex = 0, int32 PIEInstance = -1);
    /** Stop and flush CSV; returns last absolute path. */
    UFUNCTION(meta=(AICallable))
    static FString StopCapture();
    /** JSON containing recording, path, sample count and stop reason. */
    UFUNCTION(meta=(AICallable))
    static FString CaptureStatus();
    /** Toggle basic on-screen physics and raw-key display. */
    UFUNCTION(meta=(AICallable))
    static void SetOverlay(bool Enabled, int32 PlayerIndex = 0, int32 PIEInstance = -1);
};
