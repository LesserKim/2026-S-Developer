#include "pch.h"
// 소스 파일 최상단에 추가
#pragma execution_character_set("utf-8")

const char* g_pszIP = "192.168.0.11";
const WORD g_wPort = 42345;

const int REQ_RESUME = 1001;
const int CMD_RESUME_ACK = 1002;
const int REQ_INQUIRY = 1003;
const int CMD_INQUIRY = 1004;
const int REQ_ANSWER = 1005;
const int CMD_ANSWER_ACK = 1006;

// UTF-16(wstring) -> UTF-8(string)
std::string WStringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty())
        return "";

    int sizeNeeded = WideCharToMultiByte(
        CP_UTF8,
        0,
        wstr.c_str(),
        (int)wstr.size(),   // null 제외한 실제 길이
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (sizeNeeded <= 0)
        return "";

    std::string result(sizeNeeded, 0);

    int converted = WideCharToMultiByte(
        CP_UTF8,
        0,
        wstr.c_str(),
        (int)wstr.size(),
        result.data(),
        sizeNeeded,
        nullptr,
        nullptr
    );

    if (converted <= 0)
        return "";

    return result;
}

// UTF-8(string) -> UTF-16(wstring)
std::wstring Utf8ToWString(const std::string& str)
{
    if (str.empty())
        return L"";

    int sizeNeeded = MultiByteToWideChar(
        CP_UTF8,
        0,
        str.c_str(),
        (int)str.size(),   // null 제외
        nullptr,
        0
    );

    if (sizeNeeded <= 0)
        return L"";

    std::wstring result(sizeNeeded, 0);

    int converted = MultiByteToWideChar(
        CP_UTF8,
        0,
        str.c_str(),
        (int)str.size(),
        result.data(),
        sizeNeeded
    );

    if (converted <= 0)
        return L"";

    return result;
}

// 전체 전송
bool SendAll(SOCKET sock, const char* pData, int size)
{
    int totalSent = 0;

    while (totalSent < size)
    {
        int sent = send(sock, pData + totalSent, size - totalSent, 0);

        if (sent == SOCKET_ERROR)
        {
            printf("send fail, error: %d\n", WSAGetLastError());
            return false;
        }

        if (sent == 0)
        {
            printf("send result return\n");
            return false;
        }

        totalSent += sent;
    }

    return true;
}

void PrintUtf8Line(const std::string& s)
{
    printf("%s\n", s.c_str());
}

// 전체 수신
bool RecvAll(SOCKET sock, char* pData, int size)
{
    int totalRecv = 0;

    while (totalRecv < size)
    {
        int recvBytes = recv(sock, pData + totalRecv, size - totalRecv, 0);

        if (recvBytes == 0)
        {
            wprintf(L"Server finished Connection.\n");
            return false;
        }

        if (recvBytes == SOCKET_ERROR)
        {
            wprintf(L"recv fail errorcode: %d\n", WSAGetLastError());
            return false;
        }

        totalRecv += recvBytes;
    }

    return true;
}

// 패킷 전송
bool SendPacket(SOCKET sock, int packetId, const std::string& body)
{
    int bodyLen = (int)body.size();

    char header[8] = { 0 };
    memcpy(header, &packetId, 4);
    memcpy(header + 4, &bodyLen, 4);

    if (!SendAll(sock, header, 8))
    {
        wprintf(L"Packet header send fail\n");
        return false;
    }

    if (bodyLen > 0)
    {
        if (!SendAll(sock, body.c_str(), bodyLen))
        {
            wprintf(L"Packet body send fail\n");
            return false;
        }
    }

    return true;
}

// 패킷 수신
bool RecvPacket(SOCKET sock, int& packetId, std::string& body)
{
    char header[8] = { 0 };

    if (!RecvAll(sock, header, 8))
    {
        wprintf(L"packet header receive fail\n");
        return false;
    }


    int bodyLen = 0;
    memcpy(&packetId, header, 4);
    memcpy(&bodyLen, header + 4, 4);

    if (bodyLen < 0)
    {
        wprintf(L"wrong body length \n");
        return false;
    }

    body.clear();

    if (bodyLen > 0)
    {
        std::vector<char> buffer(bodyLen + 1, 0);

        if (!RecvAll(sock, buffer.data(), bodyLen))
        {
            wprintf(L"packet body receive fail\n");
            return false;
        }

        body.assign(buffer.data(), bodyLen);
    }

    return true;
}

