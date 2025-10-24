#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <set>

#ifdef _WIN32
    #include <windows.h>
    #include <wincrypt.h>
#endif

// stb_image for image loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//hallo
namespace fs = std::filesystem;

// ---------------------------------------------------------
// File information structures
// ---------------------------------------------------------
struct AudioMetadata {
    std::string title;
    std::string artist;
    std::string album;
    int length;
    int bitrate;
    
    AudioMetadata() : length(0), bitrate(0) {}
};

struct FileInfo {
    std::string path;
    uint64_t size_bytes;
    std::string type;
    std::string content_preview;
    std::string phash;
    AudioMetadata audio_meta;
};

// ---------------------------------------------------------
// SimilarityFinder class
// ---------------------------------------------------------
class SimilarityFinder {
public:
    uint64_t calculateImageHash(const std::string& imagePath) {
        int width, height, channels;
        unsigned char* img = stbi_load(imagePath.c_str(), &width, &height, &channels, 1);
        
        if (!img) return 0;
        
        // Resize to 9x8 for dHash
        const int hashWidth = 9;
        const int hashHeight = 8;
        unsigned char resized[hashWidth * hashHeight];
        
        for (int y = 0; y < hashHeight; y++) {
            for (int x = 0; x < hashWidth; x++) {
                int srcX = x * width / hashWidth;
                int srcY = y * height / hashHeight;
                resized[y * hashWidth + x] = img[srcY * width + srcX];
            }
        }
        
        stbi_image_free(img);
        
        // Calculate dHash
        uint64_t hash = 0;
        int bitIndex = 0;
        
        for (int y = 0; y < hashHeight; y++) {
            for (int x = 0; x < hashWidth - 1; x++) {
                int left = resized[y * hashWidth + x];
                int right = resized[y * hashWidth + (x + 1)];
                
                if (left < right) {
                    hash |= (1ULL << bitIndex);
                }
                bitIndex++;
            }
        }
        
        return hash;
    }

    int hammingDistance(uint64_t hash1, uint64_t hash2) {
        uint64_t diff = hash1 ^ hash2;
        int distance = 0;
        while (diff) {
            distance += diff & 1;
            diff >>= 1;
        }
        return distance;
    }

    double calculateStringSimilarity(const std::string& s1, const std::string& s2) {
        std::string s1_lower = s1, s2_lower = s2;
        std::transform(s1_lower.begin(), s1_lower.end(), s1_lower.begin(), ::tolower);
        std::transform(s2_lower.begin(), s2_lower.end(), s2_lower.begin(), ::tolower);
        
        if (s1_lower == s2_lower) return 1.0;
        if (s1_lower.find(s2_lower) != std::string::npos) return 0.8;
        if (s2_lower.find(s1_lower) != std::string::npos) return 0.8;
        
        // Simple character comparison
        int common = 0;
        for (char c1 : s1_lower) {
            for (char c2 : s2_lower) {
                if (c1 == c2) common++;
            }
        }
        
        int total = s1_lower.length() + s2_lower.length();
        return total > 0 ? (2.0 * common) / total : 0.0;
    }
    
    std::string extractTextContent(const FileInfo& file) {
        if (file.type == "document") {
            std::ifstream fs(file.path);
            if (!fs) return "";
            
            std::string content;
            std::string line;
            int lineCount = 0;
            while (std::getline(fs, line) && lineCount < 50) {
                content += line + "\n";
                lineCount++;
            }
            return content;
        }
        return "";
    }
    
    std::set<std::string> extractWords(const std::string& text) {
        std::set<std::string> words;
        std::stringstream ss(text);
        std::string word;
        
        while (ss >> word) {
            word.erase(std::remove_if(word.begin(), word.end(), 
                      [](char c) { return !std::isalnum(c); }), word.end());
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            if (word.length() > 2) {
                words.insert(word);
            }
        }
        return words;
    }
    
    double calculateTextSimilarity(const std::string& text1, const std::string& text2) {
        if (text1.empty() || text2.empty()) return 0.0;
        
        std::set<std::string> words1 = extractWords(text1);
        std::set<std::string> words2 = extractWords(text2);
        
        int common = 0;
        for (const auto& word : words1) {
            if (words2.count(word)) common++;
        }
        
        int total = words1.size() + words2.size() - common;
        return total > 0 ? (double)common / total : 0.0;
    }
    
    bool areDocumentsSimilar(const FileInfo& doc1, const FileInfo& doc2) {
        // Size similarity
        double sizeRatio = (double)std::min(doc1.size_bytes, doc2.size_bytes) 
                         / std::max(doc1.size_bytes, doc2.size_bytes);
        
        if (sizeRatio < 0.3) return false;
        
        // Filename similarity
        std::string name1 = fs::path(doc1.path).stem().string();
        std::string name2 = fs::path(doc2.path).stem().string();
        
        if (calculateStringSimilarity(name1, name2) > 0.7) {
            return true;
        }
        
        // Content similarity for text files
        if (doc1.path.find(".txt") != std::string::npos || 
            doc1.path.find(".csv") != std::string::npos) {
            std::string content1 = extractTextContent(doc1);
            std::string content2 = extractTextContent(doc2);
            return calculateTextSimilarity(content1, content2) > 0.6;
        }
        
        return false;
    }
    
