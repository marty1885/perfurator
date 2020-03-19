#include <drogon/drogon.h>

#include <unordered_map>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <tbb/concurrent_unordered_map.h>

#include <config.h>

tbb::concurrent_unordered_map<std::string, int64_t> load_last_buy_time()
{
        // stub
        return tbb::concurrent_unordered_map<std::string, int64_t>();
}

void save_data(tbb::concurrent_unordered_map<std::string, int64_t>& last_buy)
{
        std::cout << "STUB! Save the map to somewhere safe" << std::endl;
}

int main(int argc, char** argv)
{

        if(argc != 2) {
                std::cout << "usage: " << argv[0] << " server_id" << std::endl;
                return 0;
        }

        if(std::stoi(argv[1]) > NUM_VALIDATION_SERVER) {
                throw std::logic_error(std::string(argv[1]) + " is larger than NUM_VALIDATION_SERVER (" + std::to_string(NUM_VALIDATION_SERVER) + ")");
        }
        using namespace drogon;
        tbb::concurrent_unordered_map<std::string, int64_t> last_buy = load_last_buy_time();

        auto make_fail_responce = [](const std::string& message) {
                Json::Value j;
                j["result"] = 0;
                j["reason"] = message;
                auto resp = HttpResponse::newHttpJsonResponse(j);
                resp->setStatusCode(HttpStatusCode::k406NotAcceptable);
                return resp;
        };

        std::thread save_data_thread([&](){
                while(true) {
                        std::this_thread::sleep_for(std::chrono::seconds(300));
                        save_data(last_buy);
                }
        });
        save_data_thread.detach();

        std::atomic<bool> stop_accepting_requests = false;
        std::atomic<int> writing_thread_flying = 0;


        app().registerHandler("/buy_mask", [&](const HttpRequestPtr& req,
                std::function<void (const HttpResponsePtr &)> &&callback) {
                // Ignore non POST requests
                // if(req->method() != HttpMethod::Post) {
                //         callback(HttpResponse::newHttpJsonResponse(""));
                //         return;
                // }

                const std::string& id = req->getParameter("id");
                if(id.empty() == true) {
                        callback(make_fail_responce("ID is missing"));
                        return;
                }

                if(last_buy.find(id) != last_buy.end()) {
                        callback(make_fail_responce("You have bought mask in 7 days"));
                        return;
                }

                while(stop_accepting_requests == true) {}

                writing_thread_flying += 1;
                last_buy[std::move(id)] = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                writing_thread_flying -= 1;

                Json::Value json;
                json["result"]=1;
                json["message"]=std::string("Done");
                auto resp=HttpResponse::newHttpJsonResponse(json);
                callback(resp);
        })
        // Internal API. We trust the query
        .registerHandler("/remove_old", [&](const HttpRequestPtr& req,
                std::function<void (const HttpResponsePtr &)> &&callback) {

                int64_t before = std::stoll(req->getParameter("time"));

                // Wait for all threads to finish writing to the hash map
                stop_accepting_requests = true;
                while(writing_thread_flying != 0) {}

                for(auto it = last_buy.begin();it!=last_buy.end();it++)
                        last_buy.unsafe_erase(it->first);
                stop_accepting_requests = false;

                Json::Value json;
                json["result"]=1;
                json["message"]=std::string("Done");
                auto resp=HttpResponse::newHttpJsonResponse(json);
                callback(resp);
        })
                .setLogPath("./")
                .setLogLevel(trantor::Logger::kWarn)
                .addListener("127.0.0.150", 8080)
                .setThreadNum(16)
                .run();

}