std::string EscapeJsonString(const std::string& s)
{
    std::string result;
    result.reserve(s.size() * 2);

    for (char ch : s)
    {
        switch (ch)
        {
        case '\"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b";  break;
        case '\f': result += "\\f";  break;
        case '\n': result += "\\n";  break;
        case '\r': result += "\\r";  break;
        case '\t': result += "\\t";  break;
        default:
            result += ch;
            break;
        }
    }

    return result;
}
bool AnswerSender(SOCKET sock, int questionIdx, const std::wstring& wAnswer)
{
    std::string answerUtf8 = WStringToUtf8(wAnswer);
    std::string escapedAnswer = EscapeJsonString(answerUtf8);

    std::string answerBody =
        "{"
        "\"QuestionIdx\":" + std::to_string(questionIdx) + ","
        "\"Answer\":\"" + escapedAnswer + "\""
        "}";

    printf("\n[%d Question solution sending]\n", questionIdx);
    printf("REQ_ANSWER sending ready\n");
    printf("Packet-ID: %d\n", REQ_ANSWER);
    printf("Length   : %d\n", (int)answerBody.size());
    printf("Body     : %s\n", answerBody.c_str());

    if (!SendPacket(sock, REQ_ANSWER, answerBody))
    {
        printf("REQ_ANSWER Send Fail\n");
        return false;
    }

    printf("REQ_ANSWER Send Success!\n");
    return true;
}

int main()
{
    WSADATA wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup fail\n");
        return -1;
    }

    setlocale(LC_ALL, "");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        printf("Socket Create Fail \n");
        ::WSACleanup();
        return -1;
    }
    wprintf(L"Socket Create Success\n");

    // 서버 주소 설정
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(g_wPort);

    if (inet_pton(AF_INET, g_pszIP, &serverAddr.sin_addr) != 1)
    {
        wprintf(L"IP address exchange fail\n");
        closesocket(sock);
        ::WSACleanup();
        return -1;
    }

    // 서버 연결
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        wprintf(L"Server Connect Fail ErrorCode: %d\n", WSAGetLastError());
        closesocket(sock);
        ::WSACleanup();
        return -1;
    }

    wprintf(L"Server Connect Success (%s:%d)\n", g_pszIP, g_wPort);

    // =========================
    // REQ_RESUME(1001)
    // =========================
    std::wstring wResumeBody =
        L"{"
        L"\"Resume\":{"
        L"\"Name\":\"\uCD5C\uD558\uACBD\","
        L"\"Greeting\":\"\uC548\uB155\uD558\uC138\uC694\","
        L"\"Introduction\":\"\uC0B4\uB824\uC8FC\uC138\uC694\""
        L"}"
        L"}";

    std::string resumeBody = WStringToUtf8(wResumeBody);

    if (resumeBody.empty())
    {
        printf("REQ_RESUME UTF-8 exchange fail\n");
        closesocket(sock);
        ::WSACleanup();
        return -1;
    }

    printf("\nREQ_RESUME sending\n");
    printf("Packet-ID: %d\n", REQ_RESUME);
    printf("Length   : %d\n", (int)resumeBody.size());

    if (!SendPacket(sock, REQ_RESUME, resumeBody))
    {
        printf("REQ_RESUME send fail\n");
        closesocket(sock);
        ::WSACleanup();
        return -1;
    }

    printf("REQ_RESUME send success!\n");

    // =========================
    // CMD_RESUME_ACK(1002)
    // =========================
    int recvPacketId = 0;
    std::string recvBody;

    if (!RecvPacket(sock, recvPacketId, recvBody))
    {
        printf("CMD_RESUME_ACK receive fail\n");
        closesocket(sock);
        ::WSACleanup();
        return -1;
    }

    printf("\n[Server response receive]\n");
    printf("Packet-ID: %d\n", recvPacketId);
    printf("Length   : %d\n", (int)recvBody.size());

    if (recvPacketId != CMD_RESUME_ACK)
    {
        printf("Not CMD_RESUME_ACK(1002).\n");
        closesocket(sock);
        ::WSACleanup();
        return -1;
    }

    std::wstring wResumeAck = Utf8ToWString(recvBody);
    wprintf(L"[Server Response Body]\n%ls\n", wResumeAck.c_str());

    // =========================
