#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <map>
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
    double similarity_score = 0.0;
};

// ---------------------------------------------------------
// SimilarityFinder class
// ---------------------------------------------------------
class SimilarityFinder {
public:
    std::pair<bool, double> areExcelSimilar(const FileInfo& xls1, const FileInfo& xls2) {
        std::string currentDir = fs::current_path().string();
        
        #ifdef _WIN32
            std::string command = "cd /d \"" + currentDir + "\" && python excel_comparer.py \"" + xls1.path + "\" \"" + xls2.path + "\" 2>nul";
        #else
            std::string command = "cd \"" + currentDir + "\" && python3 excel_comparer.py \"" + xls1.path + "\" \"" + xls2.path + "\" 2>/dev/null";
        #endif
        
        int result = system(command.c_str());
        
        if (result != 0) {
            double sizeRatio = (double)std::min(xls1.size_bytes, xls2.size_bytes) 
                             / std::max(xls1.size_bytes, xls2.size_bytes);
            std::string name1 = fs::path(xls1.path).stem().string();
            std::string name2 = fs::path(xls2.path).stem().string();
            double nameSim = calculateStringSimilarity(name1, name2);
            
            bool similar = (sizeRatio > 0.8) && (nameSim > 0.7);
            return {similar, similar ? (sizeRatio + nameSim) / 2.0 : 0.0};
        }
        
        return {result == 0, 0.85};
    }

