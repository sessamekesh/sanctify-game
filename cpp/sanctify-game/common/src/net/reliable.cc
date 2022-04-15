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

#include <igcore/log.h>
#include <sanctify-game-common/net/reliable.h>

#include <cfloat>
#include <cmath>
#include <cstring>

using namespace std;
using namespace indigo;
using namespace core;
using namespace sanctify;
using namespace common;
using namespace reliable;

namespace {
const char* kLogLabel = "ReliableUDP";

const int kMaxPacketHeaderBytes = 9;
const int kFragmentHeaderBytes = 5;

}  // namespace

bool reliable::reliable_sequence_greater_than(uint16_t s1, uint16_t s2) {
  return ((s1 > s2) && (s1 - s2 <= 32768)) || ((s1 < s2) && (s2 - s1 > 32768));
}

bool reliable::reliable_sequence_less_than(uint16_t s1, uint16_t s2) {
  return reliable_sequence_greater_than(s2, s1);
}

ReliableFragmentReassemblyData::ReliableFragmentReassemblyData()
    : Sequence(0u),
      Ack(0u),
      AckBits(0u),
      NumFragmentsReceived(0u),
      NumFragmentsTotal(0u),
      PacketData(nullptr),
      PacketBytes(0u),
      PacketHeaderBytes(0u) {
  std::memset(FragmentReceived, 0, sizeof(FragmentReceived));
}

ReliableFragmentReassemblyData::~ReliableFragmentReassemblyData() {
  if (PacketData) {
    delete[] PacketData;
    PacketData = nullptr;
  }
}

ReliableSequenceBuffer::ReliableSequenceBuffer(uint32_t num_entries,
                                               uint32_t stride)
    : Sequence(0u),
      NumEntries(num_entries),
      EntryStride(stride),
      EntrySequence(new uint32_t[num_entries]),
      EntryData(new uint8_t[num_entries * stride]) {
  std::memset(EntrySequence, 0xFF, sizeof(uint32_t) * num_entries);
  std::memset(EntryData, 0, sizeof(uint8_t) * num_entries * stride);
}

ReliableSequenceBuffer::~ReliableSequenceBuffer() {
  if (EntrySequence) {
    delete[] EntrySequence;
  }
  if (EntryData) {
    delete[] EntryData;
  }

  EntrySequence = nullptr;
  EntryData = nullptr;
}

void ReliableSequenceBuffer::reset() {
  Sequence = 0;
  std::memset(EntrySequence, 0xFF, sizeof(uint32_t) * NumEntries);
}

void* ReliableSequenceBuffer::insert(uint16_t sequence) {
  if (reliable_sequence_less_than(sequence,
                                  Sequence - ((uint16_t)NumEntries))) {
    return nullptr;
  }

  if (reliable_sequence_greater_than(sequence + 1, Sequence)) {
    remove_entries(Sequence, sequence, nullptr);
    Sequence = sequence + 1;
  }

  int index = sequence % NumEntries;
  EntrySequence[index] = sequence;
  return EntryData + index * EntryStride;
}

void* ReliableSequenceBuffer::find(uint16_t sequence) {
  int index = sequence % NumEntries;
  return ((EntrySequence[index] == (uint32_t)sequence))
             ? (EntryData + index * EntryStride)
             : nullptr;
}

bool ReliableSequenceBuffer::exists(uint16_t sequence) {
  return EntrySequence[sequence % NumEntries] == sequence;
}

void ReliableSequenceBuffer::remove_entries(
    int start_sequence, int finish_sequence,
    std::function<void(void*)> cleanup_fn) {
  if (finish_sequence < start_sequence) {
    finish_sequence += 65536;
  }
  if (finish_sequence - start_sequence < NumEntries) {
    for (int sequence = start_sequence; sequence <= finish_sequence;
         sequence++) {
      if (cleanup_fn) {
        cleanup_fn(EntryData + EntryStride * (sequence % NumEntries));
      }
      EntrySequence[sequence % NumEntries] = 0xFFFFFFFF;
    }
  } else {
    for (int i = 0; i < NumEntries; i++) {
      if (cleanup_fn) {
        cleanup_fn(EntryData + EntryStride * i);
      }
      EntrySequence[i] = 0xFFFFFFFF;
    }
  }
}

bool ReliableSequenceBuffer::test_insert(uint16_t sequence) {
  return reliable_sequence_less_than(sequence,
                                     Sequence - ((uint16_t)NumEntries));
}

