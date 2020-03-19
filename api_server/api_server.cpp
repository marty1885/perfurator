#include <drogon/drogon.h>

#include <fstream>
#include <unordered_map>
#include <string>

#include <crypto.hpp>

#include <config.h>

inline std::unordered_map<std::string, std::string> load_all_passwords()
{
        std::ifstream in("users.csv");
        if(in.good() == false)
                throw std::runtime_error("Cannot open CSV.");

        std::unordered_map<std::string, std::string> passwords;
        passwords.reserve(23000000);

        std::string line;
        int i=0;
        while(std::getline(in, line)) {
                i++;
                if(i == 1)
                        continue;
                std::string user = line.substr(0, line.find(","));
                std::string password = line.substr(line.find(",")+1, std::string::npos);
                if(user.empty()) // In case bad data
                        continue;
                passwords[user] = password;
        }

        return passwords;
}

int main()
{
        using namespace drogon;

        const auto passwords = load_all_passwords();
        std::cout << "Done loading user passwords" << std::endl;
        auto make_fail_responce = [](const std::string& message) {
                Json::Value j;
                j["result"] = 0;
                j["reason"] = message;
                auto resp = HttpResponse::newHttpJsonResponse(j);
                resp->setStatusCode(HttpStatusCode::k406NotAcceptable);
                return resp;
        };

        app().registerHandler("/buy_mask", [&](const HttpRequestPtr& req,
                std::function<void (const HttpResponsePtr &)> &&callback)
        {
                // Ignore non POST requests
                if(req->method() != HttpMethod::Post) {
                        callback(HttpResponse::newHttpJsonResponse(""));
                        return;
                }

                const std::string& id = req->getParameter("id");
                const std::string& passwd_plain_text = req->getParameter("password");

                if(id.empty() || passwd_plain_text.empty()) {
                        callback(make_fail_responce("ID or password missing"));
                        return;
                }

                // Make sure ID is valid
                auto password_it = passwords.find(id);
                if(password_it == passwords.end()) {
                        callback(make_fail_responce("Invalid ID"));
                        return;
                }

                // Check the password
                if(verify_password(password_it->second, passwd_plain_text) == false) {
                        callback(make_fail_responce("Invalid password"));
                        return;
                }

                // All good. Ask the validation server if the user
                // Compute which server we should ask
                size_t id_hash = std::hash<std::string>()(id); // std::hash should be good enough
                int server_ip = id_hash%NUM_VALIDATION_SERVER + 150; // 150 is base offset
                auto client = HttpClient::newHttpClient("http://127.0.0." + std::to_string(server_ip) + ":8080");
                auto validation_req = HttpRequest::newHttpRequest();
                std::promise<bool> valid;
                // validation_req->setMethod(drogon::Post); //FIXME: Post not working for some reason
                validation_req->setPath("/buy_mask");
                validation_req->setParameter("id", id);
                // TODO: make this function retry
                std::string error_what;
                client->sendRequest(validation_req, [&](ReqResult result, const HttpResponsePtr &response) {
                        if(response == nullptr) { // If no server responce
                                valid.set_value(false);
                                return;
                        }
                        bool ok = (response->getJsonObject()->operator[]("result") == 1);
                        error_what = response->getJsonObject()->operator[]("reason").asString();
                        valid.set_value(ok);
                });

                // Wait for resonce
                bool ok = valid.get_future().get();
                if(ok == false) {
                        callback(make_fail_responce(error_what));
                        return;
                }

                Json::Value json;
                json["result"]=1;
                json["message"]=std::string("Done");
                auto resp=HttpResponse::newHttpJsonResponse(json);
                callback(resp);
        })
                .setLogPath("./")
                .setLogLevel(trantor::Logger::kWarn)
                .addListener("127.0.0.100", 8080)
                .setThreadNum(16)
                .run();
}