#pragma once
#include "CoreMinimal.h"
#include "CoastalIntegrationTypes.generated.h"
UENUM(BlueprintType)
enum class ECoastalStartupPhase : uint8 { Idle, Checking, Blocked, Binding, Ready, RestartRequired, Stopped };
UENUM(BlueprintType)
enum class ECoastalStartupResult : uint8 { Started, AlreadyStarted, Blocked, Busy, RestartRequired, Rejected };
USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FCoastalIntegrationIssue
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Integration") FName Code;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Integration") FString Subject;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Integration") FString Detail;
};
USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FCoastalIntegrationReport
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Integration") bool bPassed = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Integration") TArray<FCoastalIntegrationIssue> Issues;
    void Add(FName Code, const FString& Subject, const FString& Detail)
    {
        FCoastalIntegrationIssue Issue; Issue.Code = Code; Issue.Subject = Subject; Issue.Detail = Detail;
        Issues.Add(MoveTemp(Issue)); bPassed = false;
    }
    void Finish() { bPassed = Issues.IsEmpty(); }
    FString ToText() const;
};