void* ReliableSequenceBuffer::insert_with_cleanup(
    uint16_t sequence, std::function<void(void*)> cleanup_fn) {
  if (reliable_sequence_greater_than(sequence + 1, Sequence)) {
    remove_entries(Sequence, sequence, cleanup_fn);
    Sequence = sequence + 1;
  } else if (reliable_sequence_less_than(sequence,
                                         Sequence - ((uint16_t)NumEntries))) {
    return nullptr;
  }

  int index = sequence % NumEntries;
  if (EntrySequence[index] != 0xFFFFFFFF) {
    cleanup_fn(EntryData + EntryStride * index);
  }
  EntrySequence[index] = sequence;
  return EntryData + index * EntryStride;
}

void ReliableSequenceBuffer::advance_with_cleanup(
    uint16_t sequence, std::function<void(void*)> cleanup_fn) {
  if (reliable_sequence_greater_than(sequence + 1, Sequence)) {
    remove_entries(Sequence, sequence, cleanup_fn);
    Sequence = sequence + 1;
  }
}

void ReliableSequenceBuffer::advance(uint16_t sequence) {
  if (reliable_sequence_greater_than(sequence + 1, Sequence)) {
    remove_entries(Sequence, sequence, nullptr);
    Sequence = sequence + 1;
  }
}

// NOTICE: Some values were taken from
// https://stackoverflow.com/questions/11934499/webrtc-overhead
//  and should not be taken as a reference.
ReliableConfig ReliableConfig::GetDefaultConfig() {
  ReliableConfig config{};
  config.MaxPacketSize = 32 * 1024;
  config.FragmentAbove = 1280;
  config.MaxFragments = 32;
  config.FragmentSize = 1280;
  config.AckBufferSize = 256;
  config.SentPacketsBufferSize = 256;
  config.ReceivedPacketsBufferSize = 256;
  config.FragmentReassmeblyBufferSize = 32;
  config.RTTSmoothingFactor = 0.0025f;
  config.PacketLossSmoothingFactor = 0.1f;
  config.BandwidthSmoothingFactor = 0.1f;

  // NOTICE: This value is a guesstimate from StackOverflow
  config.PacketHeaderSize = 120;

  return config;
}

ReliableEndpoint::ReliableEndpoint()
    : Config({}),
      Time(0.),
      RTT(0.f),
      PacketLoss(0.f),
      SentBandwidthKbps(0.f),
      ReceivedBandwidthKbps(0.f),
      AckedBandwidthKbps(0.f),
      NumAcks(0),
      SentPackets(nullptr),
      ReceivedPackets(nullptr),
      FragmentReassembly(nullptr),
      Acks(nullptr),
      TransmitPacketFunction(nullptr),
      ProcessPacketFunction(nullptr),
      Sequence(0x0000) {
  std::memset(Counters, 0x00, sizeof(Counters));
}

ReliableEndpoint::ReliableEndpoint(
    const ReliableConfig& config, double time,
    std::function<bool(uint16_t, uint8_t*, int)> transmit_packet_function,
    std::function<bool(uint16_t, uint8_t*, int)> process_packet_function)
    : Config(config),
      Time(time),
      RTT(0.f),
      PacketLoss(0.f),
      SentBandwidthKbps(0.f),
      ReceivedBandwidthKbps(0.f),
      AckedBandwidthKbps(0.f),
      NumAcks(0),
      SentPackets(new ReliableSequenceBuffer(config.SentPacketsBufferSize,
                                             sizeof(ReliableSentPacketData))),
      ReceivedPackets(
          new ReliableSequenceBuffer(config.ReceivedPacketsBufferSize,
                                     sizeof(ReliableReceivedPacketData))),
      FragmentReassembly(
          new ReliableSequenceBuffer(config.FragmentReassmeblyBufferSize,
                                     sizeof(ReliableFragmentReassemblyData))),
      Acks(new uint16_t[config.AckBufferSize]),
      TransmitPacketFunction(transmit_packet_function),
      ProcessPacketFunction(process_packet_function),
      Sequence(0x0000) {
  std::memset(Acks, 0x00, config.AckBufferSize * sizeof(uint16_t));
}

ReliableEndpoint::~ReliableEndpoint() {
  if (SentPackets != nullptr) {
    delete SentPackets;
    SentPackets = nullptr;
  }
  if (ReceivedPackets != nullptr) {
    delete ReceivedPackets;
    ReceivedPackets = nullptr;
  }
  if (FragmentReassembly != nullptr) {
    delete FragmentReassembly;
    FragmentReassembly = nullptr;
  }
  if (Acks != nullptr) {
    delete[] Acks;
    Acks = nullptr;
  }
}

