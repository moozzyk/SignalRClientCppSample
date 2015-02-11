#include "stdafx.h"
#include "signalrclient\connection.h"
#include <iostream>

void send_message(signalr::connection &connection, std::wstring message)
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
            std::cout << "Error while sending data: " << e.what();
        }
    });
}

int _tmain(int argc, _TCHAR* argv[])
{
    signalr::connection connection{ L"http://localhost:34281/echo" };
    connection.set_message_received([](std::wstring m){ std::wcout << L"Message received:" << m << std::endl << L"Enter message: "; });

    pplx::task_completion_event<void> tce;

    connection.start()
        .then([&connection]() // fine to capture by reference - we are blocking so it is guaranteed to be valid
    {
        for (;;)
        {
            std::wstring message;
            std::getline(std::wcin, message);

            if (message == L":q")
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
            std::cout << "exception: " << e.what() << std::endl;
        }

        return connection.stop();
    })
    .then([tce](pplx::task<void> stop_task)
    {
        try
        {
            stop_task.get();
            std::cout << "connection stopped successfully" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cout << "exception when closing connection: " << e.what() << std::endl;
        }

        tce.set();
    });

    pplx::task<void>(tce).get();

    return 0;
}
