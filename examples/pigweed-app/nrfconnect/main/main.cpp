/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "AppConfig.h"
#include "LEDWidget.h"
#include "PwNetworkTestHelpers.h"
#include <kernel.h>

#include <array>
#include <span>
#include <string_view>

#include "pw_hdlc_lite/encoder.h"
#include "pw_hdlc_lite/rpc_channel.h"
#include "pw_hdlc_lite/rpc_packets.h"
#include "pw_hdlc_lite/sys_io_stream.h"
#include "pw_log/log.h"
#include "pw_rpc/channel.h"
#include "pw_rpc/echo_service_nanopb.h"
#include "pw_rpc/server.h"

#include "pw_sys_io/sys_io.h"
#include "pw_sys_io_nrfconnect/init.h"
#include "transport/SecureSessionMgr.h"
#include "transport/raw/MessageHeader.h"
#include "transport/raw/PeerAddress.h"
#include "main/pigweed_test.rpc.pb.h"

#include <dk_buttons_and_leds.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(app);

static LEDWidget sStatusLED;

namespace {
    class PigweedRpcTransport;
}

namespace chip {
namespace rpc {

namespace {
    pw::rpc::ServerWriter<chip_rpc_Response> response_writer;
}

class TestService final : public generated::TestService<TestService> {
 private:
    PigweedRpcTransport *transport = nullptr;
 public:

  void SetTransport(PigweedRpcTransport *t) {
      transport = t;
  }

  void OpenTransport(ServerContext& ctx,
                 const chip_rpc_Transport& transportnumber,
                 ServerWriter<chip_rpc_Response>& response) {
    // implementation
    response_writer = std::move(response);
  }

  // This is actually 'send' on the host side. On the device side it
  // is 'receive'.
  pw::Status SendMessage(ServerContext& ctx,
                       const chip_rpc_PacketOnTransport& packet_on_transport,
                       chip_rpc_Empty& response);

  pw::Status MessageTest(ServerContext& ctx,
                       const chip_rpc_Transport& transportnumber,
                       chip_rpc_Empty& response);
};

}  // namespace rpc
}  // namespace chip

namespace {

#define RPC_STACK_SIZE (8*1024)
#define RPC_PRIORITY 5

#define TEST_STACK_SIZE (8*1024)
#define TEST_PRIORITY 5

K_THREAD_STACK_DEFINE(rpc_stack_area, RPC_STACK_SIZE);
struct k_thread rpc_thread_data;

K_THREAD_STACK_DEFINE(test_stack_area, TEST_STACK_SIZE);
struct k_thread test_thread_data;

using std::byte;

constexpr size_t kMaxTransmissionUnit = 1500;

// Used to write HDLC data to pw::sys_io.
pw::stream::SysIoWriter writer;

struct k_mutex uart_mutex;

template <size_t buffer_size>
class ChipRpcChannelOutputBuffer : public pw::rpc::ChannelOutput {
 public:
  constexpr ChipRpcChannelOutputBuffer(pw::stream::Writer& writer,
                                       uint8_t address,
                                       const char* channel_name)
      : pw::rpc::ChannelOutput(channel_name), writer_(writer), address_(address) {}

  std::span<std::byte> AcquireBuffer() override {
      k_mutex_lock(&uart_mutex, K_FOREVER);
      return buffer_;
  }

  pw::Status SendAndReleaseBuffer(std::span<const std::byte> buffer) override {
    PW_DASSERT(buffer.data() == buffer_.data());
    if (buffer.empty()) {
      k_mutex_unlock(&uart_mutex);
      return pw::Status::Ok();
    }
    pw::Status ret = pw::hdlc_lite::WriteInformationFrame(address_, buffer, writer_);
    k_mutex_unlock(&uart_mutex);
    return ret;
  }

