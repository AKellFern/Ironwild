#include "RuntimeDiagnosticsToolset.h"
#include "RuntimeDiagnosticsSubsystem.h"
#include "Editor.h"
#include "GameFramework/Character.h"
#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

namespace { URuntimeDiagnosticsSubsystem* Diagnostics() { return GEditor->GetEditorSubsystem<URuntimeDiagnosticsSubsystem>(); } }
bool URuntimeDiagnosticsToolset::IsPIERunning() { return Diagnostics()->IsPIERunning(); }
FString URuntimeDiagnosticsToolset::ListPIEWorlds() { return Diagnostics()->ListPIEWorlds(); }
ACharacter* URuntimeDiagnosticsToolset::GetPlayerCharacter(int32 PlayerIndex, int32 PIEInstance) { return Diagnostics()->GetPlayerCharacter(PlayerIndex, PIEInstance); }
FString URuntimeDiagnosticsToolset::Snapshot(int32 PlayerIndex, int32 PIEInstance) { return Diagnostics()->Snapshot(PlayerIndex, PIEInstance); }
FString URuntimeDiagnosticsToolset::SnapshotCharacter(ACharacter* Character) { return Diagnostics()->SnapshotCharacter(Character); }
FString URuntimeDiagnosticsToolset::StartCapture(float DurationSeconds, int32 PlayerIndex, int32 PIEInstance) { return Diagnostics()->StartCapture(DurationSeconds, PlayerIndex, PIEInstance); }
FString URuntimeDiagnosticsToolset::StopCapture() { return Diagnostics()->StopCapture(); }
FString URuntimeDiagnosticsToolset::CaptureStatus() { return Diagnostics()->CaptureStatus(); }
void URuntimeDiagnosticsToolset::SetOverlay(bool Enabled, int32 PlayerIndex, int32 PIEInstance) { Diagnostics()->SetOverlay(Enabled, PlayerIndex, PIEInstance); }

class FRuntimeDiagnosticsMCPModule : public IModuleInterface
{
public:
    virtual void StartupModule() override { UToolsetRegistry::RegisterToolsetClass(URuntimeDiagnosticsToolset::StaticClass()); }
    virtual void ShutdownModule() override { UToolsetRegistry::UnregisterToolsetClass(URuntimeDiagnosticsToolset::StaticClass()); }
};
IMPLEMENT_MODULE(FRuntimeDiagnosticsMCPModule, RuntimeDiagnosticsMCP)
