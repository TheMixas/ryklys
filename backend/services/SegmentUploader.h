#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <curl/curl.h>

class SegmentUploader {
public:
    SegmentUploader(std::string storageUrl, std::string secret)
        : storageUrl_(std::move(storageUrl)), secret_(std::move(secret))
    {}

    bool upload(const std::string& localPath, const std::string& streamId, const std::string& filename)
    {
        FILE* f = fopen(localPath.c_str(), "rb");
        if (!f) {
            std::cerr << "[Uploader] Failed to open: " << localPath << std::endl;
            return false;
        }

        fseek(f, 0, SEEK_END);
        long fileSize = ftell(f);
        fseek(f, 0, SEEK_SET);

        std::string url = storageUrl_ + "/api/segments/" + streamId + "/" + filename;

        CURL* curl = curl_easy_init();
        if (!curl) {
            fclose(f);
            return false;
        }

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-Storage-Key: " + secret_).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READDATA, f);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fileSize);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        fclose(f);

        if (res != CURLE_OK) {
            std::cerr << "[Uploader] curl error: " << curl_easy_strerror(res) << std::endl;
            return false;
        }

        if (httpCode != 200) {
            std::cerr << "[Uploader] HTTP " << httpCode << " for " << url << std::endl;
            return false;
        }

        std::cout << "[Uploader] Uploaded " << streamId << "/" << filename << std::endl;
        return true;
    }

    bool remove(const std::string& streamId, const std::string& filename)
    {
        std::string url = storageUrl_ + "/api/segments/" + streamId + "/" + filename;

        CURL* curl = curl_easy_init();
        if (!curl) return false;

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-Storage-Key: " + secret_).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return (res == CURLE_OK && httpCode == 200);
    }

private:
    std::string storageUrl_;
    std::string secret_;
};