uint16_t ReliableEndpoint::next_packet_sequence() { return Sequence; }

void ReliableFragmentReassemblyData::CleanupFn(void* data) {
  ReliableFragmentReassemblyData* reassembly_data =
      (ReliableFragmentReassemblyData*)data;

  if (reassembly_data->PacketData) {
    delete[] reassembly_data->PacketData;
    reassembly_data->PacketData = nullptr;
  }
}

bool ReliableEndpoint::send_packet(uint8_t* data, int num_bytes) {
  if (num_bytes > Config.MaxPacketSize) {
    Logger::err(kLogLabel) << "Packet size is too large: packet is "
                           << num_bytes << " bytes, maximum is "
                           << Config.MaxPacketSize;
    Counters[(uint8_t)ReliableEndpointCounter::NumPacketsTooLargeToSend]++;
    return false;
  }

  uint16_t sequence = next_packet_sequence();
  uint16_t ack;
  uint32_t ack_bits;

  generate_ack_bits(ReceivedPackets, &ack, &ack_bits);

  ReliableSentPacketData* sent_packet_data =
      (ReliableSentPacketData*)SentPackets->insert(sequence);

  if (!sent_packet_data) {
    Logger::err(kLogLabel) << "Unexpected case - sent_packet_data empty";
    return false;
  }

  if (!sent_packet_data) {
    Logger::err(kLogLabel)
        << "Failed to insert packet into sent packets buffer (size: "
        << num_bytes << ", sequence_id " << sequence << ")";
    return false;
  }

  sent_packet_data->Time = Time;
  sent_packet_data->PacketBytes = Config.PacketHeaderSize + num_bytes;
  sent_packet_data->Acked = false;

  if (num_bytes <= Config.FragmentAbove) {
    // Regular packet - do not fragment, just send it
    uint8_t* transmit_packet_data =
        new uint8_t[num_bytes + kMaxPacketHeaderBytes];
    int packet_header_bytes =
        write_packet_header(transmit_packet_data, sequence, ack, ack_bits);
    memcpy(transmit_packet_data + packet_header_bytes, data, num_bytes);
    TransmitPacketFunction(sequence, transmit_packet_data,
                           packet_header_bytes + num_bytes);
    delete[] transmit_packet_data;
  } else {
    // Fragmented packet
    uint8_t packet_header[kMaxPacketHeaderBytes];
    memset(packet_header, 0x00, kMaxPacketHeaderBytes);
    int packet_header_bytes =
        write_packet_header(packet_header, sequence, ack, ack_bits);
    int num_fragments = (num_bytes + Config.FragmentSize) +
                        ((num_bytes % Config.FragmentSize) != 0 ? 1 : 0);
    if (num_fragments < 1) {
      Logger::err(kLogLabel)
          << "Nonsense case hit: packet of size " << num_bytes
          << " is tring to generate " << num_fragments << " fragments";
      return false;
    }
    if (num_fragments > Config.MaxFragments) {
      Logger::err(kLogLabel)
          << "Packet of size " << num_bytes << " is too large to be sent in "
          << num_fragments << " fragments, maximum is " << Config.MaxFragments;
      return false;
    }

    uint8_t* fragment_packet_data = new uint8_t[Config.FragmentSize];
    uint8_t* q = data;
    uint8_t* end = q + num_bytes;

    for (int fragment_id = 0; fragment_id < num_fragments; fragment_id++) {
      uint8_t* p = fragment_packet_data;

      write_uint8(&p, 1);
      write_uint16(&p, sequence);
      write_uint8(&p, (uint8_t)fragment_id);
      write_uint8(&p, (uint8_t)(num_fragments - 1));

      if (fragment_id == 0) {
        memcpy(p, packet_header, packet_header_bytes);
        p += packet_header_bytes;
      }

      int bytes_to_copy = Config.FragmentSize;
      if (q + bytes_to_copy > end) {
        bytes_to_copy = (int)(end - q);
      }

      memcpy(p, q, bytes_to_copy);
      p += bytes_to_copy;
      q += bytes_to_copy;

      int fragment_packet_bytes = (int)(p - fragment_packet_data);

      TransmitPacketFunction(sequence, fragment_packet_data,
                             fragment_packet_bytes);
      Counters[(uint8_t)ReliableEndpointCounter::NumFragmentsSent]++;
    }
    delete[] fragment_packet_data;
  }
  Counters[(uint8_t)ReliableEndpointCounter::NumPacketsSent]++;
  return true;
}

