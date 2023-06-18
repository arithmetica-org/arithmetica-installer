#define CURL_STATICLIB

#include <iostream>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <curl/curl.h>
#include <windows.h>
#include <objbase.h>
#include <shobjidl.h>
#include <shlobj.h>

#include "tabulate.hpp"

// Callback function to receive the response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

size_t WriteFileCallback(void* contents, size_t size, size_t nmemb, FILE* file) {
    return fwrite(contents, size, nmemb, file);
}

std::string GetGlobalStartMenuDirectory() {
    WCHAR path[MAX_PATH];

    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, path);
    if (SUCCEEDED(hr)) {
        // use WideCharToMultiByte() to convert 'path' to a narrow string
        char pathA[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, path, -1, pathA, MAX_PATH, NULL, NULL);
        std::string answer = std::string(pathA);
        std::replace(answer.begin(), answer.end(), '\\', '/');
        return answer;
    }

    return "NULL";
}

std::string GetLocalStartMenuDirectory() {
    WCHAR path[MAX_PATH];

    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, path);
    if (SUCCEEDED(hr)) {
        // Convert wide character string to narrow string
        char pathA[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, path, -1, pathA, MAX_PATH, NULL, NULL);
        std::string answer = std::string(pathA);
        std::replace(answer.begin(), answer.end(), '\\', '/');
        return answer;
    }

    return "NULL";
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
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:62.0) Gecko/20100101 Firefox/62.0"); // Set the User-Agent header

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

    // c = 0;
    // while ((c = tags_info.find("\"prerelease\":", c)) != std::string::npos) {
    //     size_t start = c;
    //     size_t end = tags_info.find(",", start);
    //     prerelease.push_back(tags_info.substr(start, end - start));
    // }
}

LPCOLESTR CharToLPCOLESTR(const char* str) {
    int length = strlen(str);
    int wlength = MultiByteToWideChar(CP_ACP, 0, str, length, NULL, 0);

    LPWSTR wideStr = new WCHAR[wlength + 1];
    MultiByteToWideChar(CP_ACP, 0, str, length, wideStr, wlength);
    wideStr[wlength] = L'\0';

    return wideStr;
}

BOOL IsElevated() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if( OpenProcessToken( GetCurrentProcess( ),TOKEN_QUERY,&hToken ) ) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof( TOKEN_ELEVATION );
        if( GetTokenInformation( hToken, TokenElevation, &Elevation, sizeof( Elevation ), &cbSize ) ) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if( hToken ) {
        CloseHandle( hToken );
    }
    return fRet;
}

int main() {
    std::function GetStartMenuDirectory = GetGlobalStartMenuDirectory;

    if (!IsElevated()) {
        GetStartMenuDirectory = GetLocalStartMenuDirectory;
    }

    std::vector<std::string> tags, release_dates, prerelease;

    std::cout << "Retrieving versions ...\n";

    size_t n_tags = -1;
    for (int i = 1; tags.size() != n_tags; ++i) {
        n_tags = tags.size();
        add_tags(i, tags, release_dates, prerelease);
    }

    std::cout << "List prereleases? [y/n]: ";
    std::string ans;
    std::getline(std::cin, ans);

    std::vector<std::vector<std::string>> table;
    table.push_back({"Version", "Release Date", "Prerelease"});
    for (size_t i = 0; i < tags.size(); ++i) {
        std::string pre = "false";
        if (tags[i].find("alpha") != std::string::npos) {
            pre = "true";
        }
        if (pre == "true" && ans == "n")  {
            continue;
        }
        table.push_back({tags[i], release_dates[i], pre});
    }

    std::cout << table::tabulate(table) << std::endl;

    std::string version;
    std::cout << "Enter the version you want to download: ";
    std::getline(std::cin, version);

    std::cout << "Downloading version " << version << " ...\n";

    // Get the appdata directory
    char* appdata = nullptr;
    size_t size = 0;
    _dupenv_s(&appdata, &size, "APPDATA");

    std::string path = std::string(appdata) + "\\arithmetica-tui\\";

    // Replace all the backslashes with forward slashes
    std::replace(path.begin(), path.end(), '\\', '/');

    std::cout << "Download path: " << path << std::endl;

    std::filesystem::create_directories(path);

    // Create a file to save the downloaded data
    FILE* file = fopen((path + "arithmetica.exe").c_str(), "wb");
    if (!file) {
        std::cerr << "Failed to create the file." << std::endl;
        return 1;
    }

    // Initialize libcurl
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL." << std::endl;
        fclose(file);
        return 1;
    }

    // Set the download URL
    std::string download_url = "https://github.com/avighnac/arithmetica-tui/releases/download/" + version + "/arithmetica.exe";
    curl_easy_setopt(curl, CURLOPT_URL, download_url.c_str());

    // Set the callback function to write the downloaded data to a file
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

    // Follow redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Disable peer verification for HTTPS
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // Perform the download
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Failed to download the file: " << curl_easy_strerror(res) << std::endl;
        fclose(file);
        curl_easy_cleanup(curl);
        return 1;
    }

    // Get the response code
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    // Cleanup
    fclose(file);
    curl_easy_cleanup(curl);

    if (response_code != 200) {
        std::cout << "Download failed. Response code: " << response_code << std::endl;
        return 1;
    }

    std::cout << "Successfully downloaded arithmetica to " << path + "arithmetica.exe" << std::endl;

    std::string shortcut_path = GetStartMenuDirectory() + "/arithmetica.lnk";

    std::cout << "Shortcut path: " << shortcut_path << "\n";

    std::cout << "Creating shortcut ...\n";
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr)) {
        IShellLinkW* pShellLink;
        hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&pShellLink);
        if (SUCCEEDED(hr)) {
            pShellLink->SetPath(CharToLPCOLESTR((path + "arithmetica.exe").c_str()));

            IPersistFile* pPersistFile;
            hr = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
            if (SUCCEEDED(hr)) {
                std::cout << "Saving shortcut to " << shortcut_path << std::endl;
                HRESULT res = pPersistFile->Save(CharToLPCOLESTR(shortcut_path.c_str()), TRUE);
                pPersistFile->Release();

                if (FAILED(res)) {
                    std::cerr << "Failed to save shortcut. Did you run as administrator?" << std::endl;
                    return 1;
                }
            }

            pShellLink->Release();
        }
        CoUninitialize();
    }

    std::cout << "arithmetica has been successfully installed! To run it, search for \"arithmetica\" by pressing the Windows key." << std::endl;
    std::cout << "Press enter to exit\n";
    std::cin.get();

    return 0;
}