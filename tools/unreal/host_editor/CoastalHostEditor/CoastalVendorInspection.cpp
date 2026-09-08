#include "CoastalVendorInspection.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_CallFunction.h"
#include "UObject/UnrealType.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
namespace {
using Obj = TSharedPtr<FJsonObject>;
TSharedPtr<FJsonValue> Value(Obj O) { return MakeShared<FJsonValueObject>(O); }
Obj Property(FProperty* P, const void* Container) {
 auto O=MakeShared<FJsonObject>();
 O->SetStringField("name",P->GetName()); O->SetStringField("type",P->GetCPPType());
 O->SetStringField("flags",FString::Printf(TEXT("%llu"),static_cast<uint64>(P->PropertyFlags)));
 if(Container) { FString V; P->ExportTextItem_Direct(V,P->ContainerPtrToValuePtr<void>(Container),nullptr,nullptr,PPF_None); O->SetStringField("value",V); }
 return O;
}
}
bool UCoastalVendorInspection::DumpBlueprint(const FString& AssetPath,const FString& OutputFile) {
 auto* Asset=LoadObject<UObject>(nullptr,*AssetPath);
 if(!Asset)return false;
 Obj Root=MakeShared<FJsonObject>(); Root->SetStringField("asset",AssetPath);
 UBlueprint* BP=Cast<UBlueprint>(Asset);
 UStruct* Type=BP ? BP->GeneratedClass : Cast<UStruct>(Asset);
 if(!Type)return false;
 Root->SetStringField("type",Type->GetPathName());
 if(Type->GetSuperStruct())Root->SetStringField("parent",Type->GetSuperStruct()->GetPathName());
 const void* Defaults=BP ? BP->GeneratedClass->GetDefaultObject() : nullptr;
 TArray<TSharedPtr<FJsonValue>> Props, Funcs, Graphs;
 for(TFieldIterator<FProperty> P(Type);P;++P)Props.Add(Value(Property(*P,Defaults)));
 Root->SetArrayField("properties",Props);
 if(BP) {
  Root->SetNumberField("status",BP->Status);
  for(TFieldIterator<UFunction> F(Type);F;++F) {
   if(!F->GetOuter()->GetPathName().StartsWith(TEXT("/Game/")))continue;
   auto O=MakeShared<FJsonObject>(); O->SetStringField("name",F->GetName()); O->SetStringField("owner",F->GetOuter()->GetPathName());
   O->SetNumberField("flags",F->FunctionFlags); TArray<TSharedPtr<FJsonValue>> Params;
   for(TFieldIterator<FProperty> P(*F);P && P->HasAnyPropertyFlags(CPF_Parm);++P)Params.Add(Value(Property(*P,nullptr)));
   O->SetArrayField("params",Params); Funcs.Add(Value(O));
  }
  TArray<UEdGraph*> All; BP->GetAllGraphs(All);
  for(auto* G:All) {
   auto GO=MakeShared<FJsonObject>(); GO->SetStringField("name",G->GetName()); GO->SetStringField("path",G->GetPathName()); TArray<TSharedPtr<FJsonValue>> Nodes;
   for(UEdGraphNode* N:G->Nodes) { if(!N)continue;
    auto NO=MakeShared<FJsonObject>(); NO->SetStringField("name",N->GetName()); NO->SetStringField("class",N->GetClass()->GetName()); NO->SetStringField("title",N->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
    if(auto* C=Cast<UK2Node_CallFunction>(N))NO->SetStringField("function",C->FunctionReference.GetMemberName().ToString());
    TArray<TSharedPtr<FJsonValue>> Pins;
    for(auto* P:N->Pins) { if(!P)continue; auto PO=MakeShared<FJsonObject>(); PO->SetStringField("name",P->PinName.ToString()); PO->SetNumberField("direction",P->Direction);
     PO->SetStringField("category",P->PinType.PinCategory.ToString()); PO->SetStringField("default",P->DefaultValue); if(P->DefaultObject)PO->SetStringField("object",P->DefaultObject->GetPathName());
     if(P->PinType.PinSubCategoryObject.IsValid())PO->SetStringField("type",P->PinType.PinSubCategoryObject->GetPathName());
     TArray<TSharedPtr<FJsonValue>> Links; for(auto* L:P->LinkedTo)Links.Add(MakeShared<FJsonValueString>(L->GetOwningNode()->GetName()+TEXT(".")+L->PinName.ToString()));
     PO->SetArrayField("links",Links); Pins.Add(Value(PO));
    }
    NO->SetArrayField("pins",Pins); Nodes.Add(Value(NO));
   }
   GO->SetArrayField("nodes",Nodes); Graphs.Add(Value(GO));
  }
 }
 Root->SetArrayField("functions",Funcs); Root->SetArrayField("graphs",Graphs);
 FString Text; auto Writer=TJsonWriterFactory<>::Create(&Text); FJsonSerializer::Serialize(Root,Writer);
 return FFileHelper::SaveStringToFile(Text,*OutputFile);
}