bool ReliableEndpoint::receive_packet(uint8_t* packet_data, int packet_bytes) {
  if (packet_bytes >
      Config.MaxPacketSize + kMaxPacketHeaderBytes + kFragmentHeaderBytes) {
    Logger::err(kLogLabel) << "Packet size is too large: packet is at least "
                           << packet_bytes -
                                  (kMaxPacketHeaderBytes + kFragmentHeaderBytes)
                           << " bytes, maximum is " << Config.MaxPacketSize;
    Counters[(uint8_t)ReliableEndpointCounter::NumPacketsTooLargeToReceive]++;
    return false;
  }

  uint8_t prefix_byte = packet_data[0];

  if ((prefix_byte & 1) == 0) {
    // Regular packet
    Counters[(uint8_t)ReliableEndpointCounter::NumPacketsReceived]++;

    uint16_t sequence;
    uint16_t ack;
    uint32_t ack_bits;

    int packet_header_bytes = read_packet_header(packet_data, packet_bytes,
                                                 &sequence, &ack, &ack_bits);
    if (packet_header_bytes < 0) {
      Logger::err(kLogLabel)
          << "Ignoring invalid packet - could not read packet header";
      Counters[(uint8_t)ReliableEndpointCounter::NumPacketsInvalid]++;
      return false;
    }

    if (packet_header_bytes > packet_bytes) {
      Logger::err(kLogLabel)
          << "Unexpected case hit - packet header exeeds packet size";
      return false;
    }

    int packet_payload_bytes = packet_bytes - packet_header_bytes;
    if (packet_payload_bytes > Config.MaxPacketSize) {
      Logger::err(kLogLabel)
          << "Packet payload is too large: packet is at least "
          << packet_payload_bytes << " bytes, maximum is "
          << Config.MaxPacketSize;
      Counters[(uint8_t)ReliableEndpointCounter::NumPacketsTooLargeToReceive]++;
      return false;
    }

    if (!ReceivedPackets->test_insert(sequence)) {
      Logger::err(kLogLabel) << "Ignoring stale packet " << sequence;
      Counters[(uint8_t)ReliableEndpointCounter::NumPacketsStale]++;
      // This is an okay return case - we just ignore the packet
      return true;
    }

    if (ProcessPacketFunction(sequence, packet_data + packet_header_bytes,
                              packet_bytes - packet_header_bytes)) {
      ReliableReceivedPacketData* received_packet_data =
          (ReliableReceivedPacketData*)ReceivedPackets->insert(sequence);

      if (!received_packet_data) {
        Logger::err(kLogLabel)
            << "Unexpected case - received_packet_data empty";
        return false;
      }

      FragmentReassembly->advance_with_cleanup(
          sequence, ReliableFragmentReassemblyData::CleanupFn);

      received_packet_data->Time = Time;
      received_packet_data->PacketBytes =
          Config.PacketHeaderSize + packet_bytes;

      for (int i = 0; i < 32; i++) {
        if (ack_bits & 1) {
          uint16_t ack_sequence = ack - ((uint16_t)i);

          ReliableSentPacketData* sent_packet_data =
              (ReliableSentPacketData*)SentPackets->find(ack_sequence);
          if (sent_packet_data && !sent_packet_data->Acked &&
              NumAcks < Config.AckBufferSize) {
            Acks[NumAcks++] = ack_sequence;
            Counters[(uint8_t)ReliableEndpointCounter::NumPacketsAcked]++;
            sent_packet_data->Acked = true;

            float rtt = (float)(Time - sent_packet_data->Time) * 1000.f;
            if ((RTT == 0.f && rtt > 0.f) || fabs(RTT - rtt) < 0.00001f) {
              RTT = rtt;
            } else {
              RTT += (rtt - RTT) * Config.RTTSmoothingFactor;
            }
          }
        }
        ack_bits >>= 1;
      }
    } else {
      Logger::err(kLogLabel) << "Processing packet failed";
      return false;
    }
  } else {
    // Fragment packet

    int fragment_id;
    int num_fragments;
    int fragment_bytes;

    uint16_t sequence;
    uint16_t ack;
    uint32_t ack_bits;

    int fragment_header_bytes =
        read_fragment_header(packet_data, packet_bytes, Config.MaxFragments,
                             Config.FragmentSize, &fragment_id, &num_fragments,
                             &fragment_bytes, &sequence, &ack, &ack_bits);

    if (fragment_header_bytes < 0) {
      Logger::err(kLogLabel)
          << "Ignoring invalid fragment, could not read fragment header";
      Counters[(uint8_t)ReliableEndpointCounter::NumFragmentsInvalid]++;
      return false;
    }

    ReliableFragmentReassemblyData* reassembly_data =
        (ReliableFragmentReassemblyData*)FragmentReassembly->find(sequence);

    if (!reassembly_data) {
      reassembly_data =
          (ReliableFragmentReassemblyData*)
              FragmentReassembly->insert_with_cleanup(
                  sequence, ReliableFragmentReassemblyData::CleanupFn);

      if (!reassembly_data) {
        Logger::err(kLogLabel) << "Ignoring invalid fragment, could not insert "
                                  "in reassembly buffer: stale";
        Counters[(uint8_t)ReliableEndpointCounter::NumFragmentsInvalid]++;
        return false;
      }

      ReceivedPackets->advance(sequence);

      int packet_buffer_size =
          kMaxPacketHeaderBytes + num_fragments * Config.FragmentSize;

      reassembly_data->Sequence = sequence;
      reassembly_data->Ack = 0;
      reassembly_data->AckBits = 0;
      reassembly_data->NumFragmentsReceived = 0;
      reassembly_data->NumFragmentsTotal = num_fragments;
      reassembly_data->PacketData = new uint8_t[packet_buffer_size];
      reassembly_data->PacketBytes = 0;
      memset(reassembly_data->FragmentReceived, 0x00,
             sizeof(reassembly_data->FragmentReceived));
    }

    if (num_fragments != (int)reassembly_data->NumFragmentsTotal) {
      Logger::err(kLogLabel)
          << "Ignoring invalid fragment - fragment count mismatch. Expected "
          << reassembly_data->NumFragmentsTotal << ", got " << num_fragments;
      Counters[(uint8_t)ReliableEndpointCounter::NumFragmentsInvalid]++;
      return false;
    }

    if (reassembly_data->FragmentReceived[fragment_id]) {
      Logger::err(kLogLabel)
          << "Ignoring fragment " << fragment_id << " of packet " << sequence
          << " - fragment already received!";
      return true;
    }

    reassembly_data->NumFragmentsReceived++;
    reassembly_data->FragmentReceived[fragment_id] = 1;

    reassembly_data->store_fragment_data(sequence, ack, ack_bits, fragment_id,
                                         Config.FragmentSize,
                                         packet_data + fragment_header_bytes,
                                         packet_bytes - fragment_header_bytes);

    if (reassembly_data->NumFragmentsReceived ==
        reassembly_data->NumFragmentsTotal) {
      receive_packet(
          reassembly_data->PacketData + kMaxPacketHeaderBytes -
              reassembly_data->PacketHeaderBytes,
          reassembly_data->PacketHeaderBytes + reassembly_data->PacketBytes);
    }

    Counters[(uint8_t)ReliableEndpointCounter::NumFragmentsReceived]++;
  }
  return true;
}