 private:
  pw::stream::Writer& writer_;
  std::array<std::byte, buffer_size> buffer_;
  const uint8_t address_;
};


// Set up the output channel for the pw_rpc server to use to use.
ChipRpcChannelOutputBuffer<kMaxTransmissionUnit> hdlc_channel_output(
    writer, pw::hdlc_lite::kDefaultRpcAddress, "HDLC channel");

pw::rpc::Channel channels[] = {
    pw::rpc::Channel::Create<1>(&hdlc_channel_output)};

// Declare the pw_rpc server with the HDLC channel.
pw::rpc::Server server(channels);

pw::rpc::EchoService echo_service;
chip::rpc::TestService test_service;

void RegisterServices() {
    server.RegisterService(echo_service);
    server.RegisterService(test_service);
}

void Start() {
  // Send log messages to HDLC address 1. This prevents logs from interfering
  // with pw_rpc communications.
  pw::log_basic::SetOutput([](std::string_view log) {
    k_mutex_lock(&uart_mutex, K_FOREVER);
    pw::hdlc_lite::WriteInformationFrame(
        1, std::as_bytes(std::span(log)), writer);
    k_mutex_unlock(&uart_mutex);
  });

  // Set up the server and start processing data.
  RegisterServices();

  // Declare a buffer for decoding incoming HDLC frames.
  std::array<std::byte, kMaxTransmissionUnit> input_buffer;

  PW_LOG_INFO("Starting pw_rpc server");
  pw::hdlc_lite::ReadAndProcessPackets(
      server, hdlc_channel_output, input_buffer);
}

void RunRpcService(void *, void *, void *)
{
    Start();
}


using TestContext = chip::PwTest::IOContext;
template <typename T>
using Optional    = chip::Optional<T>;
using PeerAddress = chip::Transport::PeerAddress;

TestContext sContext;

const char PAYLOAD[]                      = "Hello!";
constexpr chip::NodeId kSourceNodeId      = 123654;
constexpr chip::NodeId kDestinationNodeId = 111222333;

class PigweedRpcTransport : public chip::Transport::Base
{
public:
    /// Transports are required to have a constructor that takes exactly one argument
    CHIP_ERROR Init(const char * unused) {
        test_service.SetTransport(this);
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SendMessage(const chip::PacketHeader & header, const chip::Transport::PeerAddress & address,
                           chip::System::PacketBuffer * msgBuf) override
    {
        chip::System::PacketBufferHandle msg_ForNow;
        msg_ForNow.Adopt(msgBuf);
        chip_rpc_Response resp{0};
        std::strncpy(reinterpret_cast<char*>(resp.packet.bytes),
                    reinterpret_cast<const char*>(msg_ForNow->Start()),
                    msg_ForNow->DataLength());
        resp.packet.size = msg_ForNow->DataLength();
        // PW_LOG_INFO("data=%02x%02x%02x%02x",
        //     resp.packet.bytes[0],
        //     resp.packet.bytes[1],
        //     resp.packet.bytes[2],
        //     resp.packet.bytes[3],
        //     );
        if (chip::rpc::response_writer.open()) {
            chip::rpc::response_writer.Write(resp);
        }
        return CHIP_NO_ERROR;
    }

    bool CanSendToPeer(const PeerAddress & address) override {
        return chip::rpc::response_writer.open();
    }

    void HandleMessageReceived(const chip_rpc_PacketOnTransport& packet_on_transport) {
        chip::PacketHeader dummyPacketHeader;
        chip::Transport::PeerAddress dummyPeerAddress;
        chip::System::PacketBufferHandle buffer =
            chip::System::PacketBuffer::NewWithAvailableSize(packet_on_transport.packet.size);
        memcpy(buffer->Start(), packet_on_transport.packet.bytes, packet_on_transport.packet.size);

        chip::Transport::Base::HandleMessageReceived(
            dummyPacketHeader, dummyPeerAddress, std::move(buffer));
    }
};

class TestSessMgrCallback : public chip::SecureSessionMgrDelegate
{
public:
    void OnMessageReceived(const chip::PacketHeader & packetHeader, const chip::PayloadHeader & payloadHeader,
                                   const chip::Transport::PeerConnectionState * state, chip::System::PacketBufferHandle msgBuf,
                                   chip::SecureSessionMgr * mgr) override
    {
        //        NL_TEST_ASSERT(mSuite, header.GetSourceNodeId() == Optional<NodeId>::Value(kSourceNodeId));
        //        NL_TEST_ASSERT(mSuite, header.GetDestinationNodeId() == Optional<NodeId>::Value(kDestinationNodeId));
        //        NL_TEST_ASSERT(mSuite, state->GetPeerNodeId() == kSourceNodeId);

        size_t data_len = msgBuf->DataLength();

        int compare = memcmp(msgBuf->Start(), PAYLOAD, data_len);
        //        NL_TEST_ASSERT(mSuite, compare == 0);

        ReceiveHandlerCallCount++;
    }

    void OnNewConnection(const chip::Transport::PeerConnectionState * state, chip::SecureSessionMgr * mgr) override
    {
        NewConnectionHandlerCallCount++;
    }

