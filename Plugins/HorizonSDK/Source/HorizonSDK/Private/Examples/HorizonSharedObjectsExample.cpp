// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Examples/HorizonSharedObjectsExample.h"
#include "Examples/HorizonExampleIdentity.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonSharedObjectsManager.h"
#include "Transport/HorizonSharedObjectsTransportContract.h"
#include "Transport/HorizonValidatedActionsTransportContract.h"
#include "Engine/GameInstance.h"
#include "Misc/Base64.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
namespace HorizonSharedExampleStorage
{
FString Project(UHorizonSubsystem* S){return UHorizonExampleIdentity::Project(S->GetHttpClient()->GetApiKey());}
FString Slot(const FString& P,const FString& U){return TEXT("horizon-shared-original-")+UHorizonExampleIdentity::Project(P+TEXT("|")+U);}
UHorizonSharedOriginal* Copy(const UHorizonSharedOriginal* P)
{
    if(!P)return nullptr;auto* C=NewObject<UHorizonSharedOriginal>();
    C->Project=P->Project;C->UserId=P->UserId;C->Collection=P->Collection;C->IntentId=P->IntentId;C->Action=P->Action;C->ObjectId=P->ObjectId;C->GroupId=P->GroupId;C->ExpectedRevision=P->ExpectedRevision;C->Input=P->Input;C->SearchStringBase64=P->SearchStringBase64;C->OriginalHash=P->OriginalHash;C->SendSerial=P->SendSerial;C->Confirmed=P->Confirmed;C->ResolvedObjectId=P->ResolvedObjectId;
    return C;
}
bool Save(UHorizonSharedOriginal* P,const FString& Slot)
{
    if(!P)return false;auto* Copy=HorizonSharedExampleStorage::Copy(P);Copy->SearchStringBase64.Empty();
    for(auto& Row:Copy->Input.Search){const FTCHARToUTF8 Bytes(*Row.Value.StringValue,Row.Value.StringValue.Len()+1);if(Bytes.Length()>96)return false;Copy->SearchStringBase64.Add(FBase64::Encode(reinterpret_cast<const uint8*>(Bytes.Get()),Bytes.Length()));Row.Value.StringValue.Reset();}
    return UGameplayStatics::SaveGameToSlot(Copy,Slot,0);
}
UHorizonSharedOriginal* Load(const FString& S)
{
    auto* P=UGameplayStatics::DoesSaveGameExist(S,0)?Cast<UHorizonSharedOriginal>(UGameplayStatics::LoadGameFromSlot(S,0)):nullptr;
    if(!P||P->SearchStringBase64.Num()!=P->Input.Search.Num())return nullptr;
    for(int32 N=0;N<P->Input.Search.Num();++N){const FString& Encoded=P->SearchStringBase64[N];const FTCHARToUTF8 Enc(*Encoded,Encoded.Len()+1);std::vector<uint8_t> Bytes;if(!HorizonSharedContract::DecodeBase64(std::string(Enc.Get(),Enc.Length()),Bytes,96))return nullptr;Bytes.push_back('x');const FUTF8ToTCHAR Chars(reinterpret_cast<const char*>(Bytes.data()),int32(Bytes.size()));const int32 Count=Chars.Length()-1;auto& Out=P->Input.Search[N].Value.StringValue.GetCharArray();Out.SetNumUninitialized(Count+1);if(Count)FMemory::Memcpy(Out.GetData(),Chars.Get(),Count*sizeof(TCHAR));Out[Count]=0;}
    return P;
}
bool Uuid(const FString& S){FGuid G;return S.Len()==36&&FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G);}
bool Key(const FString& S){const FTCHARToUTF8 B(*S,S.Len()+1);return HorizonItemsContract::Key(std::string(B.Get(),B.Length()));}
void Add(FString& Out,const FString& S){Out+=LexToString(S.Len())+TEXT(":")+S;}
FString Hash(const FString& S){const FTCHARToUTF8 B(*S,S.Len()+1);return UTF8_TO_TCHAR(HorizonTransportContract::Sha256Hex(reinterpret_cast<const uint8*>(B.Get()),B.Length()).c_str());}
FString OriginalHash(const UHorizonSharedOriginal* P)
{
    if(!P)return {};FString S;for(const FString& V:{P->Project,P->UserId,P->Collection,P->IntentId,P->Action,P->ObjectId,P->GroupId,LexToString(P->ExpectedRevision),FBase64::Encode(P->Input.Data),LexToString(P->Input.bHasPosition),LexToString(P->Input.Position.X),LexToString(P->Input.Position.Y),LexToString(P->Input.Search.Num())})Add(S,V);
    for(const auto& V:P->Input.Search){Add(S,V.Key);Add(S,LexToString(uint8(V.Value.Type)));Add(S,LexToString(V.Value.IntegerValue));Add(S,V.Value.StringValue);}return Hash(S);
}
FString Signature(const UHorizonSharedOriginal* P){return P?Hash(OriginalHash(P)+TEXT("|")+P->OriginalHash+TEXT("|")+LexToString(P->SendSerial)+TEXT("|")+LexToString(P->Confirmed)+TEXT("|")+P->ResolvedObjectId):FString();}
bool InputValid(const FHorizonSharedObjectInput& I)
{
    if(I.Data.Num()>int32(HorizonSharedContract::MaxObjectBytes)||I.Search.Num()>3)return false;auto Safe=[](int64 N){return N>= -HorizonSharedContract::MaxSafe&&N<=HorizonSharedContract::MaxSafe;};if(I.bHasPosition&&(!Safe(I.Position.X)||!Safe(I.Position.Y)))return false;
    TSet<FString> Keys;for(const auto& V:I.Search){if(!Key(V.Key)||Keys.Contains(V.Key))return false;Keys.Add(V.Key);if(V.Value.Type==EHorizonSharedScalarType::Integer){if(!Safe(V.Value.IntegerValue))return false;}else if(V.Value.Type==EHorizonSharedScalarType::String){for(int32 N=0;N<V.Value.StringValue.Len();++N){uint16 C=V.Value.StringValue[N];if(C>=0xd800&&C<=0xdbff){if(++N>=V.Value.StringValue.Len()||V.Value.StringValue[N]<0xdc00||V.Value.StringValue[N]>0xdfff)return false;}else if(C>=0xdc00&&C<=0xdfff)return false;}if(FTCHARToUTF8(*V.Value.StringValue,V.Value.StringValue.Len()+1).Length()>96)return false;}else return false;}return true;
}
TArray<uint8> MarkerPrefix(const FString& Intent){const FString S=TEXT("horizon-marker:")+Intent+TEXT("\n");const FTCHARToUTF8 B(*S,S.Len()+1);TArray<uint8> Bytes;Bytes.Append(reinterpret_cast<const uint8*>(B.Get()),B.Length());return Bytes;}
bool Valid(const UHorizonSharedOriginal* P,const FString& Project,const FString& User)
{
    if(!P||P->Project!=Project||P->UserId!=User||!Key(P->Collection)||!Uuid(P->IntentId)||P->OriginalHash!=OriginalHash(P)||P->ExpectedRevision<0||P->ExpectedRevision>HorizonSharedContract::MaxSafe||P->SendSerial<0||P->SendSerial>=HorizonSharedContract::MaxSafe||(!P->GroupId.IsEmpty()&&!Uuid(P->GroupId))||!InputValid(P->Input))return false;
    if(P->Action==TEXT("create")){auto Prefix=MarkerPrefix(P->IntentId);if(!P->ObjectId.IsEmpty()||P->SendSerial>1||P->Input.Data.Num()<Prefix.Num()||FMemory::Memcmp(Prefix.GetData(),P->Input.Data.GetData(),Prefix.Num()))return false;return P->Confirmed?Uuid(P->ResolvedObjectId):P->ResolvedObjectId.IsEmpty();}
    return (P->Action==TEXT("update")||P->Action==TEXT("delete"))&&Uuid(P->ObjectId)&&!P->Confirmed&&P->ResolvedObjectId.IsEmpty();
}
bool Matches(const UHorizonSharedOriginal* P,const FHorizonSharedObject& O)
{
    if(O.OwnerId!=P->UserId||O.Collection!=P->Collection||O.Data!=P->Input.Data||O.Search.Num()!=P->Input.Search.Num()||O.bHasGroupId!=!P->GroupId.IsEmpty()||(O.bHasGroupId&&O.GroupId!=P->GroupId)||O.bHasPosition!=P->Input.bHasPosition||(O.bHasPosition&&(O.Position.X!=P->Input.Position.X||O.Position.Y!=P->Input.Position.Y)))return false;
    for(const auto& V:P->Input.Search){const auto* Found=O.Search.FindByPredicate([&](const FHorizonSharedSearchValue& X){return X.Key==V.Key;});if(!Found||Found->Value.Type!=V.Value.Type||(V.Value.Type==EHorizonSharedScalarType::Integer?Found->Value.IntegerValue!=V.Value.IntegerValue:(Found->Value.StringValue.Len()!=V.Value.StringValue.Len()||(V.Value.StringValue.Len()&&FMemory::Memcmp(*Found->Value.StringValue,*V.Value.StringValue,V.Value.StringValue.Len()*sizeof(TCHAR))))))return false;}return true;
}
}
UHorizonSubsystem* AHorizonSharedObjectsExample::Session() const{auto* GI=GetGameInstance();auto* S=GI?GI->GetSubsystem<UHorizonSubsystem>():nullptr;return S&&S->Auth&&S->Auth->IsSignedIn()&&S->SharedObjects?S:nullptr;}
bool AHorizonSharedObjectsExample::Fresh() const{auto* S=Session();return S&&SnapshotContext==S->GetHttpClient()->GetRequestContextEpoch()&&SnapshotGeneration==S->SharedObjects->GetCacheGeneration();}
FHorizonSharedQueryResult AHorizonSharedObjectsExample::GetObjects() const{return Fresh()?Objects:FHorizonSharedQueryResult{};}
FHorizonSharedUsage AHorizonSharedObjectsExample::GetUsage() const{return Fresh()&&FreshOwner?Usage:FHorizonSharedUsage{};}
bool AHorizonSharedObjectsExample::AllowRead(const FString& Key,bool Continuation,uint64 Context)
{
    if(Context!=SnapshotContext)RefreshAfter.Empty();const double Now=FPlatformTime::Seconds();for(auto It=RefreshAfter.CreateIterator();It;++It)if(It.Value()<=Now)It.RemoveCurrent();
    if(Continuation)return true;if(const double* Until=RefreshAfter.Find(Key))if(*Until>Now)return false;
    // The explicit sample keeps bounded refresh scopes; no queued or automatic retry is created.
    return RefreshAfter.Contains(Key)||RefreshAfter.Num()<256;
}
void AHorizonSharedObjectsExample::MarkRead(const FString& Key,int32 Poll){RefreshAfter.Add(Key,FPlatformTime::Seconds()+Poll);}
void AHorizonSharedObjectsExample::RefreshOwned(const FString& C,const FString& Group,const FString& Cursor)
{
    auto* S=Session();if(!S||Busy||!HorizonSharedExampleStorage::Key(C)||(!Group.IsEmpty()&&!HorizonSharedExampleStorage::Uuid(Group)))return;const uint64 Context=S->GetHttpClient()->GetRequestContextEpoch();const FString Key=TEXT("owner|")+C+TEXT("|")+Group;
    if(!AllowRead(Key,!Cursor.IsEmpty(),Context))return;
    const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonSharedExampleStorage::Project(S),Slot=HorizonSharedExampleStorage::Slot(P,U);auto* Original=HorizonSharedExampleStorage::Load(Slot);const FString Signature=HorizonSharedExampleStorage::Valid(Original,P,U)?HorizonSharedExampleStorage::Signature(Original):FString();
    Busy=true;FreshOwner=false;Objects={};Usage={};OriginalAtRefresh.Reset();
    S->SharedObjects->GetUsage(U,C,FOnSharedUsageComplete::CreateWeakLambda(this,[this,C,Group,Cursor,Key,Context,Signature](bool Ok,const FHorizonSharedUsage& V,const FString&,const FString&)
    {
        auto* Current=Session();if(!Ok||!Current||Current->GetHttpClient()->GetRequestContextEpoch()!=Context){Busy=false;return;}Usage=V;FHorizonSharedQuery Q;Q.Mode=EHorizonSharedQueryMode::Owner;Q.OwnerId=V.UserId;Q.GroupId=Group;Q.Limit=50;Q.Cursor=Cursor;
        Current->SharedObjects->QueryObjects(V.UserId,C,Q,FOnSharedQueryComplete::CreateWeakLambda(this,[this,C,Group,Key,Context,Signature](bool Good,const FHorizonSharedQueryResult& R,const FString&,const FString&)
        {
            Busy=false;auto* Now=Session();if(!Good||!Now||Now->GetHttpClient()->GetRequestContextEpoch()!=Context){Usage={};return;}Objects=R;SnapshotCollection=C;SnapshotGroup=Group;SnapshotContext=Context;SnapshotGeneration=Now->SharedObjects->GetCacheGeneration();RefreshKey=Key;MarkRead(Key,FMath::Max(Usage.PollAfterSeconds,R.PollAfterSeconds));OriginalAtRefresh=Signature;FreshOwner=true;
        }));
    }));
}
void AHorizonSharedObjectsExample::ReadQuery(const FString& C,const FHorizonSharedQuery& Q,const FString& Key)
{
    auto* S=Session();if(!S||Busy)return;const uint64 Context=S->GetHttpClient()->GetRequestContextEpoch();if(!AllowRead(Key,!Q.Cursor.IsEmpty(),Context))return;Busy=true;FreshOwner=false;Objects={};Usage={};OriginalAtRefresh.Reset();
    S->SharedObjects->QueryObjects(S->Auth->GetCurrentUser().UserId,C,Q,FOnSharedQueryComplete::CreateWeakLambda(this,[this,C,Key,Context](bool Ok,const FHorizonSharedQueryResult& R,const FString&,const FString&)
    {
        Busy=false;auto* Current=Session();if(!Ok||!Current||Current->GetHttpClient()->GetRequestContextEpoch()!=Context)return;Objects=R;SnapshotCollection=C;SnapshotContext=Context;SnapshotGeneration=Current->SharedObjects->GetCacheGeneration();RefreshKey=Key;MarkRead(Key,R.PollAfterSeconds);
    }));
}
void AHorizonSharedObjectsExample::QueryRectangle(const FString& C,int64 X,int64 X2,int64 Y,int64 Y2,const FString& Cursor)
{FHorizonSharedQuery Q;Q.Mode=EHorizonSharedQueryMode::Rectangle;Q.MinX=X;Q.MaxX=X2;Q.MinY=Y;Q.MaxY=Y2;Q.Cursor=Cursor;ReadQuery(C,Q,TEXT("rectangle|")+C+TEXT("|")+LexToString(X)+TEXT("|")+LexToString(X2)+TEXT("|")+LexToString(Y)+TEXT("|")+LexToString(Y2));}
void AHorizonSharedObjectsExample::QueryOpponents(const FString& C,const FString& Field,int64 Min,int64 Max)
{FHorizonSharedQuery Q;Q.Mode=EHorizonSharedQueryMode::Random;Q.Field=Field;Q.Min.IntegerValue=Min;Q.Max.IntegerValue=Max;ReadQuery(C,Q,TEXT("random|")+C+TEXT("|")+Field+TEXT("|")+LexToString(Min)+TEXT("|")+LexToString(Max));}
void AHorizonSharedObjectsExample::CreateMarker(const FHorizonSharedObjectInput& I){Start(TEXT("create"),TEXT(""),GetUsage().Revision,I);}
void AHorizonSharedObjectsExample::UpdateMarker(const FString& Id,int64 Revision,const FHorizonSharedObjectInput& I){Start(TEXT("update"),Id,Revision,I);}
void AHorizonSharedObjectsExample::DeleteMarker(const FString& Id,int64 Revision){Start(TEXT("delete"),Id,Revision,{});}
UHorizonSharedOriginal* AHorizonSharedObjectsExample::Prepare(const FString& A,const FString& Id,int64 Revision,const FHorizonSharedObjectInput& I)
{
    auto* S=Session();if(!S||Busy||!Fresh()||!FreshOwner||!Usage.bPresent)return nullptr;const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonSharedExampleStorage::Project(S),Slot=HorizonSharedExampleStorage::Slot(P,U);if(UGameplayStatics::DoesSaveGameExist(Slot,0))return nullptr;
    if(A==TEXT("create")){if(Revision!=Usage.Revision)return nullptr;}else {const auto* O=Objects.Objects.FindByPredicate([&](const FHorizonSharedObject& V){return V.Id==Id&&V.OwnerId==U&&V.Revision==Revision;});if(!O)return nullptr;}
    auto* Original=NewObject<UHorizonSharedOriginal>();Original->Project=P;Original->UserId=U;Original->Collection=SnapshotCollection;Original->GroupId=SnapshotGroup;Original->IntentId=FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);Original->Action=A;Original->ObjectId=Id;Original->ExpectedRevision=Revision;Original->Input=I;
    if(A==TEXT("create")){auto Prefix=HorizonSharedExampleStorage::MarkerPrefix(Original->IntentId);Prefix.Append(I.Data);Original->Input.Data=MoveTemp(Prefix);}Original->OriginalHash=HorizonSharedExampleStorage::OriginalHash(Original);
    if(!HorizonSharedExampleStorage::Valid(Original,P,U)||!HorizonSharedExampleStorage::Save(Original,Slot))return nullptr;return Original;
}
void AHorizonSharedObjectsExample::Start(const FString& A,const FString& Id,int64 Revision,const FHorizonSharedObjectInput& Input)
{
    auto* Original=Prepare(A,Id,Revision,Input);if(Original)Send(Original,HorizonSharedExampleStorage::Slot(Original->Project,Original->UserId));
}