void ReliableEndpoint::generate_ack_bits(
    ReliableSequenceBuffer* sequence_buffer, uint16_t* ack,
    uint32_t* ack_bits) {
  *ack = sequence_buffer->Sequence - 1;
  *ack_bits = 0;

  uint32_t mask = 1;
  for (int i = 0; i < 32; i++) {
    uint16_t sequence = *ack - ((uint16_t)i);
    if (sequence_buffer->exists(sequence)) {
      *ack_bits |= mask;
    }
    mask <<= 1;
  }
}

void ReliableEndpoint::update(double time) {
  Time = time;

  // calculate packet loss
  {
    uint32_t base_sequence =
        (SentPackets->Sequence - Config.SentPacketsBufferSize + 1) + 0xFFFF;
    int i;
    int num_dropped = 0;
    int num_samples = Config.SentPacketsBufferSize / 2;
    for (i = 0; i < num_samples; ++i) {
      uint16_t sequence = (uint16_t)(base_sequence + i);
      ReliableSentPacketData* sent_packet_data =
          (ReliableSentPacketData*)SentPackets->find(sequence);
      if (sent_packet_data && !sent_packet_data->Acked) {
        num_dropped++;
      }
    }
    float packet_loss = ((float)num_dropped) / ((float)num_samples) * 100.0f;
    if (fabs(PacketLoss - packet_loss) > 0.00001) {
      PacketLoss +=
          (packet_loss - PacketLoss) * Config.PacketLossSmoothingFactor;
    } else {
      PacketLoss = packet_loss;
    }
  }

  // calculate sent bandwidth
  {
    uint32_t base_sequence =
        (SentPackets->Sequence - Config.SentPacketsBufferSize + 1) + 0xFFFF;
    int i;
    int bytes_sent = 0;
    double start_time = FLT_MAX;
    double finish_time = 0.0;
    int num_samples = Config.SentPacketsBufferSize / 2;
    for (i = 0; i < num_samples; ++i) {
      uint16_t sequence = (uint16_t)(base_sequence + i);
      ReliableSentPacketData* sent_packet_data =
          (ReliableSentPacketData*)SentPackets->find(sequence);
      if (!sent_packet_data) {
        continue;
      }
      bytes_sent += sent_packet_data->PacketBytes;
      if (sent_packet_data->Time < start_time) {
        start_time = sent_packet_data->Time;
      }
      if (sent_packet_data->Time > finish_time) {
        finish_time = sent_packet_data->Time;
      }
    }
    if (start_time != FLT_MAX && finish_time != 0.0) {
      float sent_bandwidth_kbps =
          (float)(((double)bytes_sent) / (finish_time - start_time) * 8.0f /
                  1024.0f);  // I WILL DIE ON THIS FUCKING HILL
      if (fabs(SentBandwidthKbps - sent_bandwidth_kbps) > 0.00001) {
        SentBandwidthKbps += (sent_bandwidth_kbps - SentBandwidthKbps) *
                             Config.BandwidthSmoothingFactor;
      } else {
        SentBandwidthKbps = sent_bandwidth_kbps;
      }
    }
  }

  // calculate received bandwidth
  {
    uint32_t base_sequence =
        (ReceivedPackets->Sequence - Config.ReceivedPacketsBufferSize + 1) +
        0xFFFF;
    int i;
    int bytes_sent = 0;
    double start_time = FLT_MAX;
    double finish_time = 0.0;
    int num_samples = Config.ReceivedPacketsBufferSize / 2;
    for (i = 0; i < num_samples; ++i) {
      uint16_t sequence = (uint16_t)(base_sequence + i);
      ReliableReceivedPacketData* received_packet_data =
          (ReliableReceivedPacketData*)ReceivedPackets->find(sequence);
      if (!received_packet_data) {
        continue;
      }
      bytes_sent += received_packet_data->PacketBytes;
      if (received_packet_data->Time < start_time) {
        start_time = received_packet_data->Time;
      }
      if (received_packet_data->Time > finish_time) {
        finish_time = received_packet_data->Time;
      }
    }
    if (start_time != FLT_MAX && finish_time != 0.0) {
      float received_bandwidth_kbps =
          (float)(((double)bytes_sent) / (finish_time - start_time) * 8.0f /
                  1024.0f);  // I WILL DIE ON THIS FUCKING HILL
      if (fabs(ReceivedBandwidthKbps - received_bandwidth_kbps) > 0.00001) {
        ReceivedBandwidthKbps +=
            (received_bandwidth_kbps - ReceivedBandwidthKbps) *
            Config.BandwidthSmoothingFactor;
      } else {
        ReceivedBandwidthKbps = received_bandwidth_kbps;
      }
    }
  }

  // calculate acked bandwidth
  {
    uint32_t base_sequence =
        (SentPackets->Sequence - Config.SentPacketsBufferSize + 1) + 0xFFFF;
    int i;
    int bytes_sent = 0;
    double start_time = FLT_MAX;
    double finish_time = 0.0;
    int num_samples = Config.SentPacketsBufferSize / 2;
    for (i = 0; i < num_samples; ++i) {
      uint16_t sequence = (uint16_t)(base_sequence + i);
      ReliableSentPacketData* sent_packet_data =
          (ReliableSentPacketData*)SentPackets->find(sequence);
      if (!sent_packet_data || !sent_packet_data->Acked) {
        continue;
      }
      bytes_sent += sent_packet_data->PacketBytes;
      if (sent_packet_data->Time < start_time) {
        start_time = sent_packet_data->Time;
      }
      if (sent_packet_data->Time > finish_time) {
        finish_time = sent_packet_data->Time;
      }
    }
    if (start_time != FLT_MAX && finish_time != 0.0) {
      float acked_bandwidth_kbps =
          (float)(((double)bytes_sent) / (finish_time - start_time) * 8.0f /
                  1024.0f);  // I WILL DIE ON THIS FUCKING HILL
      if (fabs(AckedBandwidthKbps - acked_bandwidth_kbps) > 0.00001) {
        AckedBandwidthKbps += (acked_bandwidth_kbps - AckedBandwidthKbps) *
                              Config.BandwidthSmoothingFactor;
      } else {
        AckedBandwidthKbps = acked_bandwidth_kbps;
      }
    }
  }
}

