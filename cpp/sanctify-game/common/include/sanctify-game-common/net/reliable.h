// clang-format off
/*
    reliable.io

    Copyright © 2017 - 2019, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
// clang-format on

#ifndef SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_NET_RELIABLE_H
#define SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_NET_RELIABLE_H

/**
 * This is a C++ port of the reliable.io C project, with a few things specific
 * to the Sanctify game project.
 *
 * For the original implementation, see
 * https://github.com/networkprotocol/reliable
 *
 * It builds a reliable packet protocol over UDP, though obviously in the cases
 * of massive packet loss (lag spike) it will discard outdated packets. For a
 * reasonable update rate though, it should perform nicely.
 */

#include <igcore/maybe.h>
#include <igcore/vector.h>

#include <chrono>
#include <cstdint>
#include <functional>

namespace sanctify::common::reliable {

enum class ReliableEndpointCounter : uint8_t {
  NumPacketsSent,
  NumPacketsReceived,
  NumPacketsAcked,
  NumPacketsStale,
  NumPacketsInvalid,
  NumPacketsTooLargeToSend,
  NumPacketsTooLargeToReceive,
  NumFragmentsSent,
  NumFragmentsReceived,
  NumFragmentsInvalid,

  NumCounters,
};

bool reliable_sequence_greater_than(uint16_t s1, uint16_t s2);

bool reliable_sequence_less_than(uint16_t s1, uint16_t s2);

/**
 * Configuration used for a the local size of a two-way semi-reliable connection
 * build on an unreliable connection.
 */
struct ReliableConfig {
  int Index;
  int MaxPacketSize;
  int FragmentAbove;
  int MaxFragments;
  int FragmentSize;
  int AckBufferSize;
  int SentPacketsBufferSize;
  int ReceivedPacketsBufferSize;
  int FragmentReassmeblyBufferSize;

  float RTTSmoothingFactor;
  float PacketLossSmoothingFactor;
  float BandwidthSmoothingFactor;

  int PacketHeaderSize;

  // Original Reliable library allowed overriding alloc/free - we won't

  static ReliableConfig GetDefaultConfig();
};

// Data on a sent packet - how many bytes, was it acked, and at what time
struct ReliableSentPacketData {
  double Time;
  uint32_t Acked : 1;
  uint32_t PacketBytes : 31;
};

// Data on a received packet - how many bytes, and what time was it received at
struct ReliableReceivedPacketData {
  double Time;
  uint32_t PacketBytes;
};

// Data on a fragment - which sequence it belongs to, ack field, how many
// fragments, packet data
struct ReliableFragmentReassemblyData {
  uint16_t Sequence;
  uint16_t Ack;
  uint16_t AckBits;

  int NumFragmentsReceived;
  int NumFragmentsTotal;
  uint8_t* PacketData;
  int PacketBytes;
  int PacketHeaderBytes;
  uint8_t FragmentReceived[256];

  ReliableFragmentReassemblyData();
  ~ReliableFragmentReassemblyData();
  ReliableFragmentReassemblyData(ReliableFragmentReassemblyData&&) = default;
  ReliableFragmentReassemblyData& operator=(ReliableFragmentReassemblyData&&) =
      default;
  ReliableFragmentReassemblyData(const ReliableFragmentReassemblyData&) =
      delete;
  ReliableFragmentReassemblyData& operator=(
      const ReliableFragmentReassemblyData&) = delete;

  void store_fragment_data(uint16_t sequence, uint16_t ack, uint32_t ack_bits,
                           int fragment_id, int fragment_size,
                           uint8_t* fragment_data, int fragment_bytes);

  static void CleanupFn(void* data);
};

// Sequence buffer - contains data about an individual sequenced packet
struct ReliableSequenceBuffer {
  ReliableSequenceBuffer(uint32_t num_entries, uint32_t stride);
  ~ReliableSequenceBuffer();
  ReliableSequenceBuffer(const ReliableSequenceBuffer&) = delete;
  ReliableSequenceBuffer& operator=(const ReliableSequenceBuffer&) = delete;
  ReliableSequenceBuffer(ReliableSequenceBuffer&&) = default;
  ReliableSequenceBuffer& operator=(ReliableSequenceBuffer&&) = default;

