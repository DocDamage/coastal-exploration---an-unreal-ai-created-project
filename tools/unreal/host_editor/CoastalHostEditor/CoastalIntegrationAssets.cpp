#include "CoastalVendorInspection.h"
#include "CoastalWorldObject.h"
#include "CoastalHostSession.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_Self.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Components/StaticMeshComponent.h"
namespace {
UBlueprint* Create(UClass* Parent,const TCHAR* Name) {
 const FString Path=TEXT("/Game/Coastal/Integration/")+FString(Name);
 if(FPackageName::DoesPackageExist(Path)){UE_LOG(LogTemp,Error,TEXT("Refusing to overwrite %s"),*Path);return nullptr;}
 auto* Package=CreatePackage(*Path);
 return FKismetEditorUtilities::CreateBlueprint(Parent,Package,FName(Name),BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
}
bool Save(UBlueprint* BP) {
 FKismetEditorUtilities::CompileBlueprint(BP);
 if(BP->Status==BS_Error)return false;
 FAssetRegistryModule::AssetCreated(BP); BP->MarkPackageDirty();
 FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
 return UPackage::SavePackage(BP->GetOutermost(),BP,*FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
}
template<class T>T* Node(UEdGraph* G){auto* N=NewObject<T>(G);G->AddNode(N,false,false);N->CreateNewGuid();return N;}
bool Link(UEdGraph* G,UEdGraphNode* A,const TCHAR* AP,UEdGraphNode* B,const TCHAR* BP) {
 auto* P=A->FindPin(AP);auto* Q=B->FindPin(BP);
 if(!P||!Q){UE_LOG(LogTemp,Error,TEXT("Missing pin %s or %s"),AP,BP);return false;}
 return G->GetSchema()->TryCreateConnection(P,Q);
}
bool WireInterface(UBlueprint* BP,const TCHAR* GraphName,const TCHAR* NativeName,const TArray<TPair<FString,FString>>& Outputs) {
 UEdGraph* Graph=nullptr;
 for(const auto& Interface:BP->ImplementedInterfaces)for(UEdGraph* G:Interface.Graphs)if(G->GetFName()==GraphName)Graph=G;
 if(!Graph){UE_LOG(LogTemp,Error,TEXT("Missing interface graph %s"),GraphName);return false;}
 UK2Node_FunctionEntry* Entry=nullptr; UK2Node_FunctionResult* Result=nullptr;
 for(UEdGraphNode* N:Graph->Nodes){if(auto* E=Cast<UK2Node_FunctionEntry>(N))Entry=E;if(auto* R=Cast<UK2Node_FunctionResult>(N))Result=R;}
 if(!Entry||!Result)return false;
 auto* Call=Node<UK2Node_CallFunction>(Graph); Call->SetFromFunction(UCoastalHyperFunctions::StaticClass()->FindFunctionByName(NativeName)); Call->AllocateDefaultPins();
 auto* Self=Node<UK2Node_Self>(Graph);Self->AllocateDefaultPins();
 if(!Link(Graph,Self,TEXT("self"),Call,TEXT("Target")) || !Link(Graph,Entry,TEXT("Owning Controller"),Call,TEXT("Controller")))return false;
 for(const auto& O:Outputs)if(!Link(Graph,Call,*O.Key,Result,*O.Value))return false;
 return true;
}
}
bool UCoastalVendorInspection::BuildIntegrationAssets() {
 auto* Interface=LoadClass<UInterface>(nullptr,TEXT("/Game/Hyper/Core/Interaction/Interfaces/BPI_CanInteract.BPI_CanInteract_C"));
 auto* Interact=LoadClass<UInterface>(nullptr,TEXT("/Game/Hyper/Core/Interaction/Interfaces/BPI_Interact.BPI_Interact_C"));
 auto* HyperParent=LoadClass<UActorComponent>(nullptr,TEXT("/Game/Hyper/Interaction/Blueprints/AC_CH_Interact_Base.AC_CH_Interact_Base_C"));
 auto* ControllerParent=LoadClass<APlayerController>(nullptr,TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController.BP_ThirdPersonPlayerController_C"));
 if(!Interface||!Interact||!HyperParent||!ControllerParent)return false;
 UBlueprint* World=Create(ACoastalWorldObject::StaticClass(),TEXT("BP_CoastalHyperWorldObject")); if(!World)return false;
 if(!FBlueprintEditorUtils::ImplementNewInterface(World,FTopLevelAssetPath(Interface)) || !FBlueprintEditorUtils::ImplementNewInterface(World,FTopLevelAssetPath(Interact)))return false;
 if(!WireInterface(World,TEXT("Can Interact"),TEXT("CanPresent"),{{TEXT("ReturnValue"),TEXT("True")}})
    || !WireInterface(World,TEXT("Get Interact Text"),TEXT("PromptText"),{{TEXT("Primary"),TEXT("Interact Text")},{TEXT("Secondary"),TEXT("Secondary Interact Text")}}))return false;
 // Radius icons are deliberately disabled; target selection/prompt remain Hyper's.
 for(const auto& Impl:World->ImplementedInterfaces)for(UEdGraph* G:Impl.Graphs)for(UEdGraphNode* N:G->Nodes)if(auto* R=Cast<UK2Node_FunctionResult>(N)) {
  if(G->GetName()==TEXT("Should hide interactable in radius widget")) {if(auto* Pin=R->FindPin(TEXT("Hide")))Pin->DefaultValue=TEXT("true");}
  if(G->GetName()==TEXT("Get Relative Interact Text Location")) {if(auto* Pin=R->FindPin(TEXT("Relative Location")))Pin->DefaultValue=TEXT("0,0,90");}
 }
 FKismetEditorUtilities::CompileBlueprint(World);
 CastChecked<ACoastalWorldObject>(World->GeneratedClass->GetDefaultObject())->ProxyMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
 if(!Save(World))return false;
 UBlueprint* Focus=Create(HyperParent,TEXT("BP_CoastalHyperFocus"));if(!Focus)return false;
 UEdGraph* G=Focus->UbergraphPages[0];
 auto* Begin=Node<UK2Node_Event>(G);Begin->EventReference.SetExternalMember(TEXT("ReceiveBeginPlay"),UActorComponent::StaticClass());Begin->bOverrideFunction=true;Begin->AllocateDefaultPins();
 auto* Stop=Node<UK2Node_CallFunction>(G);Stop->SetFromFunction(UActorComponent::StaticClass()->FindFunctionByName(TEXT("SetComponentTickEnabled")));Stop->AllocateDefaultPins();
 if(!Link(G,Begin,TEXT("then"),Stop,TEXT("execute")))return false;
 Stop->FindPinChecked(TEXT("bEnabled"))->DefaultValue=TEXT("false");
 if(!Save(Focus))return false;
 UBlueprint* Controller=Create(ControllerParent,TEXT("BP_CoastalPlayerController"));if(!Controller)return false;
 auto* Host=Controller->SimpleConstructionScript->CreateNode(UCoastalHostSession::StaticClass(),TEXT("CoastalHostSession"));
 Controller->SimpleConstructionScript->AddNode(Host);if(!Save(Controller))return false;
 UE_LOG(LogTemp,Display,TEXT("COASTAL_INTEGRATION_ASSETS_CREATED"));return true;
}
