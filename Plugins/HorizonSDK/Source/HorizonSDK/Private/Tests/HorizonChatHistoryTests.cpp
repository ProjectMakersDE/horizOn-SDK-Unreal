#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Storage/HorizonChatHistory.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
namespace HorizonChatHistoryTest
{
const FString User=TEXT("00000000-0000-4000-8000-000000001014"),Channel=TEXT("00000000-0000-4000-8000-000000003014");
FHorizonChatMessage Message(int32 I){FHorizonChatMessage M;M.Id=FGuid(0,0,0,uint32(I+1)).ToString(EGuidFormats::DigitsWithHyphens).ToLower();M.ChannelId=Channel;M.AuthorId=User;M.Text=TEXT("stored 雪😀\n\t")+LexToString(I);M.CreatedAt=TEXT("2026-10-16T12:00:00.000Z");M.ExpiresAt=TEXT("2026-10-16T12:00:30.000Z");return M;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHorizonChatHistoryNative,"Horizon.Chat.LocalHistory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHorizonChatHistoryNative::RunTest(const FString&)
{
    using namespace HorizonChatHistoryTest;const FString Base=FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_CHAT_HISTORY_TEST_DIR"));if(Base.IsEmpty()){AddError(TEXT("An isolated archive directory is required"));return false;}FHorizonChatHistory H(TEXT("history-fixture"),User,Base);FString Error;bool Changed=false;
    if(FPlatformMisc::GetEnvironmentVariable(TEXT("HORIZON_CHAT_HISTORY_STAGE"))==TEXT("restart")){auto P=H.Read(Channel,TEXT(""),50);TestTrue(TEXT("Actual second editor process restores persisted device copy"),P.bSuccess&&P.Messages.Num()==1&&P.Messages[0].Text==Message(999).Text);TestTrue(TEXT("Restart persists duplicate identity"),H.Append(Channel,Message(999),H.Capture(Channel),Changed,Error)&&!Changed);TestTrue(TEXT("Final test-owned archive clear"),H.Clear(TEXT(""),Error));return true;}
    TestTrue(TEXT("Clean isolated archive"),H.Clear(TEXT(""),Error));const FString G=H.Capture(Channel);for(int32 I=0;I<150;++I){const bool Ok=H.Append(Channel,Message(I),G,Changed,Error);if(!Ok&&I<3)AddInfo(TEXT("Append index ")+LexToString(I)+TEXT(": ")+Error);TestTrue(TEXT("Append immutable device message"),Ok&&Changed);}
    auto P=H.Read(Channel,TEXT(""),20);TestTrue(TEXT("Bounded newest page keeps exact Unicode"),P.bSuccess&&P.Messages.Num()==20&&P.Messages[0].Id==Message(149).Id&&P.Messages.Last().Text==Message(130).Text);int32 Count=P.Messages.Num();while(!P.NextCursor.IsEmpty()){P=H.Read(Channel,P.NextCursor,20);if(!TestTrue(TEXT("Older byte-cursor page"),P.bSuccess))break;Count+=P.Messages.Num();}TestEqual(TEXT("All history survives server expiry and window eviction"),Count,150);
    FHorizonChatHistory Reopened(TEXT("history-fixture"),User,Base);TestTrue(TEXT("New storage instance keeps old ID index"),Reopened.Append(Channel,Message(1),Reopened.Capture(Channel),Changed,Error)&&!Changed);auto Conflict=Message(1);Conflict.Text=TEXT("changed");TestFalse(TEXT("Conflicting same ID rejected"),Reopened.Append(Channel,Conflict,G,Changed,Error));TestEqual(TEXT("Conflict visible"),Error,FString(TEXT("LOCAL_HISTORY_CONFLICT")));
    FHorizonChatHistory OtherKey(TEXT("different-key"),User,Base),OtherUser(TEXT("history-fixture"),Channel,Base);TestTrue(TEXT("Project key partitions local copies"),OtherKey.Read(Channel,TEXT(""),50).Messages.IsEmpty());TestTrue(TEXT("User partitions local copies"),OtherUser.Read(Channel,TEXT(""),50).Messages.IsEmpty());
    const FString Cursor=H.Read(Channel,TEXT(""),20).NextCursor;TestTrue(TEXT("Explicit channel clear"),H.Clear(Channel,Error));TestTrue(TEXT("Clear removes local messages"),H.Read(Channel,TEXT(""),20).Messages.IsEmpty());TestFalse(TEXT("Pre-clear callback cannot append again"),H.Append(Channel,Message(151),G,Changed,Error));TestEqual(TEXT("Clear invalidates old cursor"),H.Read(Channel,Cursor,20).ErrorCode,FString(TEXT("LOCAL_HISTORY_STALE")));
    const FString Next=H.Capture(Channel);TestTrue(TEXT("Fresh received message after clear"),H.Append(Channel,Message(152),Next,Changed,Error));TestTrue(TEXT("Clear identity"),H.Clear(TEXT(""),Error));TestFalse(TEXT("Clear-all fences captured write"),H.Append(Channel,Message(153),Next,Changed,Error));
    const FString Blocker=Base/TEXT("not-a-directory");TestTrue(TEXT("Construct genuine filesystem error"),FFileHelper::SaveStringToFile(TEXT("owned fixture"),*Blocker));FHorizonChatHistory Bad(TEXT("fixture"),User,Blocker);TestFalse(TEXT("Local filesystem failure is visible"),Bad.Append(Channel,Message(1),Bad.Capture(Channel),Changed,Error));TestEqual(TEXT("Local I/O structured error"),Error,FString(TEXT("LOCAL_HISTORY_IO")));IFileManager::Get().Delete(*Blocker);
    TestTrue(TEXT("Persist restart fixture"),H.Append(Channel,Message(999),H.Capture(Channel),Changed,Error));return true;
}
#endif
