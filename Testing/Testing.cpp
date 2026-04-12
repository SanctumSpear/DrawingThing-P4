#include "pch.h"
#include "CppUnitTest.h"

#include <fstream>
#include <sstream>
#include <string>
#include <cstdio> 

#include "../Common/Packet.h"
#include "../Common/Game.h"
#include "../Common/logger.h"
#include "../Server/AccountManager.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

static void WriteAccountsFile(const std::string& path,
                               const std::string& contents) {
    std::ofstream f(path, std::ios::trunc);
    f << contents; 
}

static std::string ReadFile(const std::string& path) {
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void DeleteFile(const std::string& path) {
    std::remove(path.c_str());
}


namespace Testing {

    TEST_CLASS(PacketTests)
    {
    public:

        // --- CONNECT 

        TEST_METHOD(Connect_HasCorrectType)
        {
            Packet p = Packet::MakeConnectPacket(1, 0, 0);
            Assert::IsTrue(p.header.type == PacketType::CONNECT);
        }

        TEST_METHOD(Connect_HasCorrectSessionID)
        {
            Packet p = Packet::MakeConnectPacket(42, 0, 0);
            Assert::AreEqual((int)42, (int)p.header.sessionID);
        }

        TEST_METHOD(Connect_HasNoPayload)
        {
            Packet p = Packet::MakeConnectPacket(1, 0, 0);
            Assert::AreEqual((uint32_t)0, p.header.payloadSize);
            Assert::IsNull(p.data);
        }

        // --- PROMPT 

        TEST_METHOD(Prompt_HasCorrectType)
        {
            Packet p = Packet::MakePromptPacket(1, "draw a cat", 0, 1);
            Assert::IsTrue(p.header.type == PacketType::PROMPT);
        }

        TEST_METHOD(Prompt_HasCorrectState)
        {
            Packet p = Packet::MakePromptPacket(1, "draw a cat", 0, 1);
            Assert::IsTrue(p.header.state == GameState::SENDING);
        }

        TEST_METHOD(Prompt_PayloadSizeIsStringLengthPlusOne)
        {
            std::string prompt = "hello world";
            Packet p = Packet::MakePromptPacket(1, prompt, 0, 1);
            Assert::AreEqual((uint32_t)(prompt.size() + 1), p.header.payloadSize);
        }

        TEST_METHOD(Prompt_GetPromptStringReturnsText)
        {
            Packet p = Packet::MakePromptPacket(1, "draw a cat", 0, 1);
            Assert::AreEqual(std::string("draw a cat"), p.GetPromptString());
        }

        TEST_METHOD(Prompt_CRCIsValid)
        {
            Packet p = Packet::MakePromptPacket(1, "draw a cat", 0, 1);
            Assert::IsTrue(p.ValidateCRC());
        }

        TEST_METHOD(Prompt_AddressesSetCorrectly)
        {
            Packet p = Packet::MakePromptPacket(5, "x", 3, 7);
            Assert::AreEqual((int)3, (int)p.header.srcAddress);
            Assert::AreEqual((int)7, (int)p.header.dstAddress);
        }

        // --- GAME_START / GAME_END

        TEST_METHOD(GameStart_HasCorrectType)
        {
            Packet p = Packet::MakeGameStartPacket(1, 0, 0);
            Assert::IsTrue(p.header.type == PacketType::GAME_START);
        }

        TEST_METHOD(GameStart_HasNoPayload)
        {
            Packet p = Packet::MakeGameStartPacket(1, 0, 0);
            Assert::AreEqual((uint32_t)0, p.header.payloadSize);
        }

        TEST_METHOD(GameEnd_HasCorrectType)
        {
            Packet p = Packet::MakeGameEndPacket(1, 0, 0);
            Assert::IsTrue(p.header.type == PacketType::GAME_END);
        }

        TEST_METHOD(GameEnd_HasCorrectState)
        {
            Packet p = Packet::MakeGameEndPacket(1, 0, 0);
            Assert::IsTrue(p.header.state == GameState::ENDING);
        }

        // --- ACK 

        TEST_METHOD(Ack_HasCorrectType)
        {
            Packet p = Packet::MakeAckPacket(1, 0, 2);
            Assert::IsTrue(p.header.type == PacketType::ACK);
        }

        TEST_METHOD(Ack_HasNoPayload)
        {
            Packet p = Packet::MakeAckPacket(1, 0, 0);
            Assert::AreEqual((uint32_t)0, p.header.payloadSize);
        }

        // --- ERROR 

        TEST_METHOD(Error_HasCorrectType)
        {
            Packet p = Packet::MakeErrorPacket(1, "auth failed", 0, 0);
            Assert::IsTrue(p.header.type == PacketType::GAME_ERROR);
        }

        TEST_METHOD(Error_GetErrorStringReturnsMessage)
        {
            Packet p = Packet::MakeErrorPacket(1, "auth failed", 0, 0);
            Assert::AreEqual(std::string("auth failed"), p.GetErrorString());
        }

        TEST_METHOD(Error_CRCIsValid)
        {
            Packet p = Packet::MakeErrorPacket(1, "bad credentials format", 0, 0);
            Assert::IsTrue(p.ValidateCRC());
        }

        // --- IMAGE

        TEST_METHOD(Image_HasCorrectType)
        {
            char data[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
            Packet p = Packet::MakeImagePacket(1, data, sizeof(data), 2, 0);
            Assert::IsTrue(p.header.type == PacketType::IMAGE);
        }

        TEST_METHOD(Image_PayloadSizeMatchesDataSize)
        {
            char data[256] = {};
            Packet p = Packet::MakeImagePacket(1, data, 256, 1, 0);
            Assert::AreEqual((uint32_t)256, p.header.payloadSize);
        }

        TEST_METHOD(Image_DataPreservedCorrectly)
        {
            char original[] = { 10, 20, 30, 40, 50 };
            Packet p = Packet::MakeImagePacket(1, original, 5, 1, 0);
            Assert::AreEqual((int)10, (int)(unsigned char)p.data[0]);
            Assert::AreEqual((int)50, (int)(unsigned char)p.data[4]);
        }

        TEST_METHOD(Image_CRCIsValid)
        {
            char data[100];
            for (int i = 0; i < 100; i++) data[i] = (char)i;
            Packet p = Packet::MakeImagePacket(1, data, 100, 1, 0);
            Assert::IsTrue(p.ValidateCRC());
        }

        TEST_METHOD(Image_SrcAddressIsDrawerID)
        {
            char data[] = { 0, 1, 2, 3, 4 };
            Packet p = Packet::MakeImagePacket(1, data, 5, 3, 0); // src=3 = player 3
            Assert::AreEqual((int)3, (int)p.header.srcAddress);
        }

        // --- VOTE

        TEST_METHOD(Vote_HasCorrectType)
        {
            Packet p = Packet::MakeVotePacket(1, 2, 1, 0);
            Assert::IsTrue(p.header.type == PacketType::VOTE);
        }

        TEST_METHOD(Vote_HasCorrectState)
        {
            Packet p = Packet::MakeVotePacket(1, 2, 1, 0);
            Assert::IsTrue(p.header.state == GameState::VOTING);
        }

        TEST_METHOD(Vote_GetVotedPlayerIdReturnsCorrectId)
        {
            Packet p = Packet::MakeVotePacket(1, 2, 1, 0);
            Assert::AreEqual((int)2, (int)p.GetVotedPlayerId());
        }

        TEST_METHOD(Vote_PayloadSizeIsOne)
        {
            Packet p = Packet::MakeVotePacket(1, 3, 2, 0);
            Assert::AreEqual((uint32_t)1, p.header.payloadSize);
        }

        TEST_METHOD(Vote_SrcAddressIsVoterID)
        {
            Packet p = Packet::MakeVotePacket(1, 2, 5, 0); // src=5
            Assert::AreEqual((int)5, (int)p.header.srcAddress);
        }

        // --- VOTE_REQUEST

        TEST_METHOD(VoteRequest_HasCorrectType)
        {
            Packet p = Packet::MakeVoteRequestPacket(1, "1:Alice\n2:Bob", 0, 0);
            Assert::IsTrue(p.header.type == PacketType::VOTE_REQUEST);
        }

        TEST_METHOD(VoteRequest_GetPayloadStringReturnsPlayerList)
        {
            Packet p = Packet::MakeVoteRequestPacket(1, "1:Alice\n2:Bob", 0, 0);
            Assert::AreEqual(std::string("1:Alice\n2:Bob"), p.GetPayloadString());
        }

        TEST_METHOD(VoteRequest_CRCIsValid)
        {
            Packet p = Packet::MakeVoteRequestPacket(1, "1:Alice\n2:Bob\n3:Charlie", 0, 0);
            Assert::IsTrue(p.ValidateCRC());
        }

        // --- RESULTS

        TEST_METHOD(Results_HasCorrectType)
        {
            Packet p = Packet::MakeResultsPacket(1, "Alice wins!", 0, 0);
            Assert::IsTrue(p.header.type == PacketType::RESULTS);
        }

        TEST_METHOD(Results_GetPromptStringReturnsResultsText)
        {
            Packet p = Packet::MakeResultsPacket(1, "Bob wins with 2 votes!", 0, 0);
            Assert::AreEqual(std::string("Bob wins with 2 votes!"), p.GetPromptString());
        }

        TEST_METHOD(Results_CRCIsValid)
        {
            Packet p = Packet::MakeResultsPacket(1, "=== VOTING RESULTS ===\nAlice wins!\n", 0, 0);
            Assert::IsTrue(p.ValidateCRC());
        }

        // --- CRC edge cases

        TEST_METHOD(ValidateCRC_NoPayload_ReturnsTrue)
        {
            Packet p = Packet::MakeAckPacket(1, 0, 0);
            Assert::IsTrue(p.ValidateCRC()); 
        }

        TEST_METHOD(ValidateCRC_ShortPayload_ReturnsTrue)
        {
            Packet p = Packet::MakeVotePacket(1, 2, 1, 0);
            Assert::IsTrue(p.ValidateCRC());
        }

        TEST_METHOD(ValidateCRC_CorruptedData_ReturnsFalse)
        {
            Packet p = Packet::MakePromptPacket(1, "draw something here", 0, 1);
            // Flip a byte in the payload
            p.data[0] ^= 0xFF;
            Assert::IsFalse(p.ValidateCRC());
        }

        // --- Serialize / Deserialize

        TEST_METHOD(Serialize_TotalSizeEqualsHeaderPlusPayload)
        {
            std::string prompt = "test prompt";
            Packet p = Packet::MakePromptPacket(1, prompt, 0, 1);
            uint32_t outSize = 0;
            char* buf = p.Serialize(outSize);
            Assert::AreEqual((uint32_t)(sizeof(PacketHeader) + p.header.payloadSize), outSize);
            delete[] buf;
        }

        TEST_METHOD(SerializeDeserialize_NoPayload_HeaderPreserved)
        {
            Packet original = Packet::MakeGameStartPacket(7, 1, 2);
            uint32_t sz = 0;
            char* buf = original.Serialize(sz);
            Packet copy = Packet::Deserialize(buf, sz);
            delete[] buf;

            Assert::IsTrue(copy.header.type == PacketType::GAME_START);
            Assert::AreEqual((int)7, (int)copy.header.sessionID);
            Assert::AreEqual((int)1, (int)copy.header.srcAddress);
            Assert::AreEqual((int)2, (int)copy.header.dstAddress);
        }

        TEST_METHOD(SerializeDeserialize_StringPayload_RoundTrip)
        {
            Packet original = Packet::MakePromptPacket(1, "A cat riding a bicycle", 0, 3);
            uint32_t sz = 0;
            char* buf = original.Serialize(sz);
            Packet copy = Packet::Deserialize(buf, sz);
            delete[] buf;

            Assert::AreEqual(std::string("A cat riding a bicycle"), copy.GetPromptString());
            Assert::IsTrue(copy.ValidateCRC());
        }

        TEST_METHOD(SerializeDeserialize_BinaryPayload_RoundTrip)
        {
            char imageData[200];
            for (int i = 0; i < 200; i++) imageData[i] = (char)(i % 256);

            Packet original = Packet::MakeImagePacket(1, imageData, 200, 2, 0);
            uint32_t sz = 0;
            char* buf = original.Serialize(sz);
            Packet copy = Packet::Deserialize(buf, sz);
            delete[] buf;

            Assert::AreEqual((uint32_t)200, copy.header.payloadSize);
            // Spot check a few bytes
            Assert::AreEqual((int)(unsigned char)imageData[0],  (int)(unsigned char)copy.data[0]);
            Assert::AreEqual((int)(unsigned char)imageData[99], (int)(unsigned char)copy.data[99]);
            Assert::AreEqual((int)(unsigned char)imageData[199],(int)(unsigned char)copy.data[199]);
            Assert::IsTrue(copy.ValidateCRC());
        }

        TEST_METHOD(SerializeDeserialize_VotePacket_RoundTrip)
        {
            Packet original = Packet::MakeVotePacket(1, 2, 1, 0);
            uint32_t sz = 0;
            char* buf = original.Serialize(sz);
            Packet copy = Packet::Deserialize(buf, sz);
            delete[] buf;

            Assert::IsTrue(copy.header.type == PacketType::VOTE);
            Assert::AreEqual((int)2, (int)copy.GetVotedPlayerId());
        }

        TEST_METHOD(CopyConstructor_DeepCopiesPayload)
        {
            Packet original = Packet::MakePromptPacket(1, "original text", 0, 1);
            Packet copy = original; // copy constructor
            // Mutating original's data should not affect copy
            original.data[0] = 'X';
            Assert::AreEqual(std::string("original text"), copy.GetPromptString());
        }
    };

    TEST_CLASS(PlayerTests)
    {
    public:

        TEST_METHOD(DefaultConstructor_ZeroValues)
        {
            Player p;
            Assert::AreEqual(std::string(""), p.GetName());
            Assert::AreEqual(0, p.GetId());
            Assert::AreEqual(0, p.GetScore());
            Assert::AreEqual(-1, p.GetSocketIndex());
        }

        TEST_METHOD(ParameterizedConstructor_SetsAllFields)
        {
            Player p("Alice", 1, 5);
            Assert::AreEqual(std::string("Alice"), p.GetName());
            Assert::AreEqual(1, p.GetId());
            Assert::AreEqual(5, p.GetSocketIndex());
            Assert::AreEqual(0, p.GetScore()); // starts at 0
        }

        TEST_METHOD(AddScore_IncreasesScore)
        {
            Player p("Bob", 2, 0);
            p.AddScore(100);
            Assert::AreEqual(100, p.GetScore());
        }

        TEST_METHOD(AddScore_Accumulates)
        {
            Player p("Bob", 2, 0);
            p.AddScore(50);
            p.AddScore(30);
            Assert::AreEqual(80, p.GetScore());
        }

        TEST_METHOD(AddScore_Zero_NoChange)
        {
            Player p("Bob", 2, 0);
            p.AddScore(0);
            Assert::AreEqual(0, p.GetScore());
        }
    };

    TEST_CLASS(GameTests)
    {
    public:

        // --- Session / state 

        TEST_METHOD(GetSessionID_ReturnsConstructedID)
        {
            Game g(7);
            Assert::AreEqual((int)7, (int)g.GetSessionID());
        }

        TEST_METHOD(InitialState_IsStartup)
        {
            Game g(1);
            Assert::IsTrue(g.GetChangeState() == GameState::STARTUP);
        }

        TEST_METHOD(ChangeState_UpdatesToNewState)
        {
            Game g(1);
            g.ChangeState(GameState::WAITING);
            Assert::IsTrue(g.GetChangeState() == GameState::WAITING);
        }

        TEST_METHOD(ChangeState_ThroughAllStates)
        {
            Game g(1);
            g.ChangeState(GameState::SENDING);
            Assert::IsTrue(g.GetChangeState() == GameState::SENDING);
            g.ChangeState(GameState::RECEIVING);
            Assert::IsTrue(g.GetChangeState() == GameState::RECEIVING);
            g.ChangeState(GameState::VOTING);
            Assert::IsTrue(g.GetChangeState() == GameState::VOTING);
            g.ChangeState(GameState::ENDING);
            Assert::IsTrue(g.GetChangeState() == GameState::ENDING);
        }

        TEST_METHOD(ProgramRunning_TrueByDefault)
        {
            Game g(1);
            Assert::IsTrue(g.ProgramRunning());
        }

        TEST_METHOD(ChangeProgramRunning_SetsFalse)
        {
            Game g(1);
            g.ChangeProgramRunning(false);
            Assert::IsFalse(g.ProgramRunning());
        }

        // --- Players

        TEST_METHOD(AddPlayer_IncreasesPlayerCount)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0); 
            Assert::AreEqual(1, g.GetPlayerCount());
        }

        TEST_METHOD(AddPlayer_TwoPlayers_CountIsTwo)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            Assert::AreEqual(2, g.GetPlayerCount());
        }

        TEST_METHOD(GetPlayer_ReturnsCorrectPlayer)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            Assert::AreEqual(std::string("Alice"), g.GetPlayer(0).GetName());
            Assert::AreEqual(1, g.GetPlayer(0).GetId());
        }

        TEST_METHOD(FindPlayerById_ExistingId_ReturnsPlayer)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            const Player* p = g.FindPlayerById(2);
            Assert::IsNotNull(p);
            Assert::AreEqual(std::string("Bob"), p->GetName());
        }

        TEST_METHOD(FindPlayerById_NonexistentId_ReturnsNull)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            const Player* p = g.FindPlayerById(99);
            Assert::IsNull(p);
        }

        TEST_METHOD(AwardPoints_IncreasesPlayerScore)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AwardPoints(1, 100);
            Assert::AreEqual(100, g.GetPlayer(0).GetScore());
        }

        TEST_METHOD(AwardPoints_MultipleAwards_Accumulates)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AwardPoints(1, 50);
            g.AwardPoints(1, 75);
            Assert::AreEqual(125, g.GetPlayer(0).GetScore());
        }

        TEST_METHOD(AwardPoints_NonexistentPlayer_DoesNotCrash)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AwardPoints(99, 100); // should silently do nothing
            Assert::AreEqual(0, g.GetPlayer(0).GetScore());
        }

        // --- Prompt

        TEST_METHOD(SetAndGetPrompt_ReturnsCorrectString)
        {
            Game g(1);
            g.SetPrompt("A dog on the moon");
            Assert::AreEqual(std::string("A dog on the moon"), g.GetPrompt());
        }

        TEST_METHOD(SetPrompt_OverwritesPrevious)
        {
            Game g(1);
            g.SetPrompt("first prompt");
            g.SetPrompt("second prompt");
            Assert::AreEqual(std::string("second prompt"), g.GetPrompt());
        }

        // --- Drawer rotation

        TEST_METHOD(GetCurrentDrawer_ReturnsFirstPlayer)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            Assert::AreEqual(std::string("Alice"), g.GetCurrentDrawer().GetName());
        }

        TEST_METHOD(NextDrawer_AdvancesToSecondPlayer)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            g.NextDrawer();
            Assert::AreEqual(std::string("Bob"), g.GetCurrentDrawer().GetName());
        }

        TEST_METHOD(NextDrawer_WrapsAroundToFirstPlayer)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            g.NextDrawer(); // -> Bob
            g.NextDrawer(); // -> wraps back to Alice
            Assert::AreEqual(std::string("Alice"), g.GetCurrentDrawer().GetName());
        }

        // --- Image storage

        TEST_METHOD(HasPlayerImage_FalseBeforeStorage)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            Assert::IsFalse(g.HasPlayerImage(1));
        }

        TEST_METHOD(StorePlayerImage_HasImageReturnsTrue)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            char data[] = { 1, 2, 3, 4, 5 };
            g.StorePlayerImage(1, data, 5);
            Assert::IsTrue(g.HasPlayerImage(1));
        }

        TEST_METHOD(GetPlayerImage_ReturnsCorrectData)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            char data[] = { 10, 20, 30, 40, 50 };
            g.StorePlayerImage(1, data, 5);
            const auto& img = g.GetPlayerImage(1);
            Assert::AreEqual((size_t)5, img.size());
            Assert::AreEqual((int)10, (int)(unsigned char)img[0]);
            Assert::AreEqual((int)50, (int)(unsigned char)img[4]);
        }

        TEST_METHOD(StorePlayerImage_LargeImage_SizeCorrect)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            std::vector<char> big(1440000, 0xAB);
            g.StorePlayerImage(1, big.data(), (uint32_t)big.size());
            Assert::AreEqual((size_t)1440000, g.GetPlayerImage(1).size());
        }

        TEST_METHOD(StorePlayerImage_TwoPlayers_StoredSeparately)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            char dataA[] = { 1 };
            char dataB[] = { 2 };
            g.StorePlayerImage(1, dataA, 1);
            g.StorePlayerImage(2, dataB, 1);
            Assert::AreEqual((int)1, (int)(unsigned char)g.GetPlayerImage(1)[0]);
            Assert::AreEqual((int)2, (int)(unsigned char)g.GetPlayerImage(2)[0]);
        }

        // --- Voting

        TEST_METHOD(RecordVote_ValidVote_ReturnsTrue)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            Assert::IsTrue(g.RecordVote(1, 2));
        }

        TEST_METHOD(RecordVote_InvalidVotedForId_ReturnsFalse)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            Assert::IsFalse(g.RecordVote(1, 99));
        }

        TEST_METHOD(RecordVote_OverwritesPreviousVote)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            g.AddPlayer("Joe",   3, 2);
            g.RecordVote(1, 2); // Alice votes Bob
            g.RecordVote(1, 3); // Alice changes vote to Joe
            // GetWinnerId should reflect the updated vote
            int winner = g.GetWinnerId();
            Assert::AreEqual(3, winner); // Joe has the only vote
        }

        TEST_METHOD(GetWinnerId_NoVotes_ReturnsNegativeOne)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            Assert::AreEqual(-1, g.GetWinnerId());
        }

        TEST_METHOD(GetWinnerId_ClearWinner)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            g.AddPlayer("Joe",   3, 2);
            g.RecordVote(1, 2); // Alice -> Bob
            g.RecordVote(3, 2); // Joe   -> Bob
            Assert::AreEqual(2, g.GetWinnerId()); // Bob wins
        }

        TEST_METHOD(GetWinnerId_TieBreaksToLowestPlayerID)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            g.RecordVote(1, 2); // Alice -> Bob  (Bob gets 1 vote)
            g.RecordVote(2, 1); // Bob   -> Alice (Alice gets 1 vote)
            // Tie: both have 1 vote; lowest player ID wins
            Assert::AreEqual(1, g.GetWinnerId());
        }

        // --- Result strings

        TEST_METHOD(GetVoteRequestString_ContainsAllPlayerNamesAndIds)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            std::string s = g.GetVoteRequestString();
            Assert::IsTrue(s.find("1:Alice") != std::string::npos);
            Assert::IsTrue(s.find("2:Bob")   != std::string::npos);
        }

        TEST_METHOD(GetVoteRequestString_EntriesNewlineSeparated)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            std::string s = g.GetVoteRequestString();
            Assert::IsTrue(s.find('\n') != std::string::npos);
        }

        TEST_METHOD(GetResultsString_ContainsWinnerName)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            g.RecordVote(1, 2); // Bob wins
            std::string s = g.GetResultsString();
            Assert::IsTrue(s.find("Bob") != std::string::npos);
        }

        TEST_METHOD(GetResultsString_ContainsVoteTallySection)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            g.RecordVote(1, 2);
            std::string s = g.GetResultsString();
            Assert::IsTrue(s.find("Vote tally") != std::string::npos);
        }

        TEST_METHOD(GetResultsString_ContainsFinalScoresSection)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            g.RecordVote(1, 2);
            g.AwardPoints(2, 100); // award before getting results
            std::string s = g.GetResultsString();
            Assert::IsTrue(s.find("Final Scores") != std::string::npos);
        }

        TEST_METHOD(GetResultsString_NoVotesCast_SaysNoVotes)
        {
            Game g(1);
            g.AddPlayer("Alice", 1, 0);
            g.AddPlayer("Bob",   2, 1);
            std::string s = g.GetResultsString();
            Assert::IsTrue(s.find("No votes") != std::string::npos);
        }
    };

    TEST_CLASS(AccountManagerTests)
    {
    private:
        // Use unique filenames so parallel tests don't collide
        static const std::string FILE_AUTH;
        static const std::string FILE_CREATE;
        static const std::string FILE_DELETE;
        static const std::string FILE_UPDATE;
        static const std::string FILE_PERSIST;
        static const std::string FILE_ROUNDTRIP;

    public:

        TEST_METHOD(Authenticate_ValidCredentials_ReturnsTrue)
        {
            WriteAccountsFile(FILE_AUTH, "alice:pass123\n");
            AccountManager am(FILE_AUTH);
            Assert::IsTrue(am.Authenticate("alice", "pass123"));
            DeleteFile(FILE_AUTH);
        }

        TEST_METHOD(Authenticate_WrongPassword_ReturnsFalse)
        {
            WriteAccountsFile(FILE_AUTH, "alice:pass123\n");
            AccountManager am(FILE_AUTH);
            Assert::IsFalse(am.Authenticate("alice", "wrongpass"));
            DeleteFile(FILE_AUTH);
        }

        TEST_METHOD(Authenticate_NonexistentUser_ReturnsFalse)
        {
            WriteAccountsFile(FILE_AUTH, "alice:pass123\n");
            AccountManager am(FILE_AUTH);
            Assert::IsFalse(am.Authenticate("nobody", "pass123"));
            DeleteFile(FILE_AUTH);
        }

        TEST_METHOD(Authenticate_EmptyFile_ReturnsFalse)
        {
            WriteAccountsFile(FILE_AUTH, "");
            AccountManager am(FILE_AUTH);
            Assert::IsFalse(am.Authenticate("alice", "pass123"));
            DeleteFile(FILE_AUTH);
        }

        TEST_METHOD(CreateAccount_NewUsername_ReturnsTrue)
        {
            WriteAccountsFile(FILE_CREATE, "");
            AccountManager am(FILE_CREATE);
            Assert::IsTrue(am.CreateAccount("newuser", "newpass"));
            DeleteFile(FILE_CREATE);
        }

        TEST_METHOD(CreateAccount_DuplicateUsername_ReturnsFalse)
        {
            WriteAccountsFile(FILE_CREATE, "alice:pass123\n");
            AccountManager am(FILE_CREATE);
            Assert::IsFalse(am.CreateAccount("alice", "otherpass"));
            DeleteFile(FILE_CREATE);
        }

        TEST_METHOD(CreateAccount_ThenAuthenticate_Succeeds)
        {
            WriteAccountsFile(FILE_ROUNDTRIP, "");
            AccountManager am(FILE_ROUNDTRIP);
            am.CreateAccount("newuser", "mypassword");
            Assert::IsTrue(am.Authenticate("newuser", "mypassword"));
            DeleteFile(FILE_ROUNDTRIP);
        }

        TEST_METHOD(DeleteAccount_ExistingUser_ReturnsTrue)
        {
            WriteAccountsFile(FILE_DELETE, "alice:pass123\n");
            AccountManager am(FILE_DELETE);
            Assert::IsTrue(am.DeleteAccount("alice"));
            DeleteFile(FILE_DELETE);
        }

        TEST_METHOD(DeleteAccount_NonexistentUser_ReturnsFalse)
        {
            WriteAccountsFile(FILE_DELETE, "alice:pass123\n");
            AccountManager am(FILE_DELETE);
            Assert::IsFalse(am.DeleteAccount("nobody"));
            DeleteFile(FILE_DELETE);
        }

        TEST_METHOD(DeleteAccount_ThenAuthenticate_Fails)
        {
            WriteAccountsFile(FILE_DELETE, "alice:pass123\n");
            AccountManager am(FILE_DELETE);
            am.DeleteAccount("alice");
            Assert::IsFalse(am.Authenticate("alice", "pass123"));
            DeleteFile(FILE_DELETE);
        }

        TEST_METHOD(UpdatePassword_ExistingUser_ReturnsTrue)
        {
            WriteAccountsFile(FILE_UPDATE, "alice:pass123\n");
            AccountManager am(FILE_UPDATE);
            Assert::IsTrue(am.UpdatePassword("alice", "newpass"));
            DeleteFile(FILE_UPDATE);
        }

        TEST_METHOD(UpdatePassword_NonexistentUser_ReturnsFalse)
        {
            WriteAccountsFile(FILE_UPDATE, "alice:pass123\n");
            AccountManager am(FILE_UPDATE);
            Assert::IsFalse(am.UpdatePassword("nobody", "newpass"));
            DeleteFile(FILE_UPDATE);
        }

        TEST_METHOD(UpdatePassword_OldPasswordNoLongerWorks)
        {
            WriteAccountsFile(FILE_UPDATE, "alice:pass123\n");
            AccountManager am(FILE_UPDATE);
            am.UpdatePassword("alice", "newpass456");
            Assert::IsFalse(am.Authenticate("alice", "pass123"));
            DeleteFile(FILE_UPDATE);
        }

        TEST_METHOD(UpdatePassword_NewPasswordWorks)
        {
            WriteAccountsFile(FILE_UPDATE, "alice:pass123\n");
            AccountManager am(FILE_UPDATE);
            am.UpdatePassword("alice", "newpass456");
            Assert::IsTrue(am.Authenticate("alice", "newpass456"));
            DeleteFile(FILE_UPDATE);
        }

        TEST_METHOD(AccountExists_TrueForExistingUser)
        {
            WriteAccountsFile(FILE_AUTH, "alice:pass123\n");
            AccountManager am(FILE_AUTH);
            Assert::IsTrue(am.AccountExists("alice"));
            DeleteFile(FILE_AUTH);
        }

        TEST_METHOD(AccountExists_FalseForNonexistentUser)
        {
            WriteAccountsFile(FILE_AUTH, "alice:pass123\n");
            AccountManager am(FILE_AUTH);
            Assert::IsFalse(am.AccountExists("nobody"));
            DeleteFile(FILE_AUTH);
        }

        TEST_METHOD(Persistence_CreatedAccountSurvivedReload)
        {
            WriteAccountsFile(FILE_PERSIST, "");
            {
                AccountManager am(FILE_PERSIST);
                am.CreateAccount("persisteduser", "persistedpass");
            } 

            AccountManager am2(FILE_PERSIST);
            Assert::IsTrue(am2.Authenticate("persisteduser", "persistedpass"));
            DeleteFile(FILE_PERSIST);
        }

        TEST_METHOD(Persistence_DeletedAccountNotPresentAfterReload)
        {
            WriteAccountsFile(FILE_PERSIST, "alice:pass123\n");
            {
                AccountManager am(FILE_PERSIST);
                am.DeleteAccount("alice");
            }
            AccountManager am2(FILE_PERSIST);
            Assert::IsFalse(am2.AccountExists("alice"));
            DeleteFile(FILE_PERSIST);
        }

        TEST_METHOD(Persistence_UpdatedPasswordCorrectAfterReload)
        {
            WriteAccountsFile(FILE_PERSIST, "alice:oldpass\n");
            {
                AccountManager am(FILE_PERSIST);
                am.UpdatePassword("alice", "newpass");
            }
            AccountManager am2(FILE_PERSIST);
            Assert::IsTrue(am2.Authenticate("alice", "newpass"));
            Assert::IsFalse(am2.Authenticate("alice", "oldpass"));
            DeleteFile(FILE_PERSIST);
        }

        TEST_METHOD(MultipleAccounts_AllAuthenticate)
        {
            WriteAccountsFile(FILE_AUTH, "alice:pass1\nbob:pass2\njoe:pass3\n");
            AccountManager am(FILE_AUTH);
            Assert::IsTrue(am.Authenticate("alice", "pass1"));
            Assert::IsTrue(am.Authenticate("bob",   "pass2"));
            Assert::IsTrue(am.Authenticate("joe",   "pass3"));
            DeleteFile(FILE_AUTH);
        }

        TEST_METHOD(MultipleAccounts_WrongCredentialsFail)
        {
            WriteAccountsFile(FILE_AUTH, "alice:pass1\nbob:pass2\n");
            AccountManager am(FILE_AUTH);
            // Alice's password doesn't work for Bob
            Assert::IsFalse(am.Authenticate("bob", "pass1"));
            DeleteFile(FILE_AUTH);
        }
    };

    const std::string AccountManagerTests::FILE_AUTH      = "test_am_auth.txt";
    const std::string AccountManagerTests::FILE_CREATE    = "test_am_create.txt";
    const std::string AccountManagerTests::FILE_DELETE    = "test_am_delete.txt";
    const std::string AccountManagerTests::FILE_UPDATE    = "test_am_update.txt";
    const std::string AccountManagerTests::FILE_PERSIST   = "test_am_persist.txt";
    const std::string AccountManagerTests::FILE_ROUNDTRIP = "test_am_roundtrip.txt";

    TEST_CLASS(LoggerTests)
    {
    private:
        static const std::string LOG_FILE;

    public:

        TEST_METHOD(Log_CreatesFile)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SC_CONNECT, 1, true, "test"); }
            std::ifstream f(LOG_FILE);
            Assert::IsTrue(f.good());
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_ContainsSessionID)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SC_CONNECT, 5, true, ""); }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find("SID:5") != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_SuccessFlag_ShowsOK)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SL_LOGIN, 1, true, "alice"); }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find("OK") != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_FailureFlag_ShowsERR)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SL_LOGIN, 1, false, "baduser"); }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find("ERR") != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_StateTransition_ContainsXSTATE)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SC_XSTATE, 1, true, "WAITING"); }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find("SC_XSTATE") != std::string::npos);
            Assert::IsTrue(content.find("WAITING")   != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_Connect_ContainsSCCONNECT)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SC_CONNECT, 1, true, "socket 0"); }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find("SC_CONNECT") != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_Disconnect_ContainsSCDISCONNECT)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SC_DISCONNECT, 1, true, "Alice"); }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find("SC_DISCONNECT") != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_Login_ContainsSLLOGIN)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SL_LOGIN, 1, true, "Alice"); }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find("SL_LOGIN") != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_ServerSentJpeg_ContainsSSJPEG)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SS_JPEG, 1, true, "1440000 bytes"); }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find("SS_JPEG") != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_ServerRecvJpeg_ContainsSRJPEG)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SR_JPEG, 1, true, "800x600 image"); }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find("SR_JPEG") != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_ContainsExtraMessage)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SS_SEND, 1, true, "GAME_START broadcast"); }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find("GAME_START broadcast") != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_ContainsDateAndTime)
        {
            // ctime output always contains a colon (e.g. "14:05:30")
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SS_SEND, 1, true, ""); }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find(':') != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_MultipleEntries_AllWritten)
        {
            DeleteFile(LOG_FILE);
            {
                ::Logger log(LOG_FILE);
                log.Log(SC_CONNECT,    1, true,  "socket 0");
                log.Log(SL_LOGIN,      1, true,  "Alice");
                log.Log(SC_XSTATE,     1, true,  "SENDING");
                log.Log(SS_PRMT,       1, true,  "prompt sent");
                log.Log(SR_JPEG,       1, true,  "image received");
                log.Log(SC_DISCONNECT, 1, true,  "Alice");
            }
            std::string content = ReadFile(LOG_FILE);
            Assert::IsTrue(content.find("SC_CONNECT")    != std::string::npos);
            Assert::IsTrue(content.find("SL_LOGIN")      != std::string::npos);
            Assert::IsTrue(content.find("SC_XSTATE")     != std::string::npos);
            Assert::IsTrue(content.find("SS_PRMT")       != std::string::npos);
            Assert::IsTrue(content.find("SR_JPEG")       != std::string::npos);
            Assert::IsTrue(content.find("SC_DISCONNECT") != std::string::npos);
            DeleteFile(LOG_FILE);
        }

        TEST_METHOD(Log_AppendMode_DoesNotOverwritePreviousEntries)
        {
            DeleteFile(LOG_FILE);
            { ::Logger log(LOG_FILE); log.Log(SC_CONNECT, 1, true, "first"); }
            { ::Logger log(LOG_FILE); log.Log(SC_CONNECT, 1, true, "second"); }
            std::string content = ReadFile(LOG_FILE);

            size_t firstPos  = content.find("first");
            size_t secondPos = content.find("second");
            Assert::IsTrue(firstPos  != std::string::npos);
            Assert::IsTrue(secondPos != std::string::npos);
            DeleteFile(LOG_FILE);
        }
    };

    const std::string LoggerTests::LOG_FILE = "test_logger_temp.txt";

}
