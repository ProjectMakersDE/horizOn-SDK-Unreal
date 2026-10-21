#include "Transport/HorizonGiftCodeTransportContract.h"
// Runs actual manager methods, with only the UE and network/storage boundaries replaced.
#include "Transport/HorizonLeaderboardTransportContract.h"
#include "Transport/HorizonCloudSaveTransportContract.h"
#include "Transport/HorizonRetryContract.h"

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#define TEXT(Value) Value
#define UE_LOG(...) ((void)0)
#define TCHAR_TO_UTF8(Value) Value
#define UTF8_TO_TCHAR(Value) Value
enum class ESPMode { ThreadSafe };
using uint8 = unsigned char;
using int32 = int;
using int64 = long long;
using uint64 = unsigned long long;
using TCHAR = char;
using ANSICHAR = char;
struct FString : std::string
{
    using std::string::string;
    FString(const std::string& Value) : std::string(Value) {}
    FString(int Length, const char* Value) : std::string(Value, Length) {}
    bool IsEmpty() const { return empty(); }
    int Len() const { return size(); }
    void Empty() { clear(); }
    const char* operator*() const { return c_str(); }
    FString operator/(const FString& Other) const { return *this + "/" + Other; }
    template <typename... Args> static FString Printf(const char* Format, Args... Values)
    { char Buffer[512]; std::snprintf(Buffer, sizeof(Buffer), Format, Values...); return Buffer; }
};
template <typename T> struct TArray : std::vector<T>
{ using std::vector<T>::vector; int Num() const { return this->size(); }
  void Add(const T& Value) { this->push_back(Value); }
  const T* GetData() const { return this->data(); }
  void Append(const T* Data, int Length) { this->insert(this->end(), Data, Data + Length); } };
template <typename K, typename V> struct TMap
{
    struct FPair { K Key; V Value; };
    std::vector<FPair> Pairs;
    void Add(const K& Key, const V& Value) { Pairs.push_back({Key, Value}); }
    typename std::vector<FPair>::const_iterator begin() const { return Pairs.begin(); }
    typename std::vector<FPair>::const_iterator end() const { return Pairs.end(); }
};
template <typename T, ESPMode Mode = ESPMode::ThreadSafe> struct TSharedPtr : std::shared_ptr<T>
{
    using std::shared_ptr<T>::shared_ptr;
    TSharedPtr(const std::shared_ptr<T>& Value) : std::shared_ptr<T>(Value) {}
    bool IsValid() const { return this->get() != nullptr; }
};
template <typename T, ESPMode Mode = ESPMode::ThreadSafe> using TSharedRef = TSharedPtr<T, Mode>;
template <typename T> TSharedRef<T> MakeShared() { return std::make_shared<T>(); }
template <typename T> T* NewObject() { static T Value; Value = T(); return &Value; }
template <typename T> struct TWeakObjectPtr
{
    T* Value;
    TWeakObjectPtr(T* In) : Value(In) {}
    T* Get() const { return Value; }
    bool IsValid() const { return Value != nullptr; }
};
enum class EJson { None, Null, String, Number, Boolean, Array, Object };
struct FJsonObject;
struct FJsonValue
{
    EJson Type = EJson::None;
    bool Bool = false;
    FString String;
    TSharedPtr<FJsonObject> Object;
    bool AsBool() const { return Bool; }
    FString AsString() const { return String; }
    TSharedPtr<FJsonObject> AsObject() const { return Object; }
};
struct FJsonObject
{
    std::map<FString, FString> Strings;
    std::map<FString, bool> Bools;
    std::map<FString, double> Numbers;
    std::map<FString, TArray<TSharedPtr<FJsonValue>>> Arrays;
    bool TryGetArrayField(const FString& Key, const TArray<TSharedPtr<FJsonValue>>*& Out) const
    { auto It = Arrays.find(Key); if (It == Arrays.end()) return false; Out = &It->second; return true; }
    void SetStringField(const FString& Key, const FString& Value) { Strings[Key] = Value; }
    void SetBoolField(const FString& Key, bool Value) { Bools[Key] = Value; }
    void SetNumberField(const FString& Key, double Value) { Numbers[Key] = Value; }
    // Like UE 5.5, the typed getters convert: a number or "true" counts as a bool, a number as a string.
    bool TryGetBoolField(const FString& Key, bool& Out) const
    {
        if (auto It = Bools.find(Key); It != Bools.end()) { Out = It->second; return true; }
        if (auto It = Numbers.find(Key); It != Numbers.end()) { Out = It->second != 0.0; return true; }
        if (auto It = Strings.find(Key); It != Strings.end()) { Out = It->second == "true"; return true; }
        return false;
    }
    TSharedPtr<FJsonValue> TryGetField(const FString& Key) const
    {
        auto Value = std::make_shared<FJsonValue>();
        if (auto It = Bools.find(Key); It != Bools.end()) { Value->Type = EJson::Boolean; Value->Bool = It->second; }
        else if (auto It = Strings.find(Key); It != Strings.end()) { Value->Type = EJson::String; Value->String = It->second; }
        else if (auto It = Numbers.find(Key); It != Numbers.end()) { Value->Type = EJson::Number; Value->String = std::to_string(It->second); }
        else return TSharedPtr<FJsonValue>();
        return Value;
    }
    bool TryGetNumberField(const FString& Key, double& Out) const
    { auto It = Numbers.find(Key); if (It == Numbers.end()) return false; Out = It->second; return true; }
    FString GetStringField(const FString& Key) const
    { auto It = Strings.find(Key); return It == Strings.end() ? FString() : It->second; }
    bool TryGetStringField(const FString& Key, FString& Out) const
    {
        if (auto It = Strings.find(Key); It != Strings.end()) { Out = It->second; return true; }
        if (auto It = Numbers.find(Key); It != Numbers.end()) { Out = std::to_string(It->second); return true; }
        return false;
    }
    bool GetBoolField(const FString& Key) const
    { auto It = Bools.find(Key); return It != Bools.end() && It->second; }
    bool HasField(const FString& Key) const { return Strings.count(Key) || Bools.count(Key) || Numbers.count(Key); }
};
template <typename T = void> struct TJsonWriter { FString* Output; };
template <typename T = void> struct TJsonReader { FString Input; };
template <typename T = void> struct TJsonReaderFactory
{
    static TSharedRef<TJsonReader<T>> Create(const FString& Input)
    { auto Reader = MakeShared<TJsonReader<T>>(); Reader->Input = Input; return Reader; }
};
template <typename T = void> struct TJsonWriterFactory
{
    static TSharedRef<TJsonWriter<T>> Create(FString* Output)
    { auto Writer = MakeShared<TJsonWriter<T>>(); Writer->Output = Output; return Writer; }
};
struct FJsonSerializer
{
    static inline TSharedRef<FJsonObject> SerializedBody;
    // UE's JSON parser is a boundary: these fixtures assert which original body
    // the production manager passes to it, not Unreal's JSON implementation.
    static inline std::map<FString, TArray<TSharedPtr<FJsonValue>>> ArrayBodies;
    static bool Deserialize(const TSharedRef<TJsonReader<>>& Reader, TArray<TSharedPtr<FJsonValue>>& Out)
    { auto It = ArrayBodies.find(Reader->Input); if (It == ArrayBodies.end()) return false; Out = It->second; return true; }
    static void Serialize(const TSharedRef<FJsonObject>& Body, const TSharedRef<TJsonWriter<>>& Writer)
    {
        SerializedBody = Body;
        *Writer->Output = "{\"userId\":\"" + Body->GetStringField("userId") + "\"}";
    }
};
struct FTCHARToUTF8
{
    const char* Data;
    FTCHARToUTF8(const char* Value) : Data(Value) {}
    const char* Get() const { return Data; }
    int Length() const { return std::strlen(Data); }
};
struct IHttpRequest
{
    FString Verb;
    std::map<FString, FString> Headers;
    void SetHeader(const FString& Key, const FString& Value) { Headers[Key] = Value; }
    FString GetVerb() const { return Verb; }
};
struct FUTF8ToTCHAR
{
    const char* Data;
    int Size;
    FUTF8ToTCHAR(const char* Value, int Length) : Data(Value), Size(Length) {}
    const char* Get() const { return Data; }
    int Length() const { return Size; }
};
struct FMath
{ template <typename T> static T Clamp(T Value, T Minimum, T Maximum)
  { return Value < Minimum ? Minimum : Value > Maximum ? Maximum : Value; } };
