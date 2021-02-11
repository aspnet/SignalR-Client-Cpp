// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <iostream>
#include <sstream>
#include "hub_connection_builder.h"
#include "log_writer.h"
#include <future>
#include "signalr_value.h"

class logger : public signalr::log_writer
{
    // Inherited via log_writer
    virtual void __cdecl write(const std::string & entry) override
    {
        std::cout << entry;
    }
};

void send_message(std::shared_ptr<signalr::hub_connection> connection, const std::string& message)
{
    std::vector<signalr::value> arr { std::string("c++"), message };
    signalr::value args(arr);

    // if you get an internal compiler error uncomment the lambda below or install VS Update 4
    connection->invoke("Send", args, [](const signalr::value& value, std::exception_ptr exception)
    {
        try
        {
            if (exception)
            {
                std::rethrow_exception(exception);
            }

            if (value.is_string())
            {
                std::cout << "Received: " << value.as_string() << std::endl;
            }
            else
            {
                std::cout << "hub method invocation has completed" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "Error while sending data: " << e.what() << std::endl;
        }
    });
}

void chat()
{
    std::shared_ptr<signalr::hub_connection> connection = signalr::hub_connection_builder::create("http://localhost:5000/default")
        .with_logging(std::make_shared<logger>(), signalr::trace_level::all)
        .build();

    connection->on("Send", [](const signalr::value & m)
    {
        std::cout << std::endl << m.as_array()[0].as_string() << std::endl << "Enter your message: ";
    });

    std::promise<void> task;
    connection->start([&connection, &task](std::exception_ptr exception)
    {
        if (exception)
        {
            try
            {
                std::rethrow_exception(exception);
            }
            catch (const std::exception & ex)
            {
                std::cout << "exception when starting connection: " << ex.what() << std::endl;
            }
            task.set_value();
            return;
        }

        std::cout << "Enter your message:";
        while (connection->get_connection_state() == signalr::connection_state::connected)
        {
            std::string message;
            std::getline(std::cin, message);

            if (message == ":q" || connection->get_connection_state() != signalr::connection_state::connected)
            {
                break;
            }

            send_message(connection, message);
        }

        connection->stop([&task](std::exception_ptr exception)
        {
            try
            {
                if (exception)
                {
                    std::rethrow_exception(exception);
                }

                std::cout << "connection stopped successfully" << std::endl;
            }
            catch (const std::exception & e)
            {
                std::cout << "exception when stopping connection: " << e.what() << std::endl;
            }

            task.set_value();
        });
    });

    task.get_future().get();
}

int main()
{
    chat();

    return 0;
}