// REQ_INQUIRY(1003)
// =========================
    std::wstring wInquiryBody =
        L"{"
        L"\"QuestionCode\":\"s-developer 4th\""
        L"}";

    std::string inquiryBody = WStringToUtf8(wInquiryBody);

    if (inquiryBody.empty())
    {
        printf("REQ_INQUIRY UTF-8 exchange fail\n");
        closesocket(sock);
        ::WSACleanup();
        return -1;
    }

    printf("\nREQ_INQUIRY sending\n");
    printf("Packet-ID: %d\n", REQ_INQUIRY);
    printf("Length   : %d\n", (int)inquiryBody.size());

    if (!SendPacket(sock, REQ_INQUIRY, inquiryBody))
    {
        printf("REQ_INQUIRY send fail \n");
        closesocket(sock);
        ::WSACleanup();
        return -1;
    }

    printf("REQ_INQUIRY send success!\n");

    // =========================
    // CMD_INQUIRY(1004)
    // =========================
    int recvPacketId2 = 0;
    std::string recvBody2;

    if (!RecvPacket(sock, recvPacketId2, recvBody2))
    {
        printf("CMD_INQUIRY receive fail\n");
        closesocket(sock);
        ::WSACleanup();
        return -1;
    }

    printf("\n[Question response header receive]\n");
    printf("Packet-ID: %d\n", recvPacketId2);
    printf("Length   : %d\n", (int)recvBody2.size());

    if (recvPacketId2 != CMD_INQUIRY)
    {
        printf("Not CMD_INQUIRY(1004).\n");
        std::wstring wUnknown = Utf8ToWString(recvBody2);
        wprintf(L"[수신 바디]\n%ls\n", wUnknown.c_str());

        closesocket(sock);
        ::WSACleanup();
        return -1;
    }

    std::wstring wInquiryResponse = Utf8ToWString(recvBody2);

    wprintf(L"[Response Body]\n%ls\n", wInquiryResponse.c_str());
    printf("Question Success\n");

    
    // 정답 전송하기 
    if (!AnswerSender(sock, 1, L"\/some\\\\path")) 
        return -1;

    if (!AnswerSender(sock, 2, L"가급적 사용하지 않는 것이 좋다."))
        return -1;

    if (!AnswerSender(sock, 3, L"char형은 유니코드 문자열이 아니다."))
        return -1;
    
    if (!AnswerSender(sock, 4, L"대표헤더 파일에서 공통헤더 파일을 include 한다."))
        return -1;

    if (!AnswerSender(sock, 5, L"이 오류는 링커에서 발생한다."))
        return -1;

    if (!AnswerSender(sock, 6, L"OPEN_ALWAYS는 지정한 경로에 파일이 없을 경우에 실패한다."))
        return -1;

    if (!AnswerSender(sock, 7, L"ZVWXYZVWXYZVWXYZVWXYZVWXYZ"))
        return -1;
        
    if (!AnswerSender(sock, 8, L"map은 key-value 쌍들로 이루어져 있으며 begin에는 가장 작은 키의 원소를 end에는 가장 큰 키의 원소를 조회할 수있다."))
        return -1;

    if (!AnswerSender(sock, 9, L"Serializer는 Formatter의 하위 개념으로 parser와 동일한 의미를 가진다."))
    return -1;

    if (!AnswerSender(sock, 10, L"UDP 소켓에서 recv할 때 0이 반환되는 경우가 존재한다.(상대방이 0바이트를 send한 경우는 제외)"))
        return -1;


     closesocket(sock);
     ::WSACleanup();

     return 0;

}

