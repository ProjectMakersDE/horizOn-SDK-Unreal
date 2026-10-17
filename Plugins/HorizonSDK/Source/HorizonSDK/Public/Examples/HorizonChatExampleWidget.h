#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HorizonChatExampleWidget.generated.h"
class AHorizonChatExample;
class UEditableTextBox;
class UComboBoxString;
class UTextBlock;
/** Plain UTextBlock feed; never interprets player text as markup. No network work from Tick. */
UCLASS()
class HORIZONSDK_API UHorizonChatExampleWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY() AHorizonChatExample* Example=nullptr;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
private:
    friend struct FHorizonChatNative;
    UPROPERTY() UComboBoxString* Kind=nullptr;
    UPROPERTY() UEditableTextBox* Context=nullptr;
    UPROPERTY() UEditableTextBox* Text=nullptr;
    UPROPERTY() UEditableTextBox* MessageId=nullptr;
    UPROPERTY() UEditableTextBox* Reason=nullptr;
    UPROPERTY() UTextBlock* Status=nullptr;
    UPROPERTY() UTextBlock* Feed=nullptr;
    UPROPERTY() UEditableTextBox* HistoryChannel=nullptr;
    UPROPERTY() UTextBlock* HistoryFeed=nullptr;
    FString HistoryCursor,HistoryIdentity,HistoryError;
    UFUNCTION() void Open();
    UFUNCTION() void Send();
    UFUNCTION() void Retry();
    UFUNCTION() void Reconcile();
    UFUNCTION() void Report();
    UFUNCTION() void ReadHistory();
    UFUNCTION() void OlderHistory();
    UFUNCTION() void ClearHistory();
    UFUNCTION() void HistoryFailed(const FString& Channel,const FString& Code);
};
