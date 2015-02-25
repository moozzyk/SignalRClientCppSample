#include "stdafx.h"
#include "signalrclient\connection.h"
#include <iostream>

void send_message(signalr::connection &connection, const utility::string_t& message)
{
    connection.send(message)
        .then([](pplx::task<void> send_task)  // fire and forget but we need to observe exceptions
    {
        try
        {
            send_task.get();
        }
        catch (const std::exception &e)
        {
            ucout << U("Error while sending data: ") << e.what();
        }
    });
}

int main()
{
    signalr::connection connection{ U("http://localhost:34281/echo") };
    connection.set_message_received([](const utility::string_t& m)
    {
        ucout << U("Message received:") << m << std::endl << U("Enter message: ");
    });

    pplx::task_completion_event<void> tce;

    connection.start()
        .then([&connection]() // fine to capture by reference - we are blocking so it is guaranteed to be valid
        {
            for (;;)
            {
                utility::string_t message;
                std::getline(ucin, message);

                if (message == U(":q"))
                {
                    break;
                }

                send_message(connection, message);
            }
        })
        .then([&connection](pplx::task<void> previous_task)
        {
            try
            {
                previous_task.get();
            }
            catch (const std::exception &e)
            {
                ucout << U("exception: ") << e.what() << std::endl;
            }

            return connection.stop();
        })
        .then([tce](pplx::task<void> stop_task)
        {
            try
            {
                stop_task.get();
                ucout << U("connection stopped successfully") << std::endl;
            }
            catch (const std::exception &e)
            {
                ucout << U("exception when closing connection: ") << e.what() << std::endl;
            }

            tce.set();
        });

    pplx::task<void>(tce).get();

    return 0;
}
