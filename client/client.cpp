#include <cstdio>

#include <vsomeip/vsomeip.hpp>


static vsomeip::service_t service_id = 0x1111;
static vsomeip::instance_t service_instance_id = 0x2222;
static vsomeip::method_t service_method_id = 0x3333;

class ClientSender
{
public:
    ClientSender() : rtm_(vsomeip::runtime::get()),
                     app_(rtm_->create_application("client")){}

    bool init()
    {
        if (!app_->init())
        {
            LOG_ERR("Couldn't initialize application");
            return false;
        }

        // register a state handler to get called back after registration at the
        // runtime was successful
        app_->register_state_handler(
            std::bind(&ClientSender::on_state_cbk, this,
                      std::placeholders::_1));

        // register a callback for responses from the service
        app_->register_message_handler(vsomeip::ANY_SERVICE,
                                       service_instance_id, vsomeip::ANY_METHOD,
                                       std::bind(&ClientSender::on_message_cbk, this,
                                                 std::placeholders::_1));

        // register a callback which is called as soon as the service is available
        app_->register_availability_handler(service_id, service_instance_id,
                                            std::bind(&ClientSender::on_availability_cbk, this,
                                                      std::placeholders::_1, std::placeholders::_2,
                                                      std::placeholders::_3));
        return true;
    }

    void start()
    {
        // start the application and wait for the on_event callback to be called
        // this method only returns when app_->stop() is called
        app_->start();
    }

    void on_state_cbk(vsomeip::state_type_e _state)
    {
        if (_state == vsomeip::state_type_e::ST_REGISTERED)
        {
            // we are registered at the runtime now we can request the service
            // and wait for the on_availability callback to be called
            app_->request_service(service_id, service_instance_id);
        }
    }

    void on_availability_cbk(vsomeip::service_t _service,
                             vsomeip::instance_t _instance, bool _is_available)
    {
        // Check if the available service is the the hello world service
        if (service_id == _service && service_instance_id == _instance && _is_available)
        {
            // The service is available then we send the request
            // Create a new request
            std::shared_ptr<vsomeip::message> rq = rtm_->create_request();
            // Set the hello world service as target of the request
            rq->set_service(service_id);
            rq->set_instance(service_instance_id);
            rq->set_method(service_method_id);

            // Create a payload which will be sent to the service
            std::shared_ptr<vsomeip::payload> pl = rtm_->create_payload();
            std::string str("World");
            std::vector<vsomeip::byte_t> pl_data(std::begin(str), std::end(str));

            pl->set_data(pl_data);
            rq->set_payload(pl);
            // Send the request to the service. Response will be delivered to the
            // registered message handler
            LOG_INF("Sending: %s", str.c_str());
            app_->send(rq);
        }
    }

    void on_message_cbk(const std::shared_ptr<vsomeip::message> &_response)
    {
        if (service_id == _response->get_service() && service_instance_id == _response->get_instance() && vsomeip::message_type_e::MT_RESPONSE == _response->get_message_type() && vsomeip::return_code_e::E_OK == _response->get_return_code())
        {
            // Get the payload and print it
            std::shared_ptr<vsomeip::payload> pl = _response->get_payload();
            std::string resp = std::string(
                reinterpret_cast<const char *>(pl->get_data()), 0,
                pl->get_length());
            LOG_INF("Received: %s", resp.c_str());
            stop();
        }
    }

    void stop()
    {
        // unregister the state handler
        app_->unregister_state_handler();
        // unregister the message handler
        app_->unregister_message_handler(vsomeip::ANY_SERVICE,
                                         service_instance_id, vsomeip::ANY_METHOD);
        // alternatively unregister all registered handlers at once
        app_->clear_all_handler();
        // release the service
        app_->release_service(service_id, service_instance_id);
        // shutdown the application
        app_->stop();
    }

private:
    std::shared_ptr<vsomeip::runtime> rtm_;
    std::shared_ptr<vsomeip::application> app_;
};

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
#include <csignal>
#endif

#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
ClientSender *hw_cl_ptr(nullptr);
void handle_signal(int _signal)
{
    if (hw_cl_ptr != nullptr &&
        (_signal == SIGINT || _signal == SIGTERM))
        hw_cl_ptr->stop();
}
#endif
#include <iostream>

int main(void)
{
    std::cout << "Clinet Started ---------";

    ClientSender hw_cl;
#ifndef VSOMEIP_ENABLE_SIGNAL_HANDLING
    hw_cl_ptr = &hw_cl;
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
#endif
    if (hw_cl.init())
    {
        hw_cl.start();
        return 0;
    }
    else
    {
        return 1;
    }
}
