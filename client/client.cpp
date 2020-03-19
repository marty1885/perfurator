#include <curl/curl.h>

#include <string>
#include <fstream>
#include <iostream>
#include <chrono>

#include <fmt/format.h>

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
   return size * nmemb;
}

int main()
{
        std::ifstream in("users_plantext.csv");
        std::string line;

        curl_global_init(CURL_GLOBAL_ALL);

        // You can make this parallel if you have the resource
        int i=0;
        int success = 0;
        auto curl = curl_easy_init();
        if(curl == nullptr) 
                throw std::runtime_error("Error: CURL init failed");
        auto t1 = std::chrono::high_resolution_clock::now();
        while(std::getline(in, line)) {
                // Ignore CSV header
                i++;
                if(i == 1)
                        continue;

                std::string id = line.substr(0, line.find(","));
                std::string password = line.substr(line.find(",")+1, std::string::npos);
                // I'm not smart enought to get CURL working this way

                curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.100:8080/buy_mask");
                auto params = fmt::format("id={}&password={}", id, password);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // Disable printing

                auto res = curl_easy_perform(curl);
                if(res != CURLE_OK)
                        std::cerr << "Request failed" << std::endl;
                else
                        success += 1;
                if(i% 1000 == 0) {
                        auto t2 = std::chrono::high_resolution_clock::now();
                        float duration = std::chrono::duration_cast<std::chrono::duration<float>>(t2-t1).count();
                        std::cout << "\33[2K\r duration = " << duration << ", " << (i-1)/duration << " requests/s" << std::flush; //]
                }
        }
        std::cout << "\n";
        curl_easy_cleanup(curl);
        curl_global_cleanup();

        std::cout << "Success rate: " << (i-1/(double)success) << std::endl;
}