    bool areArchivesSimilar(const FileInfo& arch1, const FileInfo& arch2) {
        double sizeRatio = (double)std::min(arch1.size_bytes, arch2.size_bytes) 
                         / std::max(arch1.size_bytes, arch2.size_bytes);
        
        std::string name1 = fs::path(arch1.path).stem().string();
        std::string name2 = fs::path(arch2.path).stem().string();
        
        return sizeRatio > 0.8 && calculateStringSimilarity(name1, name2) > 0.6;
    }
    bool areImagesSimilar(const FileInfo& img1, const FileInfo& img2) {
    // Use only size similarity for images
    double sizeRatio = (double)std::min(img1.size_bytes, img2.size_bytes) 
                     / std::max(img1.size_bytes, img2.size_bytes);
    
    return sizeRatio > 0.8; // 80% size similarity
    }    
    bool areAudioSimilar(const FileInfo& audio1, const FileInfo& audio2) {
        std::string name1 = fs::path(audio1.path).stem().string();
        std::string name2 = fs::path(audio2.path).stem().string();
        
        // Very strict: only names that are almost identical
        std::string name1_lower = name1;
        std::string name2_lower = name2;
        std::transform(name1_lower.begin(), name1_lower.end(), name1_lower.begin(), ::tolower);
        std::transform(name2_lower.begin(), name2_lower.end(), name2_lower.begin(), ::tolower);
        
        // Only match if names are identical or one is prefix of the other with numbers
        if (name1_lower == name2_lower) return true;
        
        // Check if one is base of the other (like "song" and "song1")
        if ((name1_lower + "1") == name2_lower || (name2_lower + "1") == name1_lower) return true;
        if ((name1_lower + "2") == name2_lower || (name2_lower + "2") == name1_lower) return true;
        
        // Very high similarity threshold
        return calculateStringSimilarity(name1, name2) > 0.9;
    }
    
    bool areFilesSimilar(const FileInfo& file1, const FileInfo& file2) {
        if (file1.type != file2.type) return false;
        
        if (file1.type == "image") {
            return areImagesSimilar(file1, file2);
        } else if (file1.type == "audio") {
            return areAudioSimilar(file1, file2);
        } else if (file1.type == "document") {
            return areDocumentsSimilar(file1, file2);
        } else if (file1.type == "other") {
            return areArchivesSimilar(file1, file2);
        }
        
        return false;
    }
};

// ---------------------------------------------------------
// FileScanner class
// ---------------------------------------------------------
class FileScanner {
private:
    SimilarityFinder similarityFinder;

public:
    std::vector<FileInfo> findFiles(const std::string& directory);
    std::string calculateHash(const std::string& filePath);
    std::map<std::string, std::vector<FileInfo>> findDuplicates(const std::vector<FileInfo>& files);
    std::vector<std::vector<FileInfo>> findSimilarFiles(const std::vector<FileInfo>& files);
};

// ---------------------------------------------------------
// Method implementations
// ---------------------------------------------------------
std::vector<FileInfo> FileScanner::findFiles(const std::string& directory) {
    std::vector<FileInfo> results;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                bool is_img = (ext == ".jpg" || ext == ".jpeg" || ext == ".png" ||
                               ext == ".bmp" || ext == ".webp" || ext == ".tiff");
                bool is_audio = (ext == ".mp3" || ext == ".flac" || ext == ".wav" ||
                                 ext == ".aac" || ext == ".ogg" || ext == ".m4a");
                bool is_doc = (ext == ".txt" || ext == ".pdf" || ext == ".docx" ||
                               ext == ".xlsx" || ext == ".csv" || ext == ".pptx");
                bool is_other = (ext == ".zip" || ext == ".rar" || ext == ".7z" || ext == ".exe");

                if (is_img || is_audio || is_doc || is_other) {
                    FileInfo info;
                    info.path = path;
                    info.size_bytes = entry.file_size();
                    if (is_img) info.type = "image";
                    else if (is_audio) info.type = "audio";
                    else if (is_doc) info.type = "document";
                    else info.type = "other";

                    results.push_back(info);
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << "Error scanning directory: " << e.what() << std::endl;
    }

    return results;
}

std::string FileScanner::calculateHash(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return "";

#ifdef _WIN32
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return "";

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }

    const size_t bufferSize = 8192;
    char buffer[bufferSize];

    while (file.read(buffer, bufferSize) || file.gcount() > 0) {
        if (!CryptHashData(hHash, (BYTE*)buffer, (DWORD)file.gcount(), 0)) {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }
    }

    BYTE hash[32];
    DWORD hashLen = 32;
    CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0);

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    std::stringstream ss;
    for (int i = 0; i < 32; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();

#else
    std::stringstream ss;
    const size_t bufferSize = 8192;
    char buffer[bufferSize];
    size_t hash = 0;
    while (file.read(buffer, bufferSize) || file.gcount() > 0) {
        for (int i = 0; i < file.gcount(); i++)
            hash = hash * 31 + buffer[i];
    }
    ss << std::hex << hash;
    return ss.str();
#endif
}

