#pragma once
#include "Models/HorizonMarketplace.h"
#include "Models/HorizonItemsWire.h"
#include "Transport/HorizonMarketplaceTransportContract.h"
namespace HorizonMarketplaceWire
{
inline std::string Utf8(const FString& S){const FTCHARToUTF8 B(*S,S.Len());return std::string(B.Get(),B.Length());}
inline bool Id(const FString& S){FGuid G;return S.Len()==36&&FGuid::ParseExact(S,EGuidFormats::DigitsWithHyphens,G)&&G.ToString(EGuidFormats::DigitsWithHyphens).ToLower().Equals(S,ESearchCase::CaseSensitive);}
inline bool Choice(const FString& S,std::initializer_list<const TCHAR*> Values){for(const TCHAR* V:Values)if(S.Equals(V,ESearchCase::CaseSensitive))return true;return false;}
inline bool Key(const FString& S){return HorizonItemsContract::Key(Utf8(S));}
inline bool Currency(const FString& S){return HorizonMarketplaceContract::Currency(Utf8(S));}
inline bool Parse(const FString& Body,TSharedPtr<FJsonObject>& Out)
{
    const FString Prefix=TEXT("horizon-market-")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT("-");std::vector<std::string> Raw;std::string Masked;
    if(!HorizonMarketplaceContract::Mask(Utf8(Body),Utf8(Prefix),Raw,Masked))return false;
    TSharedPtr<FJsonObject> J;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UTF8_TO_TCHAR(Masked.c_str())),J,FJsonSerializer::EFlags::StoreNumbersAsStrings)||!J)return false;
    int32 Count=0;if(!HorizonItemsWire::Restore(MakeShared<FJsonValueObject>(J),Prefix,Raw,Count)||size_t(Count)!=Raw.size())return false;Out=MoveTemp(J);return true;
}
inline bool Text(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::String&&V->TryGetString(S);}
inline bool ReadId(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){return Text(J,K,S)&&Id(S);}
inline bool Number(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int64& N,int64 Min=0,int64 Max=HorizonItemsContract::MaxSafe)
{auto V=J?J->TryGetField(K):nullptr;FString S;long long Parsed;if(!V||V->Type!=EJson::Number||!V->TryGetString(S)||!HorizonItemsContract::Integer(Utf8(S),Parsed)||Parsed<Min||Parsed>Max)return false;N=Parsed;return true;}
inline bool Small(const TSharedPtr<FJsonObject>& J,const TCHAR* K,int32& N,int32 Min=1,int32 Max=MAX_int32){int64 V;if(!Number(J,K,V,Min,Max))return false;N=int32(V);return true;}
inline bool Bool(const TSharedPtr<FJsonObject>& J,const TCHAR* K,bool& B){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::Boolean&&V->TryGetBool(B);}
inline int64 Millis(const FString& S)
{
    FDateTime T;if(S.Len()!=24||S[10]!='T'||S[23]!='Z'||!FDateTime::ParseIso8601(*S,T)||T.ToIso8601()!=S)return -1;return T.ToUnixTimestamp()*1000+T.GetMillisecond();
}
inline bool Time(const TSharedPtr<FJsonObject>& J,const TCHAR* K,FString& S){return Text(J,K,S)&&Millis(S)>=0;}
inline bool Null(const TSharedPtr<FJsonObject>& J,const TCHAR* K){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::Null;}
inline TSharedPtr<FJsonObject> Object(const TSharedPtr<FJsonObject>& J,const TCHAR* K){auto V=J?J->TryGetField(K):nullptr;return V&&V->Type==EJson::Object?V->AsObject():nullptr;}
inline void AddNumber(const TSharedRef<FJsonObject>& J,const TCHAR* K,int64 N){J->SetField(K,MakeShared<FJsonValueNumberString>(LexToString(N)));}
inline FString Field(const FHorizonMarketplaceIntent& Intent,const TCHAR* Key){TSharedPtr<FJsonObject> J;FString S;return Parse(Intent.Json,J)&&Text(J,Key,S)?S:FString();}
inline bool Cursor(const FString& S){if(S.IsEmpty()||S.Len()>512)return false;for(TCHAR C:S)if(!FChar::IsAlnum(C)||C>127){if(C!='_'&&C!='-')return false;}return true;}
}
