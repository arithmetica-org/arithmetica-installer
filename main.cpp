#define CURL_STATICLIB

#include <iostream>
#include <string>
#include <vector>
#include <curl/curl.h>

#include "tabulate.hpp"

// Callback function to receive the response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

void add_tags(int page, std::vector<std::string> &tags, std::vector<std::string> &release_dates, std::vector<std::string> &prerelease) {
    CURL* curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return;
    }

    std::string tags_info;
    curl_easy_setopt(curl, CURLOPT_URL, ("https://api.github.com/repos/arithmetica-org/arithmetica-tui/releases?page=" + std::to_string(page)).c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &tags_info);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "to-get-release-tags"); // Set the User-Agent header

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Failed to perform the request: " << curl_easy_strerror(res) << std::endl;
    }

    curl_easy_cleanup(curl);

    curl_global_cleanup();

    size_t c = 0;
    while ((c = tags_info.find("\"tag_name\":", c)) != std::string::npos) {
        size_t start = tags_info.find("\"", c + 10) + 1;
        size_t end = tags_info.find("\"", start);
        tags.push_back(tags_info.substr(start, end - start));
        c = end;
    }

    c = 0;
    while ((c = tags_info.find("\"created_at\":", c)) != std::string::npos) {
        size_t start = tags_info.find("\"", c + 13) + 1;
        size_t end = tags_info.find("\"", start);
        release_dates.push_back(tags_info.substr(start, end - start));
        c = end;
    }

    c = 0;
    while ((c = tags_info.find("\"prerelease\":", c)) != std::string::npos) {
        size_t start = c;
        size_t end = tags_info.find(",", start);
        prerelease.push_back(tags_info.substr(start, end - start));
    }
}

int main() {

    std::vector<std::string> tags, release_dates, prerelease;

    std::cout << "Retrieving versions ...\n";

    size_t n_tags = -1;
    for (int i = 1; tags.size() != n_tags; ++i) {
        n_tags = tags.size();
        add_tags(i, tags, release_dates, prerelease);
    }

    std::vector<std::vector<std::string>> table;
    table.push_back({"Version", "Release Date", "Prerelease"});
    for (size_t i = 0; i < tags.size(); ++i) {
        table.push_back({tags[i], release_dates[i], prerelease[i]});
    }

    std::cout << table::tabulate(table) << std::endl;

    return 0;
}