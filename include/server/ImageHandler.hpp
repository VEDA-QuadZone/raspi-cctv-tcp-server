// include/handler/ImageHandler.hpp
#ifndef IMAGE_HANDLER_HPP
#define IMAGE_HANDLER_HPP

#include <string>
#include <sqlite3.h>

class ImageHandler {
public:
    explicit ImageHandler(sqlite3* db);
    std::string handleImageUpload(int client_fd, const std::string& filename, size_t filesize);
    void handleGetImage(int client_fd, const std::string& imagePath);
    
private:
    sqlite3* db;
};

#endif // IMAGE_HANDLER_HPP
