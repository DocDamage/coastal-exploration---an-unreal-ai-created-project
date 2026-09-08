#pragma once
#include "CoreMinimal.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"

// Inspected Blueprint signatures are checked at the boundary. No generated vendor
// headers or binary layouts are copied into the original project.
namespace CoastalVendor
{
inline FProperty* Field(const UStruct* Type, const TCHAR* Name)
{
    if (!Type) return nullptr;
    if (FProperty* Exact = Type->FindPropertyByName(Name)) return Exact;
    FProperty* Result = nullptr;
    const FString Prefix = FString(Name) + TEXT("_");
    for (TFieldIterator<FProperty> It(Type); It; ++It)
        if (It->GetName().StartsWith(Prefix))
        { if (Result) return nullptr; Result = *It; }
    return Result;
}
template<class P> P* FieldAs(const UStruct* Type, const TCHAR* Name)
{ return CastField<P>(Field(Type, Name)); }
template<class T> T* Value(const UStruct* Type, void* Data, const TCHAR* Name)
{ auto* P = Field(Type, Name); return P ? P->ContainerPtrToValuePtr<T>(Data) : nullptr; }
inline bool SetInt(const UStruct* T, void* D, const TCHAR* N, int32 V)
{ auto* P = FieldAs<FIntProperty>(T,N); if (!P) return false; P->SetPropertyValue_InContainer(D,V); return true; }
inline bool SetBool(const UStruct* T, void* D, const TCHAR* N, bool V)
{ auto* P = FieldAs<FBoolProperty>(T,N); if (!P) return false; P->SetPropertyValue_InContainer(D,V); return true; }
inline bool SetName(const UStruct* T, void* D, const TCHAR* N, FName V)
{ auto* P = FieldAs<FNameProperty>(T,N); if (!P) return false; P->SetPropertyValue_InContainer(D,V); return true; }
inline bool SetObject(const UStruct* T, void* D, const TCHAR* N, UObject* V)
{ auto* P = FieldAs<FObjectPropertyBase>(T,N); if (!P || (V && !V->IsA(P->PropertyClass))) return false; P->SetObjectPropertyValue_InContainer(D,V); return true; }
inline bool CopyStruct(const UStruct* T, void* D, const TCHAR* N, const UScriptStruct* S, const void* V)
{ auto* P = FieldAs<FStructProperty>(T,N); if (!P || P->Struct != S) return false; P->CopyCompleteValue(P->ContainerPtrToValuePtr<void>(D),V); return true; }
struct Call
{
    UObject* Target;
    UFunction* Function;
    FStructOnScope Params;
    Call(UObject* InTarget, const TCHAR* Name) : Target(InTarget),
        Function(InTarget ? InTarget->FindFunction(Name) : nullptr), Params(Function) {}
    void* Data() { return Params.GetStructMemory(); }
    bool Run() { if (!Function || !IsInGameThread()) return false; Target->ProcessEvent(Function,Data()); return true; }
    bool Int(const TCHAR* N,int32 V) { return SetInt(Function,Data(),N,V); }
    bool Bool(const TCHAR* N,bool V) { return SetBool(Function,Data(),N,V); }
    bool Name(const TCHAR* N,FName V) { return SetName(Function,Data(),N,V); }
    bool Object(const TCHAR* N,UObject* V) { return SetObject(Function,Data(),N,V); }
    bool Struct(const TCHAR* N,const UScriptStruct* S,const void* V) { return CopyStruct(Function,Data(),N,S,V); }
    bool Result(const TCHAR* N=TEXT("Success")) { auto* P=FieldAs<FBoolProperty>(Function,N); return P && P->GetPropertyValue_InContainer(Data()); }
};
}