struct FPlatformTime { static inline double Now = 100.0; static double Seconds() { return Now; } };
struct FHorizonNetworkResponse
{
    bool bSuccess = true;
    int StatusCode = 200;
    FString ErrorMessage;
    FString RawJsonBody;
    TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
    TArray<uint8> BinaryData;
    TArray<FString> Headers;
    FString GetHeader(const FString& Name) const;
};
enum class EHorizonCloudSaveWriteStatus { Saved, Conflict, Failed };
struct FHorizonCloudSaveSnapshot
{
    bool bSuccess = false, bFound = false;
    FString Data;
    TArray<uint8> BinaryData;
    int64 Revision = -1;
    FString ErrorMessage;
    int StatusCode = 0;
    bool HasRevision() const { return Revision >= 0; }
};
struct FHorizonCloudSaveWriteResult
{
    EHorizonCloudSaveWriteStatus Status = EHorizonCloudSaveWriteStatus::Failed;
    int64 Revision = -1, DataSizeBytes = 0;
    FString ErrorMessage;
    int StatusCode = 0;
};
template <typename... Args> struct Delegate
{
    std::function<void(Args...)> Function;
    void ExecuteIfBound(Args... Values) const { if (Function) Function(Values...); }
    template <typename F> static Delegate CreateLambda(F Function) { return {Function}; }
    template <typename T, typename Extra>
    static Delegate CreateUObject(T* Object, void (T::*Method)(Args..., Extra), Extra Value)
    { return CreateLambda([=](Args... Arguments) { (Object->*Method)(Arguments..., Value); }); }
};
using FOnAuthComplete = Delegate<bool>;
using FOnBinaryComplete = Delegate<bool, const TArray<uint8>&>;
using FOnRequestComplete = Delegate<bool, const FString&>;
using FOnHttpResponse = Delegate<const FHorizonNetworkResponse&>;
using FOnCloudSaveSnapshotComplete = Delegate<const FHorizonCloudSaveSnapshot&>;
using FOnCloudSaveWriteComplete = Delegate<const FHorizonCloudSaveWriteResult&>;
struct FHorizonNewsEntry
{
    FString Id, Title, Message, ReleaseDate, LanguageCode;
    static FHorizonNewsEntry FromJson(const TSharedPtr<FJsonObject>&);
};
using FOnNewsComplete = Delegate<bool, const TArray<FHorizonNewsEntry>&>;
struct FGuid { static FGuid NewGuid() { return {}; } FString ToString(int) const { return "client-generated-token"; } };
struct EGuidFormats { static constexpr int DigitsLower = 0; };
enum class EHorizonAuthType { Anonymous, Email, Google, Apple };
struct FHorizonUserData
{
    FString UserId, AccessToken, AnonymousToken, DisplayName, Email, AppleUserId;
    bool bIsAnonymous = false, bIsEmailVerified = false, bIsPrivateRelayEmail = false;
    EHorizonAuthType AuthType = EHorizonAuthType::Anonymous;
    void UpdateFromAuthResponse(const TSharedPtr<FJsonObject>& JsonObject);
};
struct FOnAnonymousIdentityIssued
{
    std::function<bool(const FString&,const FHorizonUserData&)> Function;
    bool IsBound() const { return bool(Function); }
    bool Execute(const FString& P,const FHorizonUserData& U) const { return Function(P,U); }
};
struct UHorizonSessionSave
{
    FString CachedUserId, CachedAccessToken, CachedAnonymousToken, CachedDisplayName;
    bool bIsAnonymous = false;
    static inline UHorizonSessionSave* Stored = nullptr;
    bool SaveToDisk() { Stored = this; return true; }
    static UHorizonSessionSave* LoadFromDisk() { return Stored; }
};
struct UHorizonHttpClient
{
    struct Request { FString Method, Endpoint; TSharedRef<FJsonObject> Body; bool UseSession; FString ContentType, Payload; bool RetryAllowed = true; };
    std::vector<Request> Requests;
    std::function<FHorizonNetworkResponse(const Request&)> Server;
    FString SessionToken, ApiKey = "project-key-892", ActiveHost = "http://loopback.invalid";
    std::map<FString, FString> LastHeaders;
    FString GetApiKey() const { return ApiKey; }
    uint64 GetRequestContextEpoch() const { return 0; }
    void SetSessionToken(const FString& Value) { SessionToken = Value; }
    void ClearSessionToken() { SessionToken.Empty(); }
    void Send(const Request& Request, FOnHttpResponse Callback)
    { Requests.push_back(Request); Callback.ExecuteIfBound(Server(Request)); }
    void PostJson(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool Session, FOnHttpResponse Callback,
                  const TMap<FString, FString>& ExtraHeaders = TMap<FString, FString>());
    void PostJsonForBinary(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool Session, FOnHttpResponse Callback);
    void PostBinary(const FString& Endpoint, const TArray<uint8>& Data, bool Session, FOnHttpResponse Callback,
                    const TMap<FString, FString>& ExtraHeaders = TMap<FString, FString>());
    void ApplyHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe>, const FString&, bool, const FString&,
                      const TMap<FString, FString>& = TMap<FString, FString>()) const;
    void SendRequest(const FString& Verb, const FString& Url, const FString& ContentType,
                     const TArray<uint8>& Payload, bool Session, int, FOnHttpResponse Callback, const FString& Accept = "",
                     const TMap<FString, FString>& ExtraHeaders = TMap<FString, FString>(), bool bAllowRetry = true)
    {
        auto Request = MakeShared<IHttpRequest>(); Request->Verb = Verb;
        ApplyHeaders(Request, ContentType, Session, Accept, ExtraHeaders); LastHeaders = Request->Headers;
        const bool Json = ContentType == "application/json";
        Send({Verb, Url.substr(ActiveHost.size() + 1), Json ? FJsonSerializer::SerializedBody : MakeShared<FJsonObject>(),
              Session, ContentType, FString(std::string(Payload.begin(), Payload.end())), bAllowRetry}, Callback);
    }
    void GetBinary(const FString& Endpoint, bool Session, FOnHttpResponse Callback)
    { Send({"GET", Endpoint, MakeShared<FJsonObject>(), Session}, Callback); }
    void Get(const FString& Endpoint, bool Session, FOnHttpResponse Callback)
    { Send({"GET", Endpoint, MakeShared<FJsonObject>(), Session}, Callback); }
};
struct UHorizonAuthManager
{
    UHorizonHttpClient* HttpClient;
    FHorizonUserData CurrentUser;
    struct Event { int Count = 0; void Broadcast() { ++Count; } } OnUserSignedIn;
    bool ValidCachedSession = false;
    bool IsSignedIn() const { return !CurrentUser.UserId.IsEmpty() && !CurrentUser.AccessToken.IsEmpty(); }
    FHorizonUserData GetCurrentUser() const { return CurrentUser; }
    void SignUpAnonymous(const FString&, FOnAuthComplete, const FString& = "", FOnAnonymousIdentityIssued = {});
    void SignInAnonymous(const FString&, FOnAuthComplete);
    void RestoreAnonymousSession(FOnAuthComplete);
    void HandleAuthResponse(const FHorizonNetworkResponse&, FOnAuthComplete);
    void CacheSession();
    void CheckAuth(FOnAuthComplete Callback);
    void ClearSession() { CurrentUser = {}; HttpClient->ClearSessionToken(); }
};
struct UHorizonCloudSaveManager
{
    UHorizonAuthManager* AuthManager;
    UHorizonHttpClient* HttpClient;
    void LoadBytes(FOnBinaryComplete);
    void LoadSnapshot(FOnCloudSaveSnapshotComplete);
    void LoadBytesSnapshot(FOnCloudSaveSnapshotComplete);
    void Save(const FString&, int64, FOnCloudSaveWriteComplete);
    void SaveBytes(const TArray<uint8>&, int64, FOnCloudSaveWriteComplete);
    void Save(const FString&, FOnRequestComplete);
    void SaveBytes(const TArray<uint8>&, FOnRequestComplete);
};
struct UHorizonNewsManager
{
    UHorizonHttpClient* HttpClient;
    TArray<FHorizonNewsEntry> CachedNews;
    double CacheTimestamp = 0.0;
    static constexpr double CacheTTLSeconds = 300.0;
    void LoadNews(int32, const FString&, bool, FOnNewsComplete);
};
#include "ManagerMethods.inc"