    int ReceiveHandlerCallCount       = 0;
    int NewConnectionHandlerCallCount = 0;
};

TestSessMgrCallback callback;

void CheckMessageTest(void * inContext, void *, void *)
{
    TestContext & ctx = *reinterpret_cast<TestContext *>(inContext);
//     CHIP_ERROR err = ctx.Init();

//     uint16_t payload_len = sizeof(PAYLOAD);

//     ctx.GetInetLayer().SystemLayer()->Init(nullptr);

//     chip::System::PacketBufferHandle buffer = chip::System::PacketBuffer::NewWithAvailableSize(payload_len);
// //    NL_TEST_ASSERT(inSuite, !buffer.IsNull());

//     memmove(buffer->Start(), PAYLOAD, payload_len);
//     buffer->SetDataLength(payload_len);

//     chip::Inet::IPAddress addr;
//     chip::Inet::IPAddress::FromString("127.0.0.1", addr);
//     err = CHIP_NO_ERROR;

//     chip::TransportMgr<PigweedRpcTransport> transportMgr;
//     chip::SecureSessionMgr secureSessionMgr;

//     err = transportMgr.Init("LOOPBACK");
// //    NL_TEST_ASSERT(inSuite, err == CHIP_NO_ERROR);
//     auto sl = ctx.GetInetLayer().SystemLayer();
//     err = secureSessionMgr.Init(kSourceNodeId, sl, &transportMgr);
// //    NL_TEST_ASSERT(inSuite, err == CHIP_NO_ERROR);

// //    callback.mSuite = inSuite;

//     secureSessionMgr.SetDelegate(&callback);

//     chip::SecurePairingUsingTestSecret pairing1(Optional<chip::NodeId>::Value(kSourceNodeId), 1, 2);
//     Optional<chip::Transport::PeerAddress> peer(chip::Transport::PeerAddress::UDP(addr, CHIP_PORT));

//     err = secureSessionMgr.NewPairing(peer, kDestinationNodeId, &pairing1);
//     // NL_TEST_ASSERT(inSuite, err == CHIP_NO_ERROR);

//     chip::SecurePairingUsingTestSecret pairing2(Optional<chip::NodeId>::Value(kDestinationNodeId), 2, 1);
//     err = secureSessionMgr.NewPairing(peer, kSourceNodeId, &pairing2);
//     // NL_TEST_ASSERT(inSuite, err == CHIP_NO_ERROR);

//     // Should be able to send a message to itself by just calling send.
//     callback.ReceiveHandlerCallCount = 0;

//     err = secureSessionMgr.SendMessage(kDestinationNodeId, std::move(buffer));
//     // NL_TEST_ASSERT(inSuite, err == CHIP_NO_ERROR);

//     ctx.DriveIOUntil(1000 /* ms */, []() { return callback.ReceiveHandlerCallCount != 0; });

//     // NL_TEST_ASSERT(inSuite, callback.ReceiveHandlerCallCount == 1);

//     err = ctx.Shutdown();

    chip_rpc_Response resp{0};
    std::strncpy(reinterpret_cast<char*>(resp.packet.bytes),
                PAYLOAD,
                sizeof(PAYLOAD));
    resp.packet.size = sizeof(PAYLOAD);
    if (chip::rpc::response_writer.open()) {
        chip::rpc::response_writer.Write(resp);
    }

}

} // namespace

namespace chip {
namespace rpc {

pw::Status TestService::SendMessage(ServerContext& ctx,
                    const chip_rpc_PacketOnTransport& packet_on_transport,
                    chip_rpc_Empty& response) {
    // implementation
    PW_LOG_INFO("TestService::SendMessage");
    if (transport) {
        transport->HandleMessageReceived(packet_on_transport);
    }
    return pw::Status::OK;
}


pw::Status TestService::MessageTest(ServerContext& ctx,
                                    const chip_rpc_Transport& transportnumber,
                                    chip_rpc_Empty& response) {
    // implementation in a new thread
    k_thread_create(&test_thread_data, test_stack_area, K_THREAD_STACK_SIZEOF(test_stack_area), CheckMessageTest,
                                    &sContext, NULL, NULL, TEST_PRIORITY, 0, K_NO_WAIT);
//    CheckMessageTest(&sContext, NULL, NULL);
    return pw::Status::OK;
}

}
}

int main()
{
    k_mutex_init(&uart_mutex);
    pw_sys_io_Init();

    LOG_INF("==================================================");
    LOG_INF("chip-nrf52840-pigweed-example starting");
#if BUILD_RELEASE
    LOG_INF("*** PSEUDO-RELEASE BUILD ***");
#else
    LOG_INF("*** DEVELOPMENT BUILD ***");
#endif
    LOG_INF("==================================================");

    // Light up the LED as a visual feedback that the flash was
    // successful.
    LEDWidget::InitGpio();
    sStatusLED.Init(SYSTEM_STATE_LED);
    sStatusLED.Set(true);

    k_tid_t my_tid = k_thread_create(&rpc_thread_data, rpc_stack_area, K_THREAD_STACK_SIZEOF(rpc_stack_area), RunRpcService,
                                     NULL, NULL, NULL, RPC_PRIORITY, 0, K_NO_WAIT);
    k_thread_join(&rpc_thread_data, K_FOREVER);
    return 0;
}