    uint64_t calculateImageHash(const std::string& imagePath) {
        int width, height, channels;
        unsigned char* img = stbi_load(imagePath.c_str(), &width, &height, &channels, 1);
        
        if (!img) return 0;
        
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
    
    std::pair<bool, double> areDocumentsSimilar(const FileInfo& doc1, const FileInfo& doc2) {
        double sizeRatio = (double)std::min(doc1.size_bytes, doc2.size_bytes) 
                         / std::max(doc1.size_bytes, doc2.size_bytes);
        
        if (sizeRatio < 0.3) return {false, 0.0};
        
        if ((doc1.path.find(".xlsx") != std::string::npos || 
             doc1.path.find(".xls") != std::string::npos) &&
            (doc2.path.find(".xlsx") != std::string::npos || 
             doc2.path.find(".xls") != std::string::npos)) {
            return areExcelSimilar(doc1, doc2);
        }
        
        if ((doc1.path.find(".docx") != std::string::npos) &&
            (doc2.path.find(".docx") != std::string::npos)) {
            return areWordSimilar(doc1, doc2);  
        }
        
        if ((doc1.path.find(".pptx") != std::string::npos) &&
            (doc2.path.find(".pptx") != std::string::npos)) {
            return arePowerPointSimilar(doc1, doc2);
        }
        
        std::string name1 = fs::path(doc1.path).stem().string();
        std::string name2 = fs::path(doc2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        
        if (nameSim > 0.7) {
            return {true, nameSim};
        }
        
        if (doc1.path.find(".txt") != std::string::npos || 
            doc1.path.find(".csv") != std::string::npos) {
            std::string content1 = extractTextContent(doc1);
            std::string content2 = extractTextContent(doc2);
            double textSim = calculateTextSimilarity(content1, content2);
            return {textSim > 0.6, textSim};
        }
        
        return {false, 0.0};
    }
    
    std::pair<bool, double> areArchivesSimilar(const FileInfo& arch1, const FileInfo& arch2) {
        double sizeRatio = (double)std::min(arch1.size_bytes, arch2.size_bytes) 
                         / std::max(arch1.size_bytes, arch2.size_bytes);
        
        std::string name1 = fs::path(arch1.path).stem().string();
        std::string name2 = fs::path(arch2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        
        bool similar = sizeRatio > 0.8 && nameSim > 0.6;
        return {similar, similar ? (sizeRatio + nameSim) / 2.0 : 0.0};
    }
    
    std::pair<bool, double> areImagesSimilar(const FileInfo& img1, const FileInfo& img2) {
        uint64_t hash1 = calculateImageHash(img1.path);
        uint64_t hash2 = calculateImageHash(img2.path);
        
        if (hash1 == 0 || hash2 == 0) {
            return {false, 0.0};
        }
        
        int distance = hammingDistance(hash1, hash2);
        double similarity = 1.0 - (distance / 64.0);
        
        bool similar = distance <= 10;
        return {similar, similar ? similarity : 0.0};
    }
    
    std::pair<bool, double> areAudioSimilar(const FileInfo& audio1, const FileInfo& audio2) {
        std::string name1 = fs::path(audio1.path).stem().string();
        std::string name2 = fs::path(audio2.path).stem().string();
        
        std::string name1_lower = name1;
        std::string name2_lower = name2;
        std::transform(name1_lower.begin(), name1_lower.end(), name1_lower.begin(), ::tolower);
        std::transform(name2_lower.begin(), name2_lower.end(), name2_lower.begin(), ::tolower);
        
        if (name1_lower == name2_lower) return {true, 1.0};
        
        if ((name1_lower + "1") == name2_lower || (name2_lower + "1") == name1_lower) return {true, 0.95};
        if ((name1_lower + "2") == name2_lower || (name2_lower + "2") == name1_lower) return {true, 0.95};
        
        double nameSim = calculateStringSimilarity(name1, name2);
        return {nameSim > 0.9, nameSim};
    }
    
    std::pair<bool, double> areWordSimilar(const FileInfo& doc1, const FileInfo& doc2) {
        std::string currentDir = fs::current_path().string();
        
        #ifdef _WIN32
            std::string command = "cd /d \"" + currentDir + "\" && python word_comparer.py \"" + doc1.path + "\" \"" + doc2.path + "\" 2>nul";
        #else
            std::string command = "cd \"" + currentDir + "\" && python3 word_comparer.py \"" + doc1.path + "\" \"" + doc2.path + "\" 2>/dev/null";
        #endif
        
        int result = system(command.c_str());
        
        if (result != 0) {
            std::string name1 = fs::path(doc1.path).stem().string();
            std::string name2 = fs::path(doc2.path).stem().string();
            double nameSim = calculateStringSimilarity(name1, name2);
            return {nameSim > 0.7, nameSim};
        }
        
        return {result == 0, 0.85};
    }
    
    std::pair<bool, double> arePowerPointSimilar(const FileInfo& ppt1, const FileInfo& ppt2) {
        std::string currentDir = fs::current_path().string();
        
        #ifdef _WIN32
            std::string command = "cd /d \"" + currentDir + "\" && python powerpoint_comparer.py \"" + ppt1.path + "\" \"" + ppt2.path + "\" 2>nul";
        #else
            std::string command = "cd \"" + currentDir + "\" && python3 powerpoint_comparer.py \"" + ppt1.path + "\" \"" + ppt2.path + "\" 2>/dev/null";
        #endif
        
        int result = system(command.c_str());
        return {result == 0, result == 0 ? 0.85 : 0.0};
    }
    
    std::pair<bool, double> areFilesSimilar(const FileInfo& file1, const FileInfo& file2) {
        if (file1.type != file2.type) return {false, 0.0};
        
        if (file1.type == "image") {
            return areImagesSimilar(file1, file2);
        } else if (file1.type == "audio") {
            return areAudioSimilar(file1, file2);
        } else if (file1.type == "document") {
            return areDocumentsSimilar(file1, file2);
        } else if (file1.type == "other") {
            return areArchivesSimilar(file1, file2);
        }
        
        return {false, 0.0};
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
    std::map<std::string, std::vector<FileInfo>> findExactDuplicates(const std::vector<FileInfo>& files);
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
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
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

std::map<std::string, std::vector<FileInfo>> FileScanner::findExactDuplicates(
    const std::vector<FileInfo>& files) {

    std::map<std::string, std::vector<FileInfo>> duplicates;
    std::cerr << "Calculating hashes..." << std::endl;

    int processed = 0;
    for (const auto& file : files) {
        std::string hash = calculateHash(file.path);
        if (!hash.empty()) duplicates[hash].push_back(file);

        processed++;
        if (processed % 10 == 0) {
            std::cerr << "Processed " << processed << "/" << files.size() << " files..." << std::endl;
        }
    }

    std::cerr << "Done calculating hashes!" << std::endl;

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
    
    std::cerr << "Finding similar files..." << std::endl;
    
    for (size_t i = 0; i < files.size(); i++) {
        if (processed[i]) continue;
        
        std::vector<FileInfo> group;
        FileInfo firstFile = files[i];
        firstFile.similarity_score = 1.0;
        group.push_back(firstFile);
        
        for (size_t j = i + 1; j < files.size(); j++) {
            if (!processed[j]) {
                auto [similar, score] = similarityFinder.areFilesSimilar(files[i], files[j]);
                if (similar) {
                    FileInfo similarFile = files[j];
                    similarFile.similarity_score = score;
                    group.push_back(similarFile);
                    processed[j] = true;
                }
            }
        }
        
        if (group.size() > 1) {
            similarGroups.push_back(group);
        }
        
        if ((i + 1) % 10 == 0) {
            std::cerr << "Processed " << (i + 1) << "/" << files.size() << " files..." << std::endl;
        }
    }
    
    std::cerr << "Done finding similar files!" << std::endl;
    
    return similarGroups;
}

// ---------------------------------------------------------
// Main function
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory> [--similar]" << std::endl;
        return 1;
    }
    
    std::string directory = argv[1];
    bool findSimilar = (argc >= 3 && std::string(argv[2]) == "--similar");
    
    FileScanner scanner;
    auto files = scanner.findFiles(directory);
    
    if (files.empty()) {
        std::cerr << "No files found" << std::endl;
        return 0;
    }
    
    auto exactDuplicates = scanner.findExactDuplicates(files);
    
    for (const auto& [hash, fileList] : exactDuplicates) {
        if (fileList.size() > 1) {
            std::cout << "EXACT|1.0" << std::endl;
            for (const auto& file : fileList) {
                std::cout << file.path << std::endl;
            }
            std::cout << "---GROUP---" << std::endl;
        }
    }
    
    if (findSimilar) {
        std::set<std::string> exactDupPaths;
        for (const auto& [hash, fileList] : exactDuplicates) {
            for (const auto& file : fileList) {
                exactDupPaths.insert(file.path);
            }
        }
        
        std::vector<FileInfo> filesForSimilarity;
        for (const auto& file : files) {
            if (exactDupPaths.find(file.path) == exactDupPaths.end()) {
                filesForSimilarity.push_back(file);
            }
        }
        
        auto similarFiles = scanner.findSimilarFiles(filesForSimilarity);
        
        for (const auto& group : similarFiles) {
            if (group.size() > 1) {
                double avgScore = 0.0;
                for (const auto& file : group) {
                    avgScore += file.similarity_score;
                }
                avgScore /= group.size();
                
                std::cout << "SIMILAR|" << std::fixed << std::setprecision(2) << avgScore << std::endl;
                for (const auto& file : group) {
                    std::cout << file.path << "|" << std::fixed << std::setprecision(2) 
                              << file.similarity_score << std::endl;
                }
                std::cout << "---GROUP---" << std::endl;
            }
        }
    }
    
    return 0;
}