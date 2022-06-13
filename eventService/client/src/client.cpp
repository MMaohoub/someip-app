// Copyright (C) 2014-2017 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
#include <csignal>
#endif
#include <chrono>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vsomeip/vsomeip.hpp>

#define SAMPLE_SERVICE_ID 0x1234
#define SAMPLE_INSTANCE_ID 0x5678
#define SAMPLE_METHOD_ID 0x0421

#define SAMPLE_EVENT_ID 0x8778
#define SAMPLE_GET_METHOD_ID 0x0001
#define SAMPLE_SET_METHOD_ID 0x0002

#define SAMPLE_EVENTGROUP_ID 0x4465

#define OTHER_SAMPLE_SERVICE_ID 0x0248
#define OTHER_SAMPLE_INSTANCE_ID 0x5422
#define OTHER_SAMPLE_METHOD_ID 0x1421

class Sender {
 public:
  Sender(bool _use_tcp)
      : app_(vsomeip::runtime::get()->create_application("tx-signal-adapter")),
        use_tcp_(_use_tcp) {}

  bool init() {
    if (!app_->init()) {
      std::cerr << "Couldn't initialize application" << std::endl;
      return false;
    }
    std::cout << "Client settings [protocol=" << (use_tcp_ ? "TCP" : "UDP")
              << "]" << std::endl;

    app_->register_state_handler(
        std::bind(&Sender::on_state, this, std::placeholders::_1));

    app_->register_message_handler( vsomeip::ANY_SERVICE, SAMPLE_INSTANCE_ID, vsomeip::ANY_METHOD,
        std::bind(&Sender::on_message, this, std::placeholders::_1));

    app_->register_availability_handler(
        SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID,
        std::bind(&Sender::on_availability, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3));

    std::set<vsomeip::eventgroup_t> its_groups;
    its_groups.insert(SAMPLE_EVENTGROUP_ID);
    app_->request_event(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_EVENT_ID,
                        its_groups, vsomeip::event_type_e::ET_FIELD);
    app_->subscribe(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID,
                    SAMPLE_EVENTGROUP_ID);

    return true;
  }

  void start() { app_->start(); }

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
  /*
   * Handle signal to shutdown
   */
  void stop() {
    app_->clear_all_handler();
    app_->unsubscribe(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID,
                      SAMPLE_EVENTGROUP_ID);
    app_->release_event(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID, SAMPLE_EVENT_ID);
    app_->release_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    app_->stop();
  }
#endif

  void on_state(vsomeip::state_type_e _state) {
    if (_state == vsomeip::state_type_e::ST_REGISTERED) {
      app_->request_service(SAMPLE_SERVICE_ID, SAMPLE_INSTANCE_ID);
    }
  }

  void on_availability(vsomeip::service_t _service,
                       vsomeip::instance_t _instance, bool _is_available) {
    std::cout << "Service [" << std::setw(4) << std::setfill('0') << std::hex
              << _service << "." << _instance << "] is "
              << (_is_available ? "available." : "NOT available.") << std::endl;
  }

  void on_message(const std::shared_ptr<vsomeip::message> &_response) {
    std::stringstream its_message;
    its_message << "**********Received a notification for Event [" << std::setw(4)
                << std::setfill('0') << std::hex << _response->get_service()
                << "." << std::setw(4) << std::setfill('0') << std::hex
                << _response->get_instance() << "." << std::setw(4)
                << std::setfill('0') << std::hex << _response->get_method()
                << "] to Client/Session [" << std::setw(4) << std::setfill('0')
                << std::hex << _response->get_client() << "/" << std::setw(4)
                << std::setfill('0') << std::hex << _response->get_session()
                << "] = ";
    std::shared_ptr<vsomeip::payload> its_payload = _response->get_payload();
    its_message << "(" << std::dec << its_payload->get_length() << ") ";
    for (uint32_t i = 0; i < its_payload->get_length(); ++i)
      its_message << std::hex << std::setw(2) << std::setfill('0')
                  << (std::int32_t)its_payload->get_data()[i] << " ";
    std::cout << its_message.str() << std::endl;

    if (_response->get_client() == 0) {
      if ((its_payload->get_length() % 5) == 0) {
        std::shared_ptr<vsomeip::message> its_get =
            vsomeip::runtime::get()->create_request();
        its_get->set_service(SAMPLE_SERVICE_ID);
        its_get->set_instance(SAMPLE_INSTANCE_ID);
        its_get->set_method(SAMPLE_GET_METHOD_ID);
        its_get->set_reliable(use_tcp_);
        app_->send(its_get);
      }

      if ((its_payload->get_length() % 8) == 0) {
        std::shared_ptr<vsomeip::message> its_set =
            vsomeip::runtime::get()->create_request();
        its_set->set_service(SAMPLE_SERVICE_ID);
        its_set->set_instance(SAMPLE_INSTANCE_ID);
        its_set->set_method(SAMPLE_SET_METHOD_ID);
        its_set->set_reliable(use_tcp_);

        const vsomeip::byte_t its_data[] = {0x92, 0x99, 0x44, 0x45, 0x46, 0x47,
                                            0x48, 0x49, 0x50, 0x58, 0x52};
        std::shared_ptr<vsomeip::payload> its_set_payload =
            vsomeip::runtime::get()->create_payload();
        its_set_payload->set_data(its_data, sizeof(its_data));
        its_set->set_payload(its_set_payload);
        app_->send(its_set);
      }
    }
  }

 private:
  std::shared_ptr<vsomeip::application> app_;
  bool use_tcp_;
};

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
Sender *its_sample_ptr(nullptr);
void handle_signal(std::int32_t _signal) {
  if (its_sample_ptr != nullptr && (_signal == SIGINT || _signal == SIGTERM))
    its_sample_ptr->stop();
}
#endif

std::int32_t main(void) {
  bool use_tcp = false;

  std::string tcp_enable("--tcp");

  std::int32_t i = 1;

  Sender tx_signal_adapter(use_tcp);

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
  its_sample_ptr = &tx_signal_adapter;
  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);
#endif

  if (tx_signal_adapter.init()) {
    tx_signal_adapter.start();
    return 0;
  } else {
    return 1;
  }
}
