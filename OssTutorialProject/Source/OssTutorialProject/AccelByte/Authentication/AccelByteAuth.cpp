// Copyright (c) 2022 AccelByte Inc. All Rights Reserved.
// This is licensed software from AccelByte Inc, for limitations
// and restrictions contact your company contract manager.

#include "AccelByteAuth.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemAccelByte.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "OssTutorialProject/HUD/OssTutorialMenuHUD.h"
#include "OnlineSubsystemAccelByteDefines.h"
#include "Core/AccelByteMultiRegistry.h"
#include "OnlineIdentityInterfaceAccelByte.h"
#include "OnlineSubsystemAccelByte/Public/OnlineSubsystemAccelByteDefines.h"

void UAccelByteAuth::NativeConstruct()
{
	Super::NativeConstruct();

	TutorialMenuHUD = Cast<AOssTutorialMenuHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());

	Btn_Login->OnClicked.AddUniqueDynamic(this, &UAccelByteAuth::OnClickLoginButton);
}

void UAccelByteAuth::AccelByteOssLogin()
{
	const IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(ACCELBYTE_SUBSYSTEM);

	//Block call if process is already running.
	if (bSdkProcessRunning)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Error Login: SDK is locked!"));
		return;
	}
	else
	{
		bSdkProcessRunning = true;
	}

	//Save Identity to class varaible.
	if (!IdentityRef.IsValid())
	{
		//Fail and bail if still null.
		IdentityRef = OnlineSub->GetIdentityInterface();
		if (!IdentityRef.IsValid())
		{
			bSdkProcessRunning = false;
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Identity ref null @ OssAuth"));
			return;
		}
	}

	if (IdentityRef.IsValid())
	{
		FOnlineAccountCredentials LoginCredentialsRef;

		LoginCredentialsRef.Id = Etb_Username->GetText().ToString();
		LoginCredentialsRef.Token = Etb_Password->GetText().ToString();
		LoginCredentialsRef.Type = "AccelByte";

		IdentityRef->OnLoginCompleteDelegates->AddUObject(this, &UAccelByteAuth::LoginComplete);

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Logging in"));

		IdentityRef->Login(PlayerNum, LoginCredentialsRef);
	}

	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Identity ref is null! @ OssAuth"));
		bSdkProcessRunning = false;
		return;
	}
}

void UAccelByteAuth::AccelByteOssLogout()
{
	const IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(ACCELBYTE_SUBSYSTEM);

	if (!ApiClientRef.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Permission Denied: Api Client is invalid. Logout Failed!"));

		return;
	}

	if (bSdkProcessRunning)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Error Logout: SDK is locked!"));

		return;
	}

	bSdkProcessRunning = true;


	IdentityRef = OnlineSub->GetIdentityInterface();

	if (IdentityRef.IsValid())
	{
		IdentityRef->OnLogoutCompleteDelegates->AddUObject(this, &UAccelByteAuth::LogoutComplete);

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Logging out"));


		IdentityRef->Logout(PlayerNum);

	}
}

void UAccelByteAuth::OnClickLoginButton()
{
	//Login Delegate
	//const IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(ACCELBYTE_SUBSYSTEM);
	//if (!OnlineSub->GetIdentityInterface()->OnLoginCompleteDelegates->IsBoundToObject(this))
	//{
	//	OnlineSub->GetIdentityInterface()->OnLoginCompleteDelegates->AddUObject(this, &UAccelByteAuth::LoginComplete);
	//}
	AccelByteOssLogin();
}

void UAccelByteAuth::OnClickLogoutButton()
{
	//Logout Delegate
	//const IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(ACCELBYTE_SUBSYSTEM);
	//if (!OnlineSub->GetIdentityInterface()->OnLogoutCompleteDelegates->IsBoundToObject(this))
	//{
	//	OnlineSub->GetIdentityInterface()->OnLogoutCompleteDelegates->AddUObject(this, &UAccelByteAuth::LogoutComplete);
	//}
	AccelByteOssLogout();
}

void UAccelByteAuth::LoginComplete(int32 PlayerNumber, bool bIsSuccess, const FUniqueNetId& UserId, const FString& ErrorMessage)
{
	//Used for debugging.
	bool ApiClientDebug = true;

	if (bIsSuccess)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Logged in"));

		bSdkProcessRunning = false;

		//Added for debugging purposes.
		if (!ApiClientRef.IsValid() || ApiClientDebug)
		{
			ApiClientRef = WrapperGetApiClient(PlayerNumber, ApiClientRef);

			if (!ApiClientRef.IsValid())
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Error: Api Client Null!/Locking up!"));

				return;
			}

		}

		TutorialMenuHUD->OnLoginRequest.Broadcast(bIsSuccess);
	}

	else
	{

		//Sends an error message up to blueprints.
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Login with Username via AB OSS is Failed, Message: %s"), *ErrorMessage));
	}

	IdentityRef->ClearOnLoginCompleteDelegates(PlayerNumber, this);
	bSdkProcessRunning = false;
}

void UAccelByteAuth::LoginSuccess(bool bIsSuccess)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("Login with Username via AB OSS is Success"));

	TutorialMenuHUD->OnLoginRequest.Broadcast(bIsSuccess);
}

void UAccelByteAuth::LoginFailed(const FString& ErrorMessage)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Login with Username via AB OSS is Failed, Message: %s"), *ErrorMessage));
}

void UAccelByteAuth::LogoutComplete(int32 PlayerNumber, bool bIsSuccess)
{
	if (bIsSuccess)
	{
		TutorialMenuHUD->OnLogoutRequest.Broadcast(bIsSuccess);
	}

	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Logout is Failed")));
	}

	IdentityRef->ClearOnLogoutCompleteDelegates(PlayerNum, this);
	bSdkProcessRunning = false;
}

void UAccelByteAuth::LogoutSuccess(bool bIsSuccess)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("Logout via AB OSS is Success"));

	if (TutorialMenuHUD == nullptr)
	{
		TutorialMenuHUD = Cast<AOssTutorialMenuHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
	}

	TutorialMenuHUD->OnLogoutRequest.Broadcast(bIsSuccess);
}

void UAccelByteAuth::LogoutFailed()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Logout via AB OSS is Failed"));
}

//Wrapper to get the api client.
FApiClientPtr UAccelByteAuth::WrapperGetApiClient(int32 LocalUserNum, FApiClientPtr PtrIn)
{
	bool DebugMode = true;

	//Spit pointer back out if its already valid.
	if (PtrIn.IsValid() && DebugMode == false)
	{
		return PtrIn;
	}

	/** Get Identity casted to the FOnlineIdentityAccelByte class.
	 *This makes it possible to call GetApiClient. An object of FOnlineIdentityAccelByte must be created to access this-
	 *function despite the fact that is a public function for some reason.
	 */
	const FOnlineIdentityAccelBytePtr IdentityInterface = CastToIdentity();

	if (IdentityInterface.IsValid())
	{

		/** Now after all the craziness it took to construct an object of this FOnlineAccelByteIdentity: We can finally-
		 *declare our ApiClientPtr. From here we can proceed as one would expect.
		 */
		FApiClientPtr PtrRef = IdentityInterface->GetApiClient(LocalUserNum);

		if (PtrRef.IsValid())
		{
			return PtrRef;
		}
	}


	return nullptr;

}

//Get identity interface and cast it to the FOnlineIdentityAccelBytePtr.
const FOnlineIdentityAccelBytePtr UAccelByteAuth::CastToIdentity()
{

	/** This must be wrapped into an if statement because it returns a bool if it fails. This lets us do a safety check-
	 *before possibly blowing the engine and/or the entire universe. A class variable called OnlineSubRef which is a-
	 *a pointer const and holds the IonlineSubSystem will be set up and written to in this function. If it did so-
	 *safely, we can then proceed. Otherwise we should fail and bail to avoid disaster.
	 *For info on why any of this works check out: https://www.internalpointers.com/post/constant-pointers-vs-pointer-constants-c-and-c
	 */
	if (WrapperGetSubsystem())
	{
		/**Cast to FOnlineIdentityAccelByte to access the needed function and get the identity interface.
		 *In doing so this way it becomes possible to access GetApiClient.
		 */
		const FOnlineIdentityAccelBytePtr IdentityInterface = StaticCastSharedPtr<FOnlineIdentityAccelByte>(OnlineSubRef->GetIdentityInterface());
		return IdentityInterface;
	}


	return  nullptr;


}

//Attempts to get the online sub system. This may explode who knows!
bool UAccelByteAuth::WrapperGetSubsystem()
{
	//Get the online Subsystem.
	const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld(), ACCELBYTE_SUBSYSTEM);

	if (!OnlineSub)
	{
		return false;
	}

	//For more info on why this works visit: https://www.internalpointers.com/post/constant-pointers-vs-pointer-constants-c-and-c
	OnlineSubRef = OnlineSub;

	bool IsValid = false;

	//Make sure one last time this is valid to avoid a possible crash.
	if (OnlineSubRef)
	{
		IsValid = true;
	}

	else
	{
		IsValid = false;
	}

	return IsValid;
}
