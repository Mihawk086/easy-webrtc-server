#include <cstdio>
#include <iostream>
#include <map>

#include "common/utils.h"
#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"
#include "muduo/net/http/HttpServer.h"
#include "rtc/dtls_transport.h"
#include "rtc/rtp_maker.h"
#include "rtc/srtp_session.h"
#include "rtc/udp_socket.h"
#include "rtc/webrtc_transport.h"

using namespace muduo;
using namespace muduo::net;

static std::map<int, std::shared_ptr<WebRtcTransport>> s_rtc_sessions;
static int s_sessionid = 1;

int main(int argc, char* argv[]) {
  std::string ip = "192.168.2.128";
  if (argc > 1) {
    ip = argv[1];
  }
  Utils::Crypto::ClassInit();
  RTC::DtlsTransport::ClassInit();
  RTC::DepLibSRTP::ClassInit();
  RTC::SrtpSession::ClassInit();

  int threads_num = 0;
  EventLoop loop;
  HttpServer server(&loop, InetAddress(8000), "webrtc", TcpServer::kReusePort);
  server.setHttpCallback([&loop, &ip](const HttpRequest& req, HttpResponse* resp) {
    if (req.path() == "/publish") {
      resp->setStatusCode(HttpResponse::k200Ok);
      resp->setStatusMessage("OK");
      resp->setContentType("text/plain");
      resp->addHeader("Access-Control-Allow-Origin", "*");
      std::shared_ptr<WebRtcTransport> session(new WebRtcTransport(&loop, ip));
      s_rtc_sessions.insert(std::make_pair(s_sessionid, session));
      s_sessionid++;
      session->Start();
      resp->setBody(session->GetPublishSdp());
      std::cout << session->GetPublishSdp() << std::endl;
    }
  });
  server.setThreadNum(threads_num);
  server.start();
  loop.loop();
}
