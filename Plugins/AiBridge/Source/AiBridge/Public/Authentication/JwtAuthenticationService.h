#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "JwtAuthenticationService.generated.h"

UCLASS()
class AIBRIDGE_API UJwtAuthenticationService : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FString InBaseUrl);

	// Async-style callback instead of Task<string>
	void GetAuthToken(
		const FString& UserId,
		const FString& Role,
		const FString& ApiKey,
		TFunction<void(const FString& Token)> Callback
	);

private:
	FString BaseUrl;

	FString CachedToken;
	FDateTime TokenExpiry;

	FCriticalSection CacheLock;

	bool IsTokenValid() const;

	void HandleResponse(
		FHttpRequestPtr Request,
		FHttpResponsePtr Response,
		bool bWasSuccessful,
		TFunction<void(const FString&)> Callback
	);
};