  uint16_t Sequence;
  int NumEntries;
  int EntryStride;
  uint32_t* EntrySequence;
  uint8_t* EntryData;

  void* insert(uint16_t sequence);
  void* find(uint16_t sequence);
  bool exists(uint16_t sequence);
  void reset();

  void remove_entries(int start_sequence, int finish_sequence,
                      std::function<void(void*)> cleanup_fn);

  bool test_insert(uint16_t sequence);
  // TODO
  void* insert_with_cleanup(uint16_t, std::function<void(void*)> cleanup_fn);
  void advance_with_cleanup(uint16_t sequence,
                            std::function<void(void*)> cleanup_fn);
  void advance(uint16_t sequence);
};

// Endpoint - hook this up to an individual connection (send/receive). Notice
// this should be hooked up only to a WebRTC (unreliable) connection - there's
// no point in including this over a WebSocket connection.
struct ReliableEndpoint {
  ReliableConfig Config;

  double Time;
  float RTT;
  float PacketLoss;
  float SentBandwidthKbps;
  float ReceivedBandwidthKbps;
  float AckedBandwidthKbps;
  int NumAcks;

  // Callback parameters:
  // - uint16_t sequence
  // - uint8_t* packet_data
  // - int packet_size
  // Returns bool if the packet was successfully received AND CONSUMED - called
  // function MUST copy
  //  any memory it intends to keep around (or parse it inline)
  std::function<bool(uint16_t, uint8_t*, int)> TransmitPacketFunction;
  std::function<bool(uint16_t, uint8_t*, int)> ProcessPacketFunction;

  uint16_t* Acks;
  uint16_t Sequence;

  ReliableSequenceBuffer* SentPackets;
  ReliableSequenceBuffer* ReceivedPackets;
  ReliableSequenceBuffer* FragmentReassembly;
  uint64_t Counters[(uint8_t)ReliableEndpointCounter::NumCounters];

  ReliableEndpoint();
  ReliableEndpoint(
      const ReliableConfig& config, double time,
      std::function<bool(uint16_t, uint8_t*, int)> transmit_packet_function,
      std::function<bool(uint16_t, uint8_t*, int)> process_packet_function);
  ~ReliableEndpoint();

  ReliableEndpoint(ReliableEndpoint&&) = default;
  ReliableEndpoint& operator=(ReliableEndpoint&&) = default;
  ReliableEndpoint(const ReliableEndpoint&) = delete;
  ReliableEndpoint& operator=(const ReliableEndpoint&) = delete;

  uint16_t next_packet_sequence();
  bool send_packet(uint8_t* data, int num_bytes);
  bool receive_packet(uint8_t* packet_data, int packet_bytes);

  void generate_ack_bits(ReliableSequenceBuffer* sequence_buffer, uint16_t* ack,
                         uint32_t* ack_bits);

  void update(double time);
};

int read_packet_header(uint8_t* packet_data, int packet_bytes,
                       uint16_t* sequence, uint16_t* ack, uint32_t* ack_bits);
int write_packet_header(uint8_t* data, uint16_t sequence, uint16_t ack,
                        uint16_t ack_bits);

int read_fragment_header(uint8_t* packet_data, int packet_bytes,
                         int max_fragments, int fragment_size, int* fragment_id,
                         int* num_fragments, int* fragment_bytes,
                         uint16_t* sequence, uint16_t* ack, uint32_t* ack_bits);

void write_uint8(uint8_t** data, uint8_t value);
void write_uint16(uint8_t** data, uint16_t value);
void write_uint32(uint8_t** data, uint32_t value);
uint8_t read_uint8(uint8_t** data);
uint16_t read_uint16(uint8_t** data);
uint32_t read_uint32(uint8_t** data);

}  // namespace sanctify::common::reliable

#endif
