#include <cstdio>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <vsomeip/vsomeip.hpp>

static vsomeip::service_t service_id = 0x1111;
static vsomeip::instance_t service_instance_id = 0x2222;
static vsomeip::method_t service_method_id = 0x3333;

class ServiceProvider
{
public:
    ServiceProvider() : rtm_(vsomeip::runtime::get()),
                        app_(rtm_->create_application()),
                        stop_(false),
                        stop_thread_(std::bind(&ServiceProvider::stop, this))
    {
    }

    ~ServiceProvider()
    {
        stop_thread_.join();
    }

    bool init()
    {
        // init the application
        if (!app_->init())
        {
            LOG_ERR("Couldn't initialize application");
            return false;
        }

        // register a message handler callback for messages sent to our service
        app_->register_message_handler(service_id, service_instance_id,
                                       service_method_id,
                                       std::bind(&ServiceProvider::on_message_cbk, this,
                                                 std::placeholders::_1));

        // register a state handler to get called back after registration at the
        // runtime was successful
        app_->register_state_handler(
            std::bind(&ServiceProvider::on_state_cbk, this,
                      std::placeholders::_1));
        return true;
    }

    void start()
    {
        // start the application and wait for the on_event callback to be called
        // this method only returns when app_->stop() is called
        app_->start();
    }

    void stop()
    {
        std::unique_lock<std::mutex> its_lock(mutex_);
        while (!stop_)
        {
            condition_.wait(its_lock);
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
        // Stop offering the service
        app_->stop_offer_service(service_id, service_instance_id);
        // unregister the state handler
        app_->unregister_state_handler();
        // unregister the message handler
        app_->unregister_message_handler(service_id, service_instance_id,
                                         service_method_id);
        // shutdown the application
        app_->stop();
    }

    void terminate()
    {
        std::lock_guard<std::mutex> its_lock(mutex_);
        stop_ = true;
        condition_.notify_one();
    }

    void on_state_cbk(vsomeip::state_type_e _state)
    {
        if (_state == vsomeip::state_type_e::ST_REGISTERED)
        {
            // we are registered at the runtime and can offer our service
            app_->offer_service(service_id, service_instance_id);
        }
    }

    void on_message_cbk(const std::shared_ptr<vsomeip::message> &_request)
    {

        std::shared_ptr<vsomeip::message> resp = rtm_->create_response(_request);
        std::string str("Server");
        str.append(
            reinterpret_cast<const char *>(_request->get_payload()->get_data()),
            0, _request->get_payload()->get_length());

        // Create a payload which will be sent back to the client
        std::shared_ptr<vsomeip::payload> resp_pl = rtm_->create_payload();
        std::vector<vsomeip::byte_t> pl_data(str.begin(), str.end());
        resp_pl->set_data(pl_data);
        resp->set_payload(resp_pl);

        // Send the response back
        app_->send(resp);
        // we have finished
        terminate();
    }

private:
    std::shared_ptr<vsomeip::runtime> rtm_;
    std::shared_ptr<vsomeip::application> app_;
    bool stop_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread stop_thread_;
};

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
#include <csignal>
#endif

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
ServiceProvider *hw_srv_ptr(nullptr);
void handle_signal(int _signal)
{
    if (hw_srv_ptr != nullptr &&
        (_signal == SIGINT || _signal == SIGTERM))
        hw_srv_ptr->terminate();
}
#endif
#include <iostream>

int main(void)
{
    std::cout << "Server started\n";
    ServiceProvider hw_srv;
#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
    hw_srv_ptr = &hw_srv;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
#endif
    if (hw_srv.init())
    {
        hw_srv.start();
        return 0;
    }
    else
    {
        return 1;
    }
}
