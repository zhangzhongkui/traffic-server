/** @file
 *
 *  A brief file description
 *
 *  @section license License
 *
 *  Licensed to the Apache Software Foundation (ASF) under one
 *  or more contributor license agreements.  See the NOTICE file
 *  distributed with this work for additional information
 *  regarding copyright ownership.  The ASF licenses this file
 *  to you under the Apache License, Version 2.0 (the
 *  "License"); you may not use this file except in compliance
 *  with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once

#include "ts/List.h"
#include "ts/PriorityQueue.h"

#include "I_VConnection.h"

#include "QUICFrame.h"
#include "QUICStreamState.h"

class QUICFrameTransmitter;
class QUICStreamState;
class QUICStreamManager;

/**
 * @brief QUIC Stream
 * TODO: This is similar to Http2Stream. Need to think some integration.
 */
class QUICStream : public VConnection
{
public:
  QUICStream() : VConnection(nullptr) {}
  ~QUICStream() {}
  void init(QUICStreamManager *manager, QUICFrameTransmitter *tx, uint32_t id, uint64_t recv_max_stream_data = 0,
            uint64_t send_max_stream_data = 0);
  void start();
  void init_flow_control_params(uint32_t recv_max_stream_data, uint32_t send_max_stream_data);
  int main_event_handler(int event, void *data);

  uint32_t id();

  // Implement VConnection interface.
  VIO *do_io_read(Continuation *c, int64_t nbytes = INT64_MAX, MIOBuffer *buf = nullptr) override;
  VIO *do_io_write(Continuation *c = nullptr, int64_t nbytes = INT64_MAX, IOBufferReader *buf = 0, bool owner = false) override;
  void do_io_close(int lerrno = -1) override;
  void do_io_shutdown(ShutdownHowTo_t howto) override;
  void reenable(VIO *vio) override;

  QUICError recv(std::shared_ptr<const QUICStreamFrame> frame);
  QUICError recv(const std::shared_ptr<const QUICMaxStreamDataFrame> &frame);
  QUICError recv(const std::shared_ptr<const QUICStreamBlockedFrame> &frame);

  void reset();

  bool is_read_ready();

  LINK(QUICStream, link);

private:
  QUICStreamState _state;

  void _send();

  void _write_to_read_vio(const std::shared_ptr<const QUICStreamFrame> &);
  void _reorder_data();
  // NOTE: Those are called update_read_request/update_write_request in Http2Stream
  // void _read_from_net(uint64_t read_len, bool direct);
  // void _write_to_net(IOBufferReader *buf_reader, int64_t write_len, bool direct);

  void _signal_read_event(bool call_update);
  void _signal_write_event(bool call_update);

  Event *_send_tracked_event(Event *event, int send_event, VIO *vio);

  void _slide_recv_max_stream_data();
  QUICError _recv_flow_control(uint64_t new_offset);
  bool _send_flow_control(uint64_t len);

  QUICStreamId _id                = 0;
  QUICOffset _recv_offset         = 0;
  QUICOffset _recv_largest_offset = 0;
  QUICOffset _send_offset         = 0;

  uint64_t _recv_max_stream_data        = 0;
  uint64_t _recv_max_stream_data_deleta = 0;
  uint64_t _send_max_stream_data        = 0;

  VIO _read_vio;
  VIO _write_vio;

  Event *_read_event;
  Event *_write_event;

  // Fragments of received STREAM frame (offset is unmatched)
  // TODO: Consider to replace with ts/RbTree.h or other data structure
  std::map<QUICOffset, std::shared_ptr<const QUICStreamFrame>> _request_stream_frame_buffer;

  QUICStreamManager *_streamManager = nullptr;
  QUICFrameTransmitter *_tx         = nullptr;
};