int reliable::read_packet_header(uint8_t* packet_data, int packet_bytes,
                                 uint16_t* sequence, uint16_t* ack,
                                 uint32_t* ack_bits) {
  if (packet_bytes < 3) {
    Logger::err(kLogLabel) << "Packet too small to contain packet header (1) ";
    return -1;
  }

  uint8_t* p = packet_data;
  uint8_t prefix_byte = read_uint8(&p);
  if ((prefix_byte & 1) != 0) {
    Logger::err(kLogLabel) << "Prefix byte does not indicate a regular packet";
    return -1;
  }

  *sequence = read_uint16(&p);

  if (prefix_byte & (1 << 5)) {
    if (packet_bytes < 3 + 1) {
      Logger::err(kLogLabel)
          << "Packet too small to contain packet header (2) ";
      return -1;
    }
    uint8_t sequence_difference = read_uint8(&p);
    *ack = *sequence - sequence_difference;
  } else {
    if (packet_bytes < 3 + 2) {
      Logger::err(kLogLabel)
          << "Packet too small to contain packet header (3) ";
      return -1;
    }
    *ack = read_uint16(&p);
  }

  int expected_bytes = 0;
  for (int i = 0; i <= 4; i++) {
    if (prefix_byte & (1 << i)) {
      expected_bytes++;
    }
  }

  if (packet_bytes < (p - packet_data) + expected_bytes) {
    Logger::err(kLogLabel) << "Packet too small to contain packet header (4) ";
    return -1;
  }

  *ack_bits = 0xFFFFFFFF;

  if (prefix_byte & (1 << 1)) {
    *ack_bits &= 0xFFFFFF00;
    *ack_bits |= (uint32_t)(read_uint8(&p));
  }

  if (prefix_byte & (1 << 2)) {
    *ack_bits &= 0xFFFF00FF;
    *ack_bits |= (uint32_t)(read_uint8(&p)) << 8;
  }

  if (prefix_byte & (1 << 3)) {
    *ack_bits &= 0xFF00FFFF;
    *ack_bits |= (uint32_t)(read_uint8(&p)) << 16;
  }

  if (prefix_byte & (1 << 4)) {
    *ack_bits &= 0x00FFFFFF;
    *ack_bits |= (uint32_t)(read_uint8(&p)) << 24;
  }

  return (int)(p - packet_data);
}