void AHorizonSharedObjectsExample::Send(UHorizonSharedOriginal* P,const FString& Slot)
{
    auto* S=Session();if(!S||Busy||!HorizonSharedExampleStorage::Valid(P,HorizonSharedExampleStorage::Project(S),S->Auth->GetCurrentUser().UserId)||(P->Action==TEXT("create")&&P->SendSerial!=0))return;++P->SendSerial;if(!HorizonSharedExampleStorage::Save(P,Slot))return;Busy=true;FreshOwner=false;RefreshAfter.Remove(TEXT("owner|")+P->Collection+TEXT("|")+P->GroupId);TStrongObjectPtr<UHorizonSharedOriginal> Original(HorizonSharedExampleStorage::Copy(P));const uint64 C=S->GetHttpClient()->GetRequestContextEpoch();
    const auto Complete=[this,Slot,Original,C](bool Ok,const FString& Resolved,const FString& Code)
    {
        Busy=false;auto* Current=Session();if(!Ok){UE_LOG(LogTemp,Warning,TEXT("[SharedObjectsExample] Original retained; reconcile explicitly: %s"),*Code);return;}if(!Current||Current->GetHttpClient()->GetRequestContextEpoch()!=C)return;auto* Saved=HorizonSharedExampleStorage::Load(Slot);if(!Saved||HorizonSharedExampleStorage::Signature(Saved)!=HorizonSharedExampleStorage::Signature(Original.Get()))return;
        if(Saved->Action==TEXT("create")){Saved->Confirmed=true;Saved->ResolvedObjectId=Resolved;if(!HorizonSharedExampleStorage::Save(Saved,Slot))return;}UGameplayStatics::DeleteGameInSlot(Slot,0);
    };
    if(P->Action==TEXT("delete"))S->SharedObjects->DeleteObject(P->UserId,P->Collection,P->ObjectId,P->ExpectedRevision,FOnSharedDeleteComplete::CreateWeakLambda(this,[Complete](bool Ok,const FHorizonSharedDeleteResult& R,const FString& Code,const FString&){Complete(Ok,R.ObjectId,Code);}));
    else {const auto D=FOnSharedWriteComplete::CreateWeakLambda(this,[Complete](bool Ok,const FHorizonSharedWriteResult& R,const FString& Code,const FString&){Complete(Ok,R.Object.Id,Code);});if(P->Action==TEXT("create"))S->SharedObjects->CreateObject(P->UserId,P->Collection,P->ExpectedRevision,P->Input,D,P->GroupId);else S->SharedObjects->UpdateObject(P->UserId,P->Collection,P->ObjectId,P->ExpectedRevision,P->Input,D);}
}
void AHorizonSharedObjectsExample::RetryOriginal()
{
    auto* S=Session();if(!S||Busy)return;const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonSharedExampleStorage::Project(S),Slot=HorizonSharedExampleStorage::Slot(P,U);auto* Original=HorizonSharedExampleStorage::Load(Slot);if(!HorizonSharedExampleStorage::Valid(Original,P,U))return;
    if(Original->Action==TEXT("create")){if(Original->SendSerial==0)Send(Original,Slot);else RefreshOwned(Original->Collection,Original->GroupId);return;}Send(Original,Slot);
}
bool AHorizonSharedObjectsExample::ConfirmRecoveredCreate(const FString& Id)
{
    auto* S=Session();if(!S||Busy||!Fresh()||!FreshOwner)return false;const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonSharedExampleStorage::Project(S),Slot=HorizonSharedExampleStorage::Slot(P,U);auto* Original=HorizonSharedExampleStorage::Load(Slot);if(!HorizonSharedExampleStorage::Valid(Original,P,U)||Original->Action!=TEXT("create")||Original->SendSerial!=1||OriginalAtRefresh!=HorizonSharedExampleStorage::Signature(Original))return false;
    const auto* Match=Objects.Objects.FindByPredicate([&](const FHorizonSharedObject& O){return O.Id==Id&&HorizonSharedExampleStorage::Matches(Original,O);});if(!Match||Objects.Objects.FilterByPredicate([&](const FHorizonSharedObject& O){return HorizonSharedExampleStorage::Matches(Original,O);}).Num()!=1)return false;
    Original->Confirmed=true;Original->ResolvedObjectId=Id;if(!HorizonSharedExampleStorage::Save(Original,Slot))return false;return UGameplayStatics::DeleteGameInSlot(Slot,0);
}
bool AHorizonSharedObjectsExample::AcknowledgeAfterRefresh()
{
    auto* S=Session();if(!S||Busy||!Fresh()||!FreshOwner)return false;const FString U=S->Auth->GetCurrentUser().UserId,P=HorizonSharedExampleStorage::Project(S),Slot=HorizonSharedExampleStorage::Slot(P,U);auto* Original=HorizonSharedExampleStorage::Load(Slot);if(!HorizonSharedExampleStorage::Valid(Original,P,U)||OriginalAtRefresh!=HorizonSharedExampleStorage::Signature(Original)||(Original->Action==TEXT("create")&&!Original->Confirmed)||Original->Collection!=SnapshotCollection||Original->GroupId!=SnapshotGroup)return false;
    if(Objects.bHasNextCursor&&!Objects.Objects.ContainsByPredicate([&](const FHorizonSharedObject& O){return O.Id==Original->ObjectId||O.Id==Original->ResolvedObjectId;}))return false;FreshOwner=false;return UGameplayStatics::DeleteGameInSlot(Slot,0);
}
