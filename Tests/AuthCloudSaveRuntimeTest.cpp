// Runs actual manager methods, with only the UE and network/storage boundaries replaced.
#include "Transport/HorizonLeaderboardTransportContract.h"

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
struct FString : std::string
{
    using std::string::string;
    FString(const std::string& Value) : std::string(Value) {}
    bool IsEmpty() const { return empty(); }
    void Empty() { clear(); }
    const char* operator*() const { return c_str(); }
    FString operator/(const FString& Other) const { return *this + "/" + Other; }
    static FString Printf(const char* Format, const char* Value)
    { char Buffer[512]; std::snprintf(Buffer, sizeof(Buffer), Format, Value); return Buffer; }
};
template <typename T> struct TArray : std::vector<T>
{ using std::vector<T>::vector; int Num() const { return this->size(); }
  void Append(const T* Data, int Length) { this->insert(this->end(), Data, Data + Length); } };
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
struct FJsonObject
{
    std::map<FString, FString> Strings;
    std::map<FString, bool> Bools;
    void SetStringField(const FString& Key, const FString& Value) { Strings[Key] = Value; }
    void SetBoolField(const FString& Key, bool Value) { Bools[Key] = Value; }
    FString GetStringField(const FString& Key) const
    { auto It = Strings.find(Key); return It == Strings.end() ? FString() : It->second; }
    bool TryGetStringField(const FString& Key, FString& Out) const
    { auto It = Strings.find(Key); if (It == Strings.end()) return false; Out = It->second; return true; }
    bool GetBoolField(const FString& Key) const
    { auto It = Bools.find(Key); return It != Bools.end() && It->second; }
    bool HasField(const FString& Key) const { return Strings.count(Key) || Bools.count(Key); }
};
template <typename T = void> struct TJsonWriter { FString* Output; };
template <typename T = void> struct TJsonWriterFactory
{
    static TSharedRef<TJsonWriter<T>> Create(FString* Output)
    { auto Writer = MakeShared<TJsonWriter<T>>(); Writer->Output = Output; return Writer; }
};
struct FJsonSerializer
{
    static inline TSharedRef<FJsonObject> SerializedBody;
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
struct FHorizonNetworkResponse
{
    bool bSuccess = true;
    int StatusCode = 200;
    FString ErrorMessage;
    TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
    TArray<uint8> BinaryData;
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
using FOnHttpResponse = Delegate<const FHorizonNetworkResponse&>;
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
    struct Request { FString Method, Endpoint; TSharedRef<FJsonObject> Body; bool UseSession; };
    std::vector<Request> Requests;
    std::function<FHorizonNetworkResponse(const Request&)> Server;
    FString SessionToken, ApiKey = "project-key-892", ActiveHost = "http://loopback.invalid";
    std::map<FString, FString> LastHeaders;
    void SetSessionToken(const FString& Value) { SessionToken = Value; }
    void ClearSessionToken() { SessionToken.Empty(); }
    void Send(const Request& Request, FOnHttpResponse Callback)
    { Requests.push_back(Request); Callback.ExecuteIfBound(Server(Request)); }
    void PostJson(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool Session, FOnHttpResponse Callback)
    { Send({"POST", Endpoint, Body, Session}, Callback); }
    void PostJsonForBinary(const TSharedRef<FJsonObject>& Body, const FString& Endpoint, bool Session, FOnHttpResponse Callback);
    void ApplyHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe>, const FString&, bool, const FString&) const;
    void SendRequest(const FString& Verb, const FString& Url, const FString& ContentType,
                     const TArray<uint8>& Payload, bool Session, int, FOnHttpResponse Callback, const FString& Accept)
    {
        auto Request = MakeShared<IHttpRequest>(); Request->Verb = Verb;
        ApplyHeaders(Request, ContentType, Session, Accept); LastHeaders = Request->Headers;
        if (std::string(Payload.begin(), Payload.end()) != "{\"userId\":\"user-892\"}")
            throw std::runtime_error("binary load did not serialize its JSON request");
        Send({Verb, Url.substr(ActiveHost.size() + 1), FJsonSerializer::SerializedBody, Session}, Callback);
    }
    void GetBinary(const FString& Endpoint, bool Session, FOnHttpResponse Callback)
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
    void SignUpAnonymous(const FString&, FOnAuthComplete, const FString& = "");
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
        Auth.ClearSession();
        const auto RequestCount = Http.Requests.size();
        Cloud.LoadBytes(LoadCompletion);
        Require(!Success && Http.Requests.size() == RequestCount && Loads == 4,
                "unsigned binary load must fail locally and complete once");
        std::cout << "Unreal production anonymous auth and Cloud Save manager methods passed\n";
    }
    catch (const std::exception& Error)
    { std::cerr << Error.what() << '\n'; return 1; }
}