int reliable::write_packet_header(uint8_t* packet_data, uint16_t sequence,
                                  uint16_t ack, uint16_t ack_bits) {
  uint8_t* p = packet_data;

  uint8_t prefix_byte = 0;

  if ((ack_bits & 0x000000FF) != 0x000000FF) {
    prefix_byte |= (1 << 1);
  }
  if ((ack_bits & 0x0000FF00) != 0x0000FF00) {
    prefix_byte |= (1 << 2);
  }

  if ((ack_bits & 0x00FF0000) != 0x00FF0000) {
    prefix_byte |= (1 << 3);
  }

  if ((ack_bits & 0xFF000000) != 0xFF000000) {
    prefix_byte |= (1 << 4);
  }

  int sequence_difference = sequence - ack;
  if (sequence_difference < 0) {
    sequence_difference += 65536;
  }
  if (sequence_difference <= 255) {
    prefix_byte |= (1 << 5);
  }

  write_uint8(&p, prefix_byte);
  write_uint16(&p, sequence);

  if (sequence_difference <= 255) {
    write_uint8(&p, (uint8_t)sequence_difference);
  } else {
    write_uint16(&p, ack);
  }

  if ((ack_bits & 0x000000FF) != 0x000000FF) {
    write_uint8(&p, (uint8_t)(ack_bits & 0x000000FF));
  }

  if ((ack_bits & 0x0000FF00) != 0x0000FF00) {
    write_uint8(&p, (uint8_t)((ack_bits & 0x0000FF00) >> 8));
  }

  if ((ack_bits & 0x00FF0000) != 0x00FF0000) {
    write_uint8(&p, (uint8_t)((ack_bits & 0x00FF0000) >> 16));
  }

  if ((ack_bits & 0xFF000000) != 0xFF000000) {
    write_uint8(&p, (uint8_t)((ack_bits & 0xFF000000) >> 24));
  }

  if (p - packet_data > kMaxPacketHeaderBytes) {
    Logger::err(kLogLabel) << "WritePacketHeader wrote too big of a header at "
                           << p - packet_data
                           << " bytes! Expected no more than "
                           << kMaxPacketHeaderBytes;
  }

  return (int)(p - packet_data);
}

