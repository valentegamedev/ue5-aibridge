#include "Authentication/JwtAuthenticationService.h"
#include "Json.h"
#include "JsonUtilities.h"

void UJwtAuthenticationService::Initialize(const FString InBaseUrl)
{
    BaseUrl = InBaseUrl.IsEmpty() ? TEXT("https://conversation-api.com") : InBaseUrl;
}

bool UJwtAuthenticationService::IsTokenValid() const
{
    return !CachedToken.IsEmpty() && FDateTime::UtcNow() < TokenExpiry;
}

void UJwtAuthenticationService::GetAuthToken(
    const FString& UserId,
    const FString& Role,
    const FString& ApiKey,
    TFunction<void(const FString&)> Callback)
{

    if (IsTokenValid())
    {
        Callback(CachedToken);
        return;
    }


    FString Url = BaseUrl + TEXT("/api/auth/token"); // adjust endpoint

    UE_LOG(LogTemp, Warning, TEXT("[%s] Auth Url: %s"), *StaticClass()->GetName(), *Url);
    
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        FHttpModule::Get().CreateRequest();

    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));

    // Headers
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("x-api-key"), ApiKey);

    // JSON Body
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    JsonObject->SetStringField(TEXT("userId"), UserId);
    JsonObject->SetStringField(TEXT("role"), Role);

    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    Request->SetContentAsString(RequestBody);

    Request->OnProcessRequestComplete().BindUObject(
        this,
        &UJwtAuthenticationService::HandleResponse,
        Callback
    );

    Request->ProcessRequest();
}

void UJwtAuthenticationService::HandleResponse(
    FHttpRequestPtr Request,
    FHttpResponsePtr Response,
    bool bWasSuccessful,
    TFunction<void(const FString&)> Callback)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Auth request failed"));
        Callback(TEXT(""));
        return;
    }

    FString ResponseStr = Response->GetContentAsString();

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse auth response"));
        Callback(TEXT(""));
        return;
    }

    FString Token = JsonObject->GetStringField(TEXT("token"));

    if (!Token.IsEmpty())
    {
        FScopeLock Lock(&CacheLock);

        CachedToken = Token;
        TokenExpiry = FDateTime::UtcNow() + FTimespan::FromMinutes(55);
    }

    Callback(Token);
}