void Require(bool Condition, const char* Message)
{ if (!Condition) throw std::runtime_error(Message); }

int main()
{
    try
    {
        // The same model parser still accepts email and Apple auth responses.
        FHorizonUserData Model;
        auto EmailResponse = MakeShared<FJsonObject>();
        EmailResponse->SetStringField("userId", "email-user");
        EmailResponse->SetStringField("username", "Email User");
        EmailResponse->SetStringField("email", "user@example.invalid");
        EmailResponse->SetStringField("accessToken", "email-session");
        Model.UpdateFromAuthResponse(EmailResponse);
        Require(Model.Email == "user@example.invalid" && Model.AccessToken == "email-session" && Model.AuthType == EHorizonAuthType::Email,
                "email auth fields changed");
        EmailResponse->SetStringField("appleUserId", "apple-user");
        EmailResponse->SetBoolField("isPrivateRelayEmail", true);
        Model.UpdateFromAuthResponse(EmailResponse);
        Require(Model.AuthType == EHorizonAuthType::Apple && Model.bIsPrivateRelayEmail, "Apple auth fields changed");
        UHorizonHttpClient Http;
        UHorizonAuthManager Auth{&Http};
        const FString Issued = "server-issued-anonymous-token-892";
        int SignIns = 0;
        bool SignupHasSession = false, FailSignIn = false, MissingIssuedToken = false, FailCheckAuth = false;
        Http.Server = [&](const auto& Request) {
            if (Request.Endpoint == "api/v1/app/user-management/check-auth")
            {
                FHorizonNetworkResponse Response;
                Response.bSuccess = !FailCheckAuth;
                Response.JsonData->SetBoolField("isAuthenticated", Auth.ValidCachedSession);
                return Response;
            }
            Require(!Request.UseSession, "signup/signin must not use a Bearer session");
            Require(Request.Body->GetStringField("type") == "ANONYMOUS", "wrong auth type");
            FHorizonNetworkResponse Response;
            Response.JsonData->SetStringField("userId", "user-892");
            Response.JsonData->SetStringField("username", "Player");
            if (Request.Endpoint == "api/v1/app/user-management/signup")
            {
                Require(!Request.Body->HasField("anonymousToken"), "anonymous signup sent a client token");
                Require(!Request.RetryAllowed, "non-idempotent signup must have exactly one transport attempt");
                if (!MissingIssuedToken) Response.JsonData->SetStringField("anonymousToken", Issued);
                if (SignupHasSession) Response.JsonData->SetStringField("accessToken", "signup-session");
                Response.JsonData->SetBoolField("isAnonymous", true);
            }
            else
            {
                Require(Request.Endpoint == "api/v1/app/user-management/signin", "wrong signin endpoint");
                Require(Request.Body->GetStringField("anonymousToken") == Issued, "signin did not use issued token");
                ++SignIns;
                Response.bSuccess = !FailSignIn;
                Response.JsonData->SetStringField("accessToken", "signin-session");
                // SignInResponse intentionally omits isAnonymous and anonymousToken.
            }
            return Response;
        };
        int Completed = 0;
        bool Success = false;
        auto Completion = FOnAuthComplete::CreateLambda([&](bool Value) { ++Completed; Success = Value; });
        Auth.SignUpAnonymous("Player", Completion);
        Require(Success && Completed == 1 && SignIns == 1, "signup must complete once after issued-token signin");
        Require(Auth.IsSignedIn() && Auth.CurrentUser.AnonymousToken == Issued && Auth.CurrentUser.bIsAnonymous,
                "anonymous identity was lost from the signin response");
        Require(Http.SessionToken == "signin-session", "signin session was not installed");
        Require(!Http.LastHeaders.count("If-Match"), "requests without extra headers must not send If-Match");
        Require(UHorizonSessionSave::Stored->CachedAnonymousToken == Issued, "issued token was not persisted");
        Auth.ClearSession();
        Auth.RestoreAnonymousSession(Completion);
        Require(Success && SignIns == 2 && Auth.IsSignedIn(), "expired session must restore using the issued token");
        Auth.ClearSession();
        FailCheckAuth = true;
        FailSignIn = true;
        Auth.RestoreAnonymousSession(Completion);
        Require(!Success && !Auth.IsSignedIn() && UHorizonSessionSave::Stored->CachedAnonymousToken == Issued,
                "failed expiry check and signin must preserve the disk credential");
        FailSignIn = false;
        Auth.RestoreAnonymousSession(Completion);
        Require(Success && Auth.IsSignedIn(), "restore after an unsuccessful expiry refresh must sign in again");
        FailCheckAuth = false;
        Auth.ClearSession();
        SignupHasSession = true;
        Auth.SignUpAnonymous("Player", Completion, "deprecated-client-token");
        Require(Success && Http.SessionToken == "signup-session", "direct signup session must not trigger signin");
        Auth.ClearSession();
        SignupHasSession = false;
        FailSignIn = true;
        Auth.SignUpAnonymous("Player", Completion);
        Require(!Success && !Auth.IsSignedIn() && UHorizonSessionSave::Stored->CachedAnonymousToken == Issued,
                "failed signin must retain the newly issued account credential");
        FailSignIn = false;
        Auth.RestoreAnonymousSession(Completion);
        Require(Success && Auth.IsSignedIn(), "token-only cache must restore the existing account");
        Auth.ClearSession();
        MissingIssuedToken = true;
        const int Before = SignIns;
        Auth.SignUpAnonymous("Player", Completion);
        Require(!Success && SignIns == Before && !Auth.IsSignedIn(), "signup without issued token must fail without signin");
        Auth.SignInAnonymous(Issued, Completion);
        Require(Success && Auth.CurrentUser.AnonymousToken == Issued && Auth.CurrentUser.bIsAnonymous,
                "returning-account signin must retain its token and anonymous status");

        UHorizonCloudSaveManager Cloud{&Auth, &Http};
        const TArray<uint8> Bytes{0, 255, 128, 42};
        int Status = 200;
        Http.Server = [&](const auto& Request) {
            Require(Http.LastHeaders["Content-Type"] == "application/json", "binary load request must be JSON");
            Require(Http.LastHeaders["Accept"] == "application/octet-stream", "binary load must negotiate binary response");
            Require(Http.LastHeaders["Authorization"] == "Bearer signin-session", "binary load must send Bearer session");
            Require(Http.LastHeaders["X-API-Key"] == "project-key-892", "binary load must send API key");
            Require(Request.Method == "POST", "binary Cloud Save load must use POST");
            Require(Request.Endpoint == "api/v1/app/cloud-save/load", "binary load must not use a query-only route");
            Require(Request.UseSession, "binary load must use the player session");
            Require(Request.Body->GetStringField("userId") == "user-892", "binary load needs JSON userId body");
            Require(Request.Payload == "{\"userId\":\"user-892\"}", "binary load did not serialize its JSON request");
            Require(!Http.LastHeaders.count("If-Match"), "unconditional binary load must not send If-Match");
            FHorizonNetworkResponse Response;
            Response.StatusCode = Status; Response.bSuccess = Status < 400;
            if (Status == 200) Response.BinaryData = Bytes;
            return Response;
        };
        TArray<uint8> Loaded;
        int Loads = 0;
        auto LoadCompletion = FOnBinaryComplete::CreateLambda([&](bool Value, const TArray<uint8>& Data) {
            ++Loads; Success = Value; Loaded = Data;
        });
        Cloud.LoadBytes(LoadCompletion);
        Require(Success && Loaded == Bytes, "binary bytes were not preserved");
        Status = 204;
        Cloud.LoadBytes(LoadCompletion);
        Require(!Success && Loaded.empty(), "204 must return absent data");
        Status = 401;
        Cloud.LoadBytes(LoadCompletion);
        Require(!Success && Loaded.empty(), "401 must return failure");

        // Revision-checked saves for several devices. Server contract: AppCloudSaveController and
        // CloudSaveExceptionHandler in horizOn-Server, checked 2026-10-11 (released in 1.64.1).
        struct FExpected
        {
            FString Endpoint, ContentType, Payload, IfMatch;
            int Status = 200;
            std::vector<FString> Headers;
            std::function<void(FHorizonNetworkResponse&)> Fill;
        };
        std::vector<FExpected> Script;
        Http.Server = [&](const auto& Request) {
            Require(!Script.empty(), "unexpected cloud save request");
            const FExpected Expected = Script.front();
            Script.erase(Script.begin());
            Require(Request.Method == "POST" && Request.UseSession, "cloud save must POST with the player session");
            Require(Request.Endpoint == Expected.Endpoint, "wrong cloud save endpoint");
            Require(Http.LastHeaders["Content-Type"] == Expected.ContentType, "wrong cloud save content type");
            Require(Http.LastHeaders["Authorization"] == "Bearer signin-session", "cloud save must send Bearer session");
            if (!Expected.Payload.IsEmpty()) Require(Request.Payload == Expected.Payload, "wrong cloud save payload");
            const auto IfMatch = Http.LastHeaders.find("If-Match");
            if (Expected.IfMatch.IsEmpty()) Require(IfMatch == Http.LastHeaders.end(), "request must not send If-Match");
            else Require(IfMatch != Http.LastHeaders.end() && IfMatch->second == Expected.IfMatch, "wrong If-Match");
            FHorizonNetworkResponse Response;
            Response.StatusCode = Expected.Status;
            Response.bSuccess = Expected.Status < 400;
            if (!Response.bSuccess) Response.ErrorMessage = "HTTP " + std::to_string(Expected.Status);
            for (const auto& Header : Expected.Headers) Response.Headers.push_back(Header);
            if (Expected.Fill) Expected.Fill(Response);
            return Response;
        };
        auto Found = [](const FString& Data) {
            return [Data](FHorizonNetworkResponse& Response) {
                Response.JsonData->SetBoolField("found", true);
                Response.JsonData->SetStringField("saveData", Data);
            };
        };
        auto Written = [](double Size, double Revision) {
            return [Size, Revision](FHorizonNetworkResponse& Response) {
                Response.JsonData->SetBoolField("success", true);
                Response.JsonData->SetNumberField("dataSizeBytes", Size);
                if (Revision > 0) Response.JsonData->SetNumberField("revision", Revision);
            };
        };
        const FString Load = "api/v1/app/cloud-save/load", SaveJson = "api/v1/app/cloud-save/save";
        const FString SaveBinary = "api/v1/app/cloud-save/save?userId=user-892";
        FHorizonCloudSaveSnapshot Snapshot;
        FHorizonCloudSaveWriteResult Write;
        int Snapshots = 0, Writes = 0;
        auto OnSnapshot = FOnCloudSaveSnapshotComplete::CreateLambda([&](const FHorizonCloudSaveSnapshot& Value) { ++Snapshots; Snapshot = Value; });
        auto OnWrite = FOnCloudSaveWriteComplete::CreateLambda([&](const FHorizonCloudSaveWriteResult& Value) { ++Writes; Write = Value; });
        const FString Saved = "{\"level\":4}";

        Script.push_back({Load, "application/json", "", "", 200, {"x-cloud-save-revision: 3"}, Found("{\"level\":3}")});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(Snapshot.bSuccess && Snapshot.bFound && Snapshot.Data == "{\"level\":3}" && Snapshot.Revision == 3,
                "string snapshot must carry data and the header revision (case-insensitive)");
        Require(Http.LastHeaders["Accept"].empty(), "string snapshot must not negotiate binary");

        Script.push_back({SaveJson, "application/json", "", "3", 200, {}, Written(11, 4)});
        Cloud.Save(Saved, 3, OnWrite);
        Require(Http.Requests.back().Body->GetStringField("saveData") == Saved, "conditional save lost its data");
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Saved && Write.Revision == 4 && Write.DataSizeBytes == 11,
                "conditional save must return the new revision");

        Script.push_back({SaveJson, "application/json", "", "3", 409, {}, [](FHorizonNetworkResponse& Response) {
            Response.JsonData->SetStringField("error", "Cloud save revision mismatch"); }});
        Cloud.Save(Saved, 3, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Conflict && Write.StatusCode == 409 &&
                Write.ErrorMessage.find("another device") != std::string::npos && Write.Revision == -1,
                "HTTP 409 must be a conflict, not a failure");

        Script.push_back({Load, "application/json", "{\"userId\":\"user-892\"}", "", 200, {"X-Cloud-Save-Revision: 7"},
                          [&](FHorizonNetworkResponse& Response) { Response.BinaryData = Bytes; }});
        Cloud.LoadBytesSnapshot(OnSnapshot);
        Require(Http.LastHeaders["Accept"] == "application/octet-stream", "bytes snapshot must negotiate binary");
        Require(Snapshot.bSuccess && Snapshot.bFound && Snapshot.BinaryData == Bytes && Snapshot.Revision == 7,
                "bytes snapshot must carry bytes and the header revision");

        Script.push_back({Load, "application/json", "", "", 204, {"X-Cloud-Save-Revision: 0"}, nullptr});
        Cloud.LoadBytesSnapshot(OnSnapshot);
        Require(Snapshot.bSuccess && !Snapshot.bFound && Snapshot.BinaryData.empty() && Snapshot.Revision == 0 && Snapshot.HasRevision(),
                "204 must be an empty slot at revision 0");

        Script.push_back({SaveBinary, "application/octet-stream", FString(std::string(Bytes.begin(), Bytes.end())), "0", 200, {}, Written(4, 1)});
        Cloud.SaveBytes(Bytes, 0, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Saved && Write.Revision == 1,
                "first conditional bytes save must return revision 1");

        Script.push_back({SaveBinary, "application/octet-stream", "", "1", 409, {}, nullptr});
        Cloud.SaveBytes(Bytes, 1, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Conflict, "bytes conflict must be a conflict");

        // simpleServer sends no revision: unknown, not an empty slot.
        Script.push_back({Load, "application/json", "", "", 200, {}, Found("{}")});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(Snapshot.bSuccess && Snapshot.Revision == -1 && !Snapshot.HasRevision(), "missing header must be unknown");
        Script.push_back({Load, "application/json", "", "", 200, {"X-Cloud-Save-Revision: abc"}, Found("{}")});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(Snapshot.Revision == -1, "a non-numeric header must be unknown");
        Script.push_back({SaveJson, "application/json", "", "0", 200, {}, Written(2, 0)});
        Cloud.Save("{}", 0, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Saved && Write.Revision == -1, "save without revision must be unknown");

        Script.push_back({Load, "application/json", "", "", 200, {"X-Cloud-Save-Revision: 0"}, [](FHorizonNetworkResponse& Response) {
            Response.JsonData->SetBoolField("found", false); }});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(Snapshot.bSuccess && !Snapshot.bFound && Snapshot.Data.empty() && Snapshot.Revision == 0,
                "an empty JSON slot must be found=false at revision 0");
        Script.push_back({Load, "application/json", "", "", 200, {}, [&](FHorizonNetworkResponse& Response) { Response.BinaryData = Bytes; }});
        Cloud.LoadBytesSnapshot(OnSnapshot);
        Require(Snapshot.bSuccess && Snapshot.bFound && Snapshot.Revision == -1, "a binary load without header must be unknown");
        Script.push_back({Load, "application/json", "", "", 200, {"X-Cloud-Save-Revision: 9223372036854775807"}, Found("{}")});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(Snapshot.Revision == 9223372036854775807LL, "the full int64 range must parse");
        Script.push_back({Load, "application/json", "", "", 200, {"X-Cloud-Save-Revision: 9223372036854775808"}, Found("{}")});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(Snapshot.Revision == -1, "an int64 overflow must be unknown");

        // An unreadable 2xx answer is a failure, never an empty slot or a confirmed write.
        Script.push_back({Load, "application/json", "", "", 200, {"X-Cloud-Save-Revision: 7"}, [](FHorizonNetworkResponse& Response) {
            Response.JsonData = TSharedPtr<FJsonObject>(); }});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(!Snapshot.bSuccess && Snapshot.Revision == -1 && !Snapshot.ErrorMessage.empty(), "a non-JSON load must fail");
        Script.push_back({Load, "application/json", "", "", 200, {"X-Cloud-Save-Revision: 7"}, nullptr});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(!Snapshot.bSuccess && Snapshot.Revision == -1, "a load without found must fail");
        Script.push_back({Load, "application/json", "", "", 200, {"X-Cloud-Save-Revision: 7"}, [](FHorizonNetworkResponse& Response) {
            Response.JsonData->SetBoolField("found", true); }});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(!Snapshot.bSuccess && !Snapshot.bFound, "a found load without saveData must fail");
        Script.push_back({SaveJson, "application/json", "", "2", 200, {}, nullptr});
        Cloud.Save(Saved, 2, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Failed && Write.ErrorMessage.find("did not confirm") != std::string::npos,
                "a save answer without success must not count as written");
        Script.push_back({SaveJson, "application/json", "", "2", 200, {}, [](FHorizonNetworkResponse& Response) {
            Response.JsonData->SetBoolField("success", false); Response.JsonData->SetNumberField("revision", 3); }});
        Cloud.Save(Saved, 2, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Failed && Write.Revision == -1, "success=false must fail");
        Script.push_back({SaveBinary, "application/octet-stream", "", "2", 200, {}, [](FHorizonNetworkResponse& Response) {
            Response.JsonData = TSharedPtr<FJsonObject>(); }});
        Cloud.SaveBytes(Bytes, 2, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Failed, "a non-JSON save answer must fail");
        // Wrong JSON types: UE's getters would convert them, the SDK must not.
        Script.push_back({Load, "application/json", "", "", 200, {"X-Cloud-Save-Revision: 7"}, [](FHorizonNetworkResponse& Response) {
            Response.JsonData->SetNumberField("found", 0); }});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(!Snapshot.bSuccess, "a numeric found must fail");
        Script.push_back({Load, "application/json", "", "", 200, {"X-Cloud-Save-Revision: 7"}, [](FHorizonNetworkResponse& Response) {
            Response.JsonData->SetBoolField("found", true); Response.JsonData->SetNumberField("saveData", 5); }});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(!Snapshot.bSuccess, "a numeric saveData must fail");
        Script.push_back({SaveJson, "application/json", "", "2", 200, {}, [](FHorizonNetworkResponse& Response) {
            Response.JsonData->SetNumberField("success", 1); Response.JsonData->SetNumberField("revision", 3); }});
        Cloud.Save(Saved, 2, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Failed, "a numeric success must not count as written");
        Script.push_back({SaveJson, "application/json", "", "2", 200, {}, Written(11, 2.5)});
        Cloud.Save(Saved, 2, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Saved && Write.Revision == -1, "a fractional revision must be unknown");
        Script.push_back({SaveJson, "application/json", "", "2", 200, {}, Written(11, 1e19)});
        Cloud.Save(Saved, 2, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Saved && Write.Revision == -1, "an out-of-range revision must be unknown");

        // The unconditional methods stay as they were: no If-Match.
        bool LegacySaved = false;
        auto OnLegacy = FOnRequestComplete::CreateLambda([&](bool Value, const FString&) { LegacySaved = Value; });
        Script.push_back({SaveJson, "application/json", "", "", 200, {}, Written(11, 5)});
        Cloud.Save(Saved, OnLegacy);
        Require(LegacySaved, "legacy save must still work without If-Match");
        LegacySaved = false;
        Script.push_back({SaveBinary, "application/octet-stream", "", "", 200, {}, Written(4, 6)});
        Cloud.SaveBytes(Bytes, OnLegacy);
        Require(LegacySaved, "legacy bytes save must still work without If-Match");

        Script.push_back({SaveJson, "application/json", "", "5", 500, {}, nullptr});
        Cloud.Save(Saved, 5, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Failed && Write.ErrorMessage == "HTTP 500", "5xx must fail");
        Script.push_back({Load, "application/json", "", "", 401, {"X-Cloud-Save-Revision: 9"}, nullptr});
        Cloud.LoadSnapshot(OnSnapshot);
        Require(!Snapshot.bSuccess && Snapshot.Revision == -1 && Snapshot.StatusCode == 401, "failed load must not report a revision");

        const auto RequestsBefore = Http.Requests.size();
        Cloud.Save(Saved, -1, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Failed && Write.ErrorMessage.find("ExpectedRevision") == 0,
                "negative revision must fail locally");
        Cloud.Save("", 0, OnWrite);
        Require(Write.ErrorMessage == "Save data is required.", "empty data must fail locally");
        Cloud.Save("", -1, OnWrite);
        Require(Write.ErrorMessage == "Save data is required.", "missing data must be reported before an invalid revision");
        Cloud.SaveBytes(TArray<uint8>(), 0, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Failed && Http.Requests.size() == RequestsBefore && Script.empty(),
                "local failures must not send a request");
        Require(Snapshots == 15 && Writes == 16, "every revision call must complete exactly once");

        Auth.ClearSession();
        const auto RequestCount = Http.Requests.size();
        Cloud.LoadBytes(LoadCompletion);
        Require(!Success && Http.Requests.size() == RequestCount && Loads == 4,
                "unsigned binary load must fail locally and complete once");
        Cloud.LoadSnapshot(OnSnapshot);
        Require(!Snapshot.bSuccess && Snapshot.ErrorMessage == "User is not signed in.", "unsigned snapshot must fail locally");
        Cloud.LoadBytesSnapshot(OnSnapshot);
        Require(!Snapshot.bSuccess, "unsigned bytes snapshot must fail locally");
        Cloud.Save(Saved, 0, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Failed && Write.ErrorMessage == "User is not signed in.",
                "unsigned conditional save must fail locally");
        Cloud.SaveBytes(Bytes, 0, OnWrite);
        Require(Write.Status == EHorizonCloudSaveWriteStatus::Failed, "unsigned conditional bytes save must fail locally");
        Cloud.Save(Saved, -1, OnWrite);
        Require(Write.ErrorMessage.find("ExpectedRevision") == 0, "an invalid revision must be reported before the missing sign-in");
        Require(Http.Requests.size() == RequestCount && Snapshots == 17 && Writes == 19,
                "unsigned revision calls must not send a request");
        // HorizonHttpClient::ParseResponse stores application/json root arrays in
        // RawJsonBody, not JsonData or BinaryData (inspected 2026-10-22).
        const FString NewsBody = R"([{"id":"news-892","title":"Release","message":"Hello players","releaseDate":"2026-10-22T00:00:00Z","languageCode":"EN"}])";
        auto NewsObject = MakeShared<FJsonObject>();
        NewsObject->SetStringField("id", "news-892");
        NewsObject->SetStringField("title", "Release");
        NewsObject->SetStringField("message", "Hello players");
        NewsObject->SetStringField("releaseDate", "2026-10-22T00:00:00Z");
        NewsObject->SetStringField("languageCode", "EN");
        auto NewsValue = MakeShared<FJsonValue>();
        NewsValue->Type = EJson::Object;
        NewsValue->Object = NewsObject;
        FJsonSerializer::ArrayBodies[NewsBody] = {NewsValue};
        FJsonSerializer::ArrayBodies["[]"] = {};
        FHorizonNetworkResponse NewsResponse;
        NewsResponse.JsonData = {};
        NewsResponse.RawJsonBody = NewsBody;
        UHorizonHttpClient NewsHttp;
        NewsHttp.Server = [&](const auto& Request) {
            Require(Request.Method == "GET" && !Request.UseSession, "News must remain an API-key-only GET");
            return NewsResponse;
        };
        UHorizonNewsManager News{&NewsHttp};
        int NewsCompletions = 0;
        bool NewsSuccess = false;
        TArray<FHorizonNewsEntry> NewsEntries;
        auto OnNews = FOnNewsComplete::CreateLambda([&](bool Value, const TArray<FHorizonNewsEntry>& Entries) {
            ++NewsCompletions; NewsSuccess = Value; NewsEntries = Entries;
        });
        News.LoadNews(1000, "EN", false, OnNews);
        Require(NewsSuccess && NewsEntries.Num() == 1 && NewsCompletions == 1,
                "application/json root array must return its News entry from RawJsonBody");
        Require(NewsEntries[0].Id == "news-892" && NewsEntries[0].Title == "Release" &&
                NewsEntries[0].Message == "Hello players" && NewsEntries[0].LanguageCode == "EN" &&
                NewsEntries[0].ReleaseDate == "2026-10-22T00:00:00Z", "News entry fields must survive parsing");
        Require(NewsHttp.Requests[0].Endpoint == "api/v1/app/news?limit=100&languageCode=EN",
                "News endpoint must retain limit clamping and language");
        News.LoadNews(100, "EN", true, OnNews);
        Require(NewsSuccess && NewsEntries.Num() == 1 && NewsCompletions == 2 && NewsHttp.Requests.size() == 1,
                "parsed News must populate the existing cache without an extra request");
        FPlatformTime::Now += 301.0;
        NewsResponse.RawJsonBody = "[]";
        News.LoadNews(0, "DE", true, OnNews);
        Require(NewsSuccess && NewsEntries.empty() && NewsCompletions == 3 && NewsHttp.Requests.size() == 2 &&
                NewsHttp.Requests.back().Endpoint == "api/v1/app/news?limit=1&languageCode=DE",
                "expired cache must accept an empty News array and retain minimum limit");
        NewsResponse.RawJsonBody.Empty();
        NewsResponse.BinaryData.Append(reinterpret_cast<const uint8*>(NewsBody.data()), NewsBody.Len());
        News.LoadNews(10, "EN", false, OnNews);
        Require(NewsSuccess && NewsEntries.Num() == 1 && NewsCompletions == 4,
                "legacy binary JSON array fallback must still work");
        NewsResponse.RawJsonBody = "[]";
        News.LoadNews(10, "EN", false, OnNews);
        Require(NewsSuccess && NewsEntries.empty() && NewsCompletions == 5,
                "original JSON body must take precedence over binary fallback");
        NewsResponse.JsonData = MakeShared<FJsonObject>();
        auto NonObject = MakeShared<FJsonValue>();
        NonObject->Type = EJson::String;
        NewsResponse.JsonData->Arrays["items"] = {{}, NonObject, NewsValue};
        News.LoadNews(10, "EN", false, OnNews);
        Require(NewsSuccess && NewsEntries.Num() == 1 && NewsCompletions == 6,
                "legacy wrapped News must retain valid objects and ignore other values");
        NewsResponse.bSuccess = false;
        News.LoadNews(10, "EN", false, OnNews);
        Require(!NewsSuccess && NewsEntries.empty() && NewsCompletions == 7,
                "failed News response must complete once without returning cached entries");
        std::cout << "Unreal production anonymous auth, Cloud Save and News manager methods passed\n";
    }
    catch (const std::exception& Error)
    { std::cerr << Error.what() << '\n'; return 1; }
}