int reliable::read_fragment_header(uint8_t* packet_data, int packet_bytes,
                                   int max_fragments, int fragment_size,
                                   int* fragment_id, int* num_fragments,
                                   int* fragment_bytes, uint16_t* sequence,
                                   uint16_t* ack, uint32_t* ack_bits) {
  if (packet_bytes < kFragmentHeaderBytes) {
    Logger::err(kLogLabel) << "Packet is too small to read fragment header";
    return -1;
  }

  uint8_t* p = packet_data;

  uint8_t prefix_byte = read_uint8(&p);
  if (prefix_byte != 1) {
    Logger::err(kLogLabel) << "Prefix byte is not a fragment";
    return -1;
  }

  *sequence = read_uint16(&p);
  *fragment_id = (int)read_uint8(&p);
  *num_fragments = (int)read_uint8(&p) + 1;

  if (*num_fragments > max_fragments) {
    Logger::err(kLogLabel) << "Num fragments " << *num_fragments
                           << " outside range " << max_fragments;
    return -1;
  }

  if (*fragment_id > *num_fragments) {
    Logger::err(kLogLabel) << "Fragment id " << *fragment_id
                           << " outside range of num fragments "
                           << *num_fragments;
    return -1;
  }

  *fragment_bytes = packet_bytes - kFragmentHeaderBytes;

  uint16_t packet_sequence = 0;
  uint16_t packet_ack = 0;
  uint32_t packet_ack_bits = 0;

  if (*fragment_id == 0) {
    int packet_header_bytes =
        read_packet_header(packet_data + kFragmentHeaderBytes, packet_bytes,
                           &packet_sequence, &packet_ack, &packet_ack_bits);
    if (packet_header_bytes < 0) {
      Logger::err(kLogLabel) << "Bad packet header in fragment";
      return -1;
    }

    if (packet_sequence != *sequence) {
      Logger::err(kLogLabel) << "Bad packet sequence in fragment. Expected "
                             << *sequence << ", got " << packet_sequence;
      return -1;
    }

    *fragment_bytes = packet_bytes - packet_header_bytes - kFragmentHeaderBytes;
  }

  *ack = packet_ack;
  *ack_bits = packet_ack_bits;

  if (*fragment_bytes > fragment_size) {
    Logger::err(kLogLabel) << "Fragment bytes " << *fragment_bytes
                           << " > fragment size " << fragment_size;
    return -1;
  }

  if (*fragment_id != *num_fragments - 1 && *fragment_bytes != fragment_size) {
    Logger::err(kLogLabel) << "Fragment " << *fragment_id << " is "
                           << *fragment_bytes
                           << ", which is not the expected fragment size";
    return -1;
  }

  return (int)(p - packet_data);
}

void reliable::write_uint8(uint8_t** p, uint8_t value) {
  **p = value;
  ++(*p);
}

void reliable::write_uint16(uint8_t** p, uint16_t value) {
  (*p)[0] = value & 0xFF;
  (*p)[1] = value >> 8;
  *p += 2;
}

void reliable::write_uint32(uint8_t** p, uint32_t value) {
  (*p)[0] = value & 0xFF;
  (*p)[1] = (value >> 8) & 0xFF;
  (*p)[2] = (value >> 16) & 0xFF;
  (*p)[3] = value >> 24;
  *p += 4;
}

uint8_t reliable::read_uint8(uint8_t** p) {
  uint8_t value = **p;
  ++(*p);
  return value;
}

uint16_t reliable::read_uint16(uint8_t** p) {
  uint16_t value;
  value = (*p)[0];
  value |= (((uint16_t)((*p)[1])) << 8);
  *p += 2;
  return value;
}

uint32_t reliable::read_uint32(uint8_t** p) {
  uint32_t value;
  value = (*p)[0];
  value |= (((uint32_t)((*p)[1])) << 8);
  value |= (((uint32_t)((*p)[2])) << 16);
  value |= (((uint32_t)((*p)[3])) << 24);
  *p += 4;
  return value;
}