std::map<std::string, std::vector<FileInfo>> FileScanner::findDuplicates(
    const std::vector<FileInfo>& files) {

    std::map<std::string, std::vector<FileInfo>> duplicates;
    std::cout << "Calculating hashes...\n";

    int processed = 0;
    for (const auto& file : files) {
        std::string hash = calculateHash(file.path);
        if (!hash.empty()) duplicates[hash].push_back(file);

        processed++;
        if (processed % 10 == 0) {
            std::cout << "Processed " << processed << "/" << files.size() << " files...\r";
            std::cout.flush();
        }
    }

    std::cout << "\nDone calculating hashes!\n\n";

    for (auto it = duplicates.begin(); it != duplicates.end();) {
        if (it->second.size() < 2)
            it = duplicates.erase(it);
        else
            ++it;
    }

    return duplicates;
}

std::vector<std::vector<FileInfo>> FileScanner::findSimilarFiles(const std::vector<FileInfo>& files) {
    std::vector<std::vector<FileInfo>> similarGroups;
    std::vector<bool> processed(files.size(), false);
    
    std::cout << "Finding similar files...\n";
    
    for (size_t i = 0; i < files.size(); i++) {
        if (processed[i]) continue;
        
        std::vector<FileInfo> group;
        group.push_back(files[i]);
        
        for (size_t j = i + 1; j < files.size(); j++) {
            if (!processed[j] && similarityFinder.areFilesSimilar(files[i], files[j])) {
                group.push_back(files[j]);
                processed[j] = true;
            }
        }
        
        if (group.size() > 1) {
            similarGroups.push_back(group);
        }
        
        if ((i + 1) % 10 == 0) {
            std::cout << "Processed " << (i + 1) << "/" << files.size() << " files...\r";
            std::cout.flush();
        }
    }
    
    std::cout << "\nDone finding similar files!\n\n";
    
    return similarGroups;
}

// ---------------------------------------------------------
// Main program
// ---------------------------------------------------------
std::string selectDirectory() {
    return "C:\\Users\\dimi1\\Downloads\\Bilder_Dimitrios";
}

int main() {
    // Simple UTF-8 fix for Windows
    #ifdef _WIN32
    system("chcp 65001 > nul");
    #endif
    
    std::string directory = selectDirectory();
    FileScanner scanner;

    auto files = scanner.findFiles(directory);
    std::cout << "\n=== MediaDuplicateFinder ===\n";
    std::cout << "Found " << files.size() << " files\n\n";

    if (files.empty()) {
        std::cout << "No files found!\n";
        return 0;
    }

    // Find exact duplicates
    auto duplicates = scanner.findDuplicates(files);

    if (duplicates.empty()) {
        std::cout << "No exact duplicates found!\n";
    } else {
        std::cout << "Found " << duplicates.size() << " groups of exact duplicates:\n\n";
        int groupNum = 1;
        for (const auto& [hash, fileList] : duplicates) {
            std::cout << "Group " << groupNum++ << " (" << fileList.size() << " files):\n";
            for (const auto& file : fileList)
                std::cout << "  - " << file.path << " (" << file.size_bytes
                          << " bytes, type: " << file.type << ")\n";
            std::cout << "\n";
        }
    }

    // Build set of files that are exact duplicates (to exclude from similarity search)
    std::set<std::string> exactDuplicatePaths;
    for (const auto& [hash, fileList] : duplicates) {
        for (const auto& file : fileList) {
            exactDuplicatePaths.insert(file.path);
        }
    }

    // Filter out exact duplicates from similarity search
    std::vector<FileInfo> filesForSimilarity;
    for (const auto& file : files) {
        if (exactDuplicatePaths.find(file.path) == exactDuplicatePaths.end()) {
            filesForSimilarity.push_back(file);
        }
    }

    // Find similar files (only among non-exact-duplicates)
    if (!filesForSimilarity.empty()) {
        auto similarFiles = scanner.findSimilarFiles(filesForSimilarity);
        
        if (!similarFiles.empty()) {
            std::cout << "\n=== SIMILAR FILES (not exact duplicates) ===" << std::endl;
            std::cout << "Found " << similarFiles.size() << " groups of similar files:\n\n";
            int groupNum = 1;
            for (const auto& group : similarFiles) {
                std::cout << "Similar Group " << groupNum++ << " (" << group.size() << " files):\n";
                for (const auto& file : group) {
                    std::cout << "  - " << file.path << " (" << file.size_bytes 
                              << " bytes, type: " << file.type << ")\n";
                }
                std::cout << std::endl;
            }
        } else {
            std::cout << "\nNo similar files found!\n";
        }
    }

    return 0;
}