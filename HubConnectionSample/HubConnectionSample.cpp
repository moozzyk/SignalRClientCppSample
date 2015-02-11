#include "stdafx.h"

#include <iostream>
#include <sstream>
#include "signalrclient\hub_connection.h"

void send_message(signalr::hub_proxy proxy, const std::wstring& name, const std::wstring& message)
{
    web::json::value args{};
    args[0] = web::json::value::string(name);
    args[1] = web::json::value(message);

    proxy.invoke<void>(L"send", args, [](const web::json::value&){})
        .then([](pplx::task<void> invoke_task)  // fire and forget but we need to observe exceptions
    {
        try
        {
            invoke_task.get();
        }
        catch (const std::exception &e)
        {
            std::cout << "Error while sending data: " << e.what();
        }
    });
}

void chat(std::wstring name)
{
    signalr::hub_connection connection{L"http://localhost:34281/SignalR"};
    auto proxy = connection.create_hub_proxy(L"ChatHub");
    proxy.on(L"broadcastMessage", [](const web::json::value& m)
    {
        std::wcout << std::endl << m.at(0).as_string() << " wrote:" << m.at(1).as_string() << std::endl << L"Enter your message: ";
    });

    pplx::task_completion_event<void> tce;

    connection.start()
        .then([proxy, name]()
        mutable {
        std::wcout << L"Enter your message:";
        for (;;)
            {
                std::wstring message;
                std::getline(std::wcin, message);

                if (message == L":q")
                {
                    break;
                }

                send_message(proxy, name, message);
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
}

int _tmain(int argc, _TCHAR* argv[])
{
    std::wcout << L"Enter your name: ";
    std::wstring name;
    std::getline(std::wcin, name);

    chat(name);

    return 0;
}
