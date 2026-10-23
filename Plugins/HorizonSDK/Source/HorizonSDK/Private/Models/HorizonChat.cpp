// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Models/HorizonChat.h"
#include "Models/HorizonChatWire.h"
namespace HorizonChatJson
{
bool Keys(const TSharedPtr<FJsonObject>& J,std::initializer_list<const TCHAR*> Names)
{
    if(!J||J->Values.Num()!=int32(Names.size()))return false;for(auto K:Names)if(!J->HasField(K))return false;return true;
}
bool String(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S,int32 Max=2048){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::String&&V->TryGetString(S)&&S.Len()<=Max;}
bool Id(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){return String(J,K,S,36)&&HorizonChatWire::Id(S);}
bool Time(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){return String(J,K,S,24)&&HorizonChatWire::Time(S);}
bool NullableTime(const TSharedPtr<FJsonObject>& J,const TCHAR* K,bool& Has,FString& S){auto V=J?J->TryGetField(K):nullptr;if(!V)return false;Has=!V->IsNull();return !Has||Time(J,K,S);}
bool Number(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int64& N){auto V=J?J->TryGetField(K):nullptr;FString S;if(!V||V->Type!=EJson::Number||!V->TryGetString(S))return false;long long Value;if(!HorizonItemsContract::Integer(HorizonChatWire::Utf8(S),Value)||Value<0)return false;N=Value;return true;}
bool Small(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int32& N,int32 Max){int64 V;if(!Number(J,K,V)||V>Max)return false;N=int32(V);return true;}
bool Bool(const TSharedPtr<FJsonObject>& J,const TCHAR* K,bool& B){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::Boolean&&V->TryGetBool(B);}
bool ReadModeration(const TSharedPtr<FJsonObject>& J,FHorizonChatModeration& M){return Keys(J,{TEXT("muteUntil"),TEXT("banUntil")})&&NullableTime(J,TEXT("muteUntil"),M.bHasMuteUntil,M.MuteUntil)&&NullableTime(J,TEXT("banUntil"),M.bHasBanUntil,M.BanUntil);}
bool Ids(const TSharedPtr<FJsonObject>& J,const TCHAR* K,TArray<FString>& Out,int32 Max)
{
    const TArray<TSharedPtr<FJsonValue>>* A;if(!J||!J->TryGetArrayField(K,A)||A->Num()>Max)return false;TSet<FString> Seen;
    for(const auto& V:*A){FString S;if(!V||V->Type!=EJson::String||!V->TryGetString(S)||!HorizonChatWire::Id(S)||Seen.Contains(S))return false;Seen.Add(S);Out.Add(S);}return true;
}
bool ReadChannel(const TSharedPtr<FJsonObject>& J,FHorizonChatChannel& C)
{
    if(!Keys(J,{TEXT("id"),TEXT("epoch"),TEXT("kind"),TEXT("peerIds"),TEXT("groupId"),TEXT("revision"),TEXT("expiresAt")})||!Id(J,TEXT("id"),C.Id)||!Id(J,TEXT("epoch"),C.Epoch)||!String(J,TEXT("kind"),C.Kind,7)||!Ids(J,TEXT("peerIds"),C.PeerIds,2)||!Number(J,TEXT("revision"),C.Revision)||!Time(J,TEXT("expiresAt"),C.ExpiresAt))return false;
    const auto G=J->TryGetField(TEXT("groupId"));C.bHasGroupId=!G->IsNull();if(C.bHasGroupId&&!Id(J,TEXT("groupId"),C.GroupId))return false;
    return C.Kind==TEXT("direct")?C.PeerIds.Num()==2&&C.PeerIds[0]<C.PeerIds[1]&&!C.bHasGroupId:C.Kind==TEXT("group")?C.PeerIds.IsEmpty()&&C.bHasGroupId:C.Kind==TEXT("project")&&C.PeerIds.IsEmpty()&&!C.bHasGroupId;
}
bool ReadMessage(const TSharedPtr<FJsonObject>& J,FHorizonChatMessage& M)
{
    return Keys(J,{TEXT("id"),TEXT("channelId"),TEXT("authorId"),TEXT("text"),TEXT("createdAt"),TEXT("expiresAt")})&&Id(J,TEXT("id"),M.Id)&&Id(J,TEXT("channelId"),M.ChannelId)&&Id(J,TEXT("authorId"),M.AuthorId)&&String(J,TEXT("text"),M.Text,2000)&&HorizonChatWire::Text(M.Text)&&Time(J,TEXT("createdAt"),M.CreatedAt)&&Time(J,TEXT("expiresAt"),M.ExpiresAt)&&M.CreatedAt<M.ExpiresAt;
}
bool ReadReceipt(const TSharedPtr<FJsonObject>& J,FHorizonChatReceipt& R)
{
    return Keys(J,{TEXT("operationId"),TEXT("issuedAt"),TEXT("channelId"),TEXT("messageId"),TEXT("createdAt"),TEXT("expiresAt")})&&Id(J,TEXT("operationId"),R.OperationId)&&Time(J,TEXT("issuedAt"),R.IssuedAt)&&Id(J,TEXT("channelId"),R.ChannelId)&&Id(J,TEXT("messageId"),R.MessageId)&&Time(J,TEXT("createdAt"),R.CreatedAt)&&Time(J,TEXT("expiresAt"),R.ExpiresAt)&&R.CreatedAt<R.ExpiresAt;
}
}
bool FHorizonChatLimitsView::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonChatLimitsView& Out)
{
    using namespace HorizonChatJson;FHorizonChatLimitsView V;const TSharedPtr<FJsonObject> *L,*C,*M;
    if(!Keys(J,{TEXT("limits"),TEXT("capabilities"),TEXT("moderation"),TEXT("serverTime")})||!J->TryGetObjectField(TEXT("limits"),L)||!J->TryGetObjectField(TEXT("capabilities"),C)||!J->TryGetObjectField(TEXT("moderation"),M)||!Time(J,TEXT("serverTime"),V.ServerTime)||!ReadModeration(*M,V.Moderation)||!Keys(*C,{TEXT("chat"),TEXT("realtime"),TEXT("incrementalSync"),TEXT("localHistory")})||!Bool(*C,TEXT("chat"),V.Capabilities.bChat)||!Bool(*C,TEXT("realtime"),V.Capabilities.bRealtime)||!Bool(*C,TEXT("incrementalSync"),V.Capabilities.bIncrementalSync)||!V.Capabilities.bIncrementalSync||!Bool(*C,TEXT("localHistory"),V.Capabilities.bLocalHistory)||!V.Capabilities.bLocalHistory)return false;
    if(!Small(*L,TEXT("retainedMessagesPerPlayer"),V.Limits.RetainedMessagesPerPlayer,0))return false;
    if(!Small(*L,TEXT("retainedMessagesPerChannel"),V.Limits.RetainedMessagesPerChannel,20)||V.Limits.RetainedMessagesPerChannel!=20)return false;
    if(!Small(*L,TEXT("retentionSeconds"),V.Limits.RetentionSeconds,604800)||V.Limits.RetentionSeconds!=604800)return false;
    if(!Small(*L,TEXT("privateRetainedMessagesPerChannel"),V.Limits.PrivateRetainedMessagesPerChannel,15)||V.Limits.PrivateRetainedMessagesPerChannel!=15)return false;
    if(!Small(*L,TEXT("privateRetentionSeconds"),V.Limits.PrivateRetentionSeconds,30)||V.Limits.PrivateRetentionSeconds!=30)return false;
    if(!Small(*L,TEXT("operationWindowSeconds"),V.Limits.OperationWindowSeconds,30)||V.Limits.OperationWindowSeconds!=30)return false;
    if(!Small(*L,TEXT("operationFutureToleranceSeconds"),V.Limits.OperationFutureToleranceSeconds,60)||V.Limits.OperationFutureToleranceSeconds!=60)return false;
    if(!Small(*L,TEXT("messageCharacters"),V.Limits.MessageCharacters,500))return false;
    if(!Small(*L,TEXT("messageUtf8Bytes"),V.Limits.MessageUtf8Bytes,2000))return false;
    if(!Small(*L,TEXT("channelsPerAccount"),V.Limits.ChannelsPerAccount,MAX_int32))return false;
    if(!Small(*L,TEXT("sendsPerAccountMinute"),V.Limits.SendsPerAccountMinute,MAX_int32))return false;
    if(!Small(*L,TEXT("playerSendIntervalMillis"),V.Limits.PlayerSendIntervalMillis,3000))return false;
    if(!Small(*L,TEXT("channelSendIntervalMillis"),V.Limits.ChannelSendIntervalMillis,2000))return false;
    if(!Small(*L,TEXT("reportsPerProject"),V.Limits.ReportsPerProject,MAX_int32))return false;
    if(!Small(*L,TEXT("receiptCount"),V.Limits.ReceiptCount,0))return false;
    if(!Small(*L,TEXT("reportReceiptCount"),V.Limits.ReportReceiptCount,0))return false;
    if(!Small(*L,TEXT("wordFilterTerms"),V.Limits.WordFilterTerms,100))return false;

    if(!Keys(*L,{TEXT("retainedMessagesPerPlayer"),TEXT("retainedMessagesPerChannel"),TEXT("retentionSeconds"),TEXT("privateRetainedMessagesPerChannel"),TEXT("privateRetentionSeconds"),TEXT("operationWindowSeconds"),TEXT("operationFutureToleranceSeconds"),TEXT("messageCharacters"),TEXT("messageUtf8Bytes"),TEXT("channelsPerAccount"),TEXT("sendsPerAccountMinute"),TEXT("playerSendIntervalMillis"),TEXT("channelSendIntervalMillis"),TEXT("reportsPerProject"),TEXT("receiptCount"),TEXT("reportReceiptCount"),TEXT("wordFilterTerms")}))return false;V.bPresent=true;Out=MoveTemp(V);return true;
}
bool FHorizonChatOpened::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonChatOpened& Out)
{
    using namespace HorizonChatJson;FHorizonChatOpened V;const TSharedPtr<FJsonObject>* C;if(!Keys(J,{TEXT("channel"),TEXT("serverTime")})||!J->TryGetObjectField(TEXT("channel"),C)||!ReadChannel(*C,V.Channel)||!Time(J,TEXT("serverTime"),V.ServerTime))return false;V.bPresent=true;Out=MoveTemp(V);return true;
}
bool FHorizonChatSynced::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonChatSynced& Out)
{
    using namespace HorizonChatJson;FHorizonChatSynced V;const TSharedPtr<FJsonObject> *C,*M;const TArray<TSharedPtr<FJsonValue>>* A;
    if(!Keys(J,{TEXT("channel"),TEXT("visibleMessageIds"),TEXT("messages"),TEXT("moderation"),TEXT("serverTime")})||!J->TryGetObjectField(TEXT("channel"),C)||!ReadChannel(*C,V.Channel)||!Ids(J,TEXT("visibleMessageIds"),V.VisibleMessageIds,V.Channel.Kind==TEXT("project")?20:15)||!J->TryGetArrayField(TEXT("messages"),A)||A->Num()>(V.Channel.Kind==TEXT("project")?20:15)||!J->TryGetObjectField(TEXT("moderation"),M)||!ReadModeration(*M,V.Moderation)||!Time(J,TEXT("serverTime"),V.ServerTime))return false;
    TSet<FString> Seen;int32 Last=-1;for(const auto& Row:*A){FHorizonChatMessage R;if(!Row||Row->Type!=EJson::Object||!ReadMessage(Row->AsObject(),R)||R.ChannelId!=V.Channel.Id||Seen.Contains(R.Id))return false;const int32 Index=V.VisibleMessageIds.IndexOfByKey(R.Id);if(Index<=Last)return false;FDateTime Created,Expires;FDateTime::ParseIso8601(*R.CreatedAt,Created);FDateTime::ParseIso8601(*R.ExpiresAt,Expires);if((Expires-Created).GetTotalSeconds()>(V.Channel.Kind==TEXT("project")?604800:30))return false;Last=Index;Seen.Add(R.Id);V.Messages.Add(R);}V.bPresent=true;Out=MoveTemp(V);return true;
}
bool FHorizonChatSent::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonChatSent& Out){using namespace HorizonChatJson;FHorizonChatSent V;const TSharedPtr<FJsonObject>* R;if(!Keys(J,{TEXT("receipt"),TEXT("replayed"),TEXT("serverTime")})||!J->TryGetObjectField(TEXT("receipt"),R)||!ReadReceipt(*R,V.Receipt)||!Bool(J,TEXT("replayed"),V.bReplayed)||!Time(J,TEXT("serverTime"),V.ServerTime))return false;V.bPresent=true;Out=MoveTemp(V);return true;}
bool FHorizonChatOperation::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonChatOperation& Out){using namespace HorizonChatJson;FHorizonChatOperation V;const TSharedPtr<FJsonObject>* R;if(!Keys(J,{TEXT("receipt"),TEXT("serverTime")})||!J->TryGetObjectField(TEXT("receipt"),R)||!ReadReceipt(*R,V.Receipt)||!Time(J,TEXT("serverTime"),V.ServerTime))return false;V.bPresent=true;Out=MoveTemp(V);return true;}
bool FHorizonChatReported::TryFromJson(const TSharedPtr<FJsonObject>& J,FHorizonChatReported& Out){using namespace HorizonChatJson;FHorizonChatReported V;if(!Keys(J,{TEXT("reportId"),TEXT("status"),TEXT("replayed"),TEXT("serverTime")})||!Id(J,TEXT("reportId"),V.ReportId)||!String(J,TEXT("status"),V.Status,9)||(V.Status!=TEXT("open")&&V.Status!=TEXT("dismissed")&&V.Status!=TEXT("actioned")&&V.Status!=TEXT("removed"))||!Bool(J,TEXT("replayed"),V.bReplayed)||!Time(J,TEXT("serverTime"),V.ServerTime))return false;V.bPresent=true;Out=MoveTemp(V);return true;}
