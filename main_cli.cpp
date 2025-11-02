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
#include <chrono>
#include <ctime>
#include <locale>
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
// SimilarityFinder class (OPTIMIZED VERSION)
// ---------------------------------------------------------
class SimilarityFinder {
public:
    // Structs for batch processing
    struct ComparisonPair {
        std::string type;
        std::string file1;
        std::string file2;
        size_t index;
    };
    
    struct ComparisonResult {
        bool similar;
        double score;
    };

    // ========== BATCH PROCESSING METHOD (NEW) ==========
    std::map<size_t, ComparisonResult> compareOfficeFilesBatch(
        const std::vector<ComparisonPair>& comparisons) {
        
        if (comparisons.empty()) {
            return {};
        }
        
        
        
        // Build JSON input
        std::stringstream jsonInput;
        jsonInput << "[";
        for (size_t i = 0; i < comparisons.size(); i++) {
            if (i > 0) jsonInput << ",";
            jsonInput << "{\"type\":\"" << comparisons[i].type << "\","
                     << "\"file1\":\"" << escapeJsonString(comparisons[i].file1) << "\","
                     << "\"file2\":\"" << escapeJsonString(comparisons[i].file2) << "\"}";
        }
        jsonInput << "]";
        
        // Create temporary files for communication
        std::string inputFile = "temp_input_" + std::to_string(std::time(nullptr)) + ".json";
        std::string outputFile = "temp_output_" + std::to_string(std::time(nullptr)) + ".json";
        
        // Write input to file
        std::ofstream outf(inputFile);
        outf << jsonInput.str();
        outf.close();
        
        // Execute Python script
        std::string currentDir = fs::current_path().string();
        
        
        // #ifdef _WIN32
        //     std::string command = "cd /d \"" + currentDir + "\" && python office_comparer_batch.py < \"" 
        //                         + inputFile + "\" > \"" + outputFile + "\" 2>nul";
        // #else
        //     std::string command = "cd \"" + currentDir + "\" && python3 office_comparer_batch.py < \"" 
        //                         + inputFile + "\" > \"" + outputFile + "\" 2>/dev/null";
        // #endif
        #ifdef _WIN32
            std::string command = "cd /d \"" + currentDir + "\" && python office_comparer_batch.py < \"" 
                        + inputFile + "\" > \"" + outputFile + "\"";  // ← 2>nul entfernt!
        #else
            std::string command = "cd \"" + currentDir + "\" && python3 office_comparer_batch.py < \"" 
                        + inputFile + "\" > \"" + outputFile + "\"";  // ← 2>/dev/null entfernt!
        #endif
        
        
        int result = system(command.c_str());
        
        
        // Read results
        std::map<size_t, ComparisonResult> results;
        
        if (result == 0) {
            std::ifstream inf(outputFile);
            std::stringstream buffer;
            buffer << inf.rdbuf();
            std::string jsonOutput = buffer.str();
            inf.close();
            
            
            
            // Parse JSON output
            results = parseJsonResults(jsonOutput, comparisons);
        } else {
            
        }
        
        // Cleanup temp files
        std::remove(inputFile.c_str());
        std::remove(outputFile.c_str());
        
        return results;
    }

    // ========== FALLBACK METHODS ==========
    std::pair<bool, double> areWordSimilarFallback(const FileInfo& doc1, const FileInfo& doc2) {
        std::string name1 = fs::path(doc1.path).stem().string();
        std::string name2 = fs::path(doc2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        return {nameSim > 0.7, nameSim};
    }
    
    std::pair<bool, double> areExcelSimilarFallback(const FileInfo& xls1, const FileInfo& xls2) {
        double sizeRatio = (double)std::min(xls1.size_bytes, xls2.size_bytes) 
                         / std::max(xls1.size_bytes, xls2.size_bytes);
        std::string name1 = fs::path(xls1.path).stem().string();
        std::string name2 = fs::path(xls2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        
        bool similar = (sizeRatio > 0.8) && (nameSim > 0.7);
        return {similar, similar ? (sizeRatio + nameSim) / 2.0 : 0.0};
    }
    
    std::pair<bool, double> arePowerPointSimilarFallback(const FileInfo& ppt1, const FileInfo& ppt2) {
        std::string name1 = fs::path(ppt1.path).stem().string();
        std::string name2 = fs::path(ppt2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        return {nameSim > 0.7, nameSim};
    }

    // ========== IMAGE COMPARISON ==========
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

    uint64_t calculateAverageHash(const std::string& imagePath) {
        int width, height, channels;
        unsigned char* img = stbi_load(imagePath.c_str(), &width, &height, &channels, 1);
        if (!img) return 0;
        
        const int hashSize = 8;
        unsigned char resized[hashSize * hashSize];
        
        int total = 0;
        for (int i = 0; i < hashSize * hashSize; i++) {
            int srcX = (i % hashSize) * width / hashSize;
            int srcY = (i / hashSize) * height / hashSize;
            resized[i] = img[srcY * width + srcX];
            total += resized[i];
        }
        int average = total / (hashSize * hashSize);
        
        uint64_t hash = 0;
        for (int i = 0; i < hashSize * hashSize; i++) {
            if (resized[i] > average) {
                hash |= (1ULL << i);
            }
        }
        
        stbi_image_free(img);
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

    std::pair<bool, double> areImagesSimilar(const FileInfo& img1, const FileInfo& img2) {
        uint64_t dhash1 = calculateImageHash(img1.path);
        uint64_t dhash2 = calculateImageHash(img2.path);
        uint64_t ahash1 = calculateAverageHash(img1.path);
        uint64_t ahash2 = calculateAverageHash(img2.path);

        if (!dhash1 || !dhash2 || !ahash1 || !ahash2)
            return {false, 0.0};

        int dDistance = hammingDistance(dhash1, dhash2);
        int aDistance = hammingDistance(ahash1, ahash2);

        double dSim = 1.0 - (dDistance / 64.0);
        double aSim = 1.0 - (aDistance / 64.0);
        double similarity = (dSim + aSim) / 2.0;

        bool similar = ((dDistance + aDistance) / 2.0) <= 15;

        return {similar, similar ? similarity : 0.0};
    }

    // ========== AUDIO COMPARISON ==========
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

    // ========== TEXT/DOCUMENT COMPARISON ==========
    std::string extractTextContent(const FileInfo& file) {
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
        
        std::string name1 = fs::path(doc1.path).stem().string();
        std::string name2 = fs::path(doc2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        
        if (nameSim > 0.7) {
            return {true, nameSim};
        }
        
        if (doc1.path.find(".txt") != std::string::npos || 
            doc1.path.find(".csv") != std::string::npos ||
            doc1.path.find(".pdf") != std::string::npos) {
            std::string content1 = extractTextContent(doc1);
            std::string content2 = extractTextContent(doc2);
            double textSim = calculateTextSimilarity(content1, content2);
            return {textSim > 0.6, textSim};
        }
        
        return {false, 0.0};
    }

    // ========== ARCHIVE COMPARISON ==========
    std::pair<bool, double> areArchivesSimilar(const FileInfo& arch1, const FileInfo& arch2) {
        double sizeRatio = (double)std::min(arch1.size_bytes, arch2.size_bytes) 
                         / std::max(arch1.size_bytes, arch2.size_bytes);
        
        std::string name1 = fs::path(arch1.path).stem().string();
        std::string name2 = fs::path(arch2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        
        bool similar = sizeRatio > 0.8 && nameSim > 0.6;
        return {similar, similar ? (sizeRatio + nameSim) / 2.0 : 0.0};
    }

    // ========== MAIN DISPATCHER ==========
    std::pair<bool, double> areFilesSimilar(const FileInfo& file1, const FileInfo& file2) {
        if (file1.type != file2.type) return {false, 0.0};
        
        if (file1.type == "image") {
            return areImagesSimilar(file1, file2);
        } else if (file1.type == "audio") {
            return areAudioSimilar(file1, file2);
        } else if (file1.type == "text") {
            return areDocumentsSimilar(file1, file2);
        } else if (file1.type == "other") {
            return areArchivesSimilar(file1, file2);
        }
        // Note: word, excel, powerpoint are handled via batch
        return {false, 0.0};
    }

    // ========== UTILITY METHODS ==========
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

private:
    // ========== PRIVATE HELPER METHODS ==========
    std::string escapeJsonString(const std::string& str) {
        std::string escaped;
        for (char c : str) {
            switch (c) {
                case '\\': escaped += "\\\\"; break;
                case '\"': escaped += "\\\""; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += c;
            }
        }
        return escaped;
    }
    
std::map<size_t, ComparisonResult> parseJsonResults(
    const std::string& json, 
    const std::vector<ComparisonPair>& comparisons) {
    
    std::map<size_t, ComparisonResult> results;
    
    size_t pos = 0;
    size_t compIndex = 0;
    
    while ((pos = json.find("{\"similar\":", pos)) != std::string::npos && 
           compIndex < comparisons.size()) {
        
        ComparisonResult result;
        
        // Extract "similar" value - FIXED!
        size_t similarPos = json.find(":", pos) + 1;
        // Skip whitespace
        while (similarPos < json.length() && json[similarPos] == ' ') similarPos++;
        
        // Check for "true" or "false"
        if (json.substr(similarPos, 4) == "true") {
            result.similar = true;
        } else if (json.substr(similarPos, 5) == "false") {
            result.similar = false;
        } else {
            result.similar = false;
        }
        
        
        
        // Extract "score" value
        size_t scorePos = json.find("\"score\":", pos);
        if (scorePos != std::string::npos) {
            scorePos = json.find(":", scorePos) + 1;
            // Skip whitespace
            while (scorePos < json.length() && json[scorePos] == ' ') scorePos++;
            size_t endPos = json.find_first_of(",}", scorePos);
            std::string scoreStr = json.substr(scorePos, endPos - scorePos);
            result.score = std::stod(scoreStr);
        } else {
            result.score = 0.0;
        }
        
        results[comparisons[compIndex].index] = result;
        compIndex++;
        pos++;
    }
    
    return results;
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
        for (const auto& entry : fs::recursive_directory_iterator(directory, 
            fs::directory_options::skip_permission_denied)) {
            
            try {  // Innerer try-catch für einzelne Dateien
                if (entry.is_regular_file()) {
                    std::string path = entry.path().string();
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    
                    FileInfo info;
                    info.path = path;
                    info.size_bytes = entry.file_size();
                    
                    // WICHTIG: Separate types für Office-Dateien!
                    if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" ||
                        ext == ".bmp" || ext == ".webp" || ext == ".tiff") {
                        info.type = "image";
                        results.push_back(info);
                    } else if (ext == ".mp3" || ext == ".flac" || ext == ".wav" ||
                              ext == ".aac" || ext == ".ogg" || ext == ".m4a") {
                        info.type = "audio";
                        results.push_back(info);
                    } else if (ext == ".docx") {
                        info.type = "word";
                        results.push_back(info);
                    } else if (ext == ".xlsx" || ext == ".xls") {
                        info.type = "excel";
                        results.push_back(info);
                    } else if (ext == ".pptx") {
                        info.type = "powerpoint";
                        results.push_back(info);
                    } else if (ext == ".txt" || ext == ".pdf" || ext == ".csv") {
                        info.type = "text";
                        results.push_back(info);
                    } else if (ext == ".zip" || ext == ".rar" || ext == ".7z" || ext == ".exe") {
                        info.type = "other";
                        results.push_back(info);
                    }
                }
            } catch (const std::exception& e) {
                // Skip einzelne problematische Dateien
                std::cerr << "Warning: Skipping file - " << e.what() << std::endl;
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
        std::cerr << "Problematic path: " << e.path1() << std::endl;
    }
    
    std::cerr << "Found " << results.size() << " files total" << std::endl;
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
    std::cerr << "Calculating hashes for exact duplicates..." << std::endl;

    int processed = 0;
    auto startTime = std::chrono::steady_clock::now();
    
    for (const auto& file : files) {
        std::string hash = calculateHash(file.path);
        if (!hash.empty()) duplicates[hash].push_back(file);

        processed++;
        
        if (processed % 20 == 0) {
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - startTime).count();
            double eta = elapsed / processed * (files.size() - processed);
            std::cerr << "Processed " << processed << "/" << files.size()
                      << " files, ETA: " << (int)eta << "s" << std::endl;
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
    
    // Count comparisons per file type
    std::map<std::string, int> filesPerType;
    for (const auto& file : files) {
        filesPerType[file.type]++;
    }
    
    // Calculate total comparisons for same-type files only
    size_t totalComparisons = 0;
    for (const auto& [type, count] : filesPerType) {
        totalComparisons += count * (count - 1) / 2;
    }
    
    std::cerr << "Starting similarity scan with " << totalComparisons << " comparisons..." << std::endl;
    
    // ========== STEP 1: Collect all Office file comparisons ==========
    std::vector<SimilarityFinder::ComparisonPair> officeComparisons;
    std::map<std::pair<size_t, size_t>, size_t> comparisonIndexMap;
    
    for (size_t i = 0; i < files.size(); i++) {
        if (processed[i]) continue;
        
        for (size_t j = i + 1; j < files.size(); j++) {
            if (processed[j] || files[i].type != files[j].type) continue;
            
            // Collect Office file comparisons for batch processing
            if (files[i].type == "word" || files[i].type == "excel" || files[i].type == "powerpoint") {
                SimilarityFinder::ComparisonPair pair;
                pair.type = files[i].type;
                pair.file1 = files[i].path;
                pair.file2 = files[j].path;
                pair.index = officeComparisons.size();
                std::cerr << "🔥 Collecting: " << pair.type << " - " 
              << fs::path(pair.file1).filename() << " vs " 
              << fs::path(pair.file2).filename() << std::endl;  // ← NEU
                
                comparisonIndexMap[{i, j}] = pair.index;
                officeComparisons.push_back(pair);
            }
        }
    }
    
    // ========== STEP 2: Execute batch Office comparison (ONLY ONCE!) ==========
  std::map<size_t, SimilarityFinder::ComparisonResult> officeResults;
if (!officeComparisons.empty()) {
    std::cerr << "Processing " << officeComparisons.size() 
              << " Office file comparisons in batch..." << std::endl;
    
    officeResults = similarityFinder.compareOfficeFilesBatch(officeComparisons);
    
    
    std::cerr << "Office batch comparison complete!" << std::endl;
}

// ========== STEP 3: Process all files and use cached Office results ==========
size_t doneComparisons = 0;
auto startTime = std::chrono::steady_clock::now();

for (size_t i = 0; i < files.size(); i++) {
    if (processed[i]) continue;
    
    std::vector<FileInfo> group;
    FileInfo firstFile = files[i];
    firstFile.similarity_score = 1.0;
    group.push_back(firstFile);
    
    for (size_t j = i + 1; j < files.size(); j++) {
        if (!processed[j] && files[i].type == files[j].type) {
            bool similar = false;
            double score = 0.0;
            
            // Use pre-computed Office results
            if (files[i].type == "word" || files[i].type == "excel" || files[i].type == "powerpoint") {
                auto it = comparisonIndexMap.find({i, j});
                if (it != comparisonIndexMap.end()) {
                    size_t batchIndex = it->second;
                    auto resultIt = officeResults.find(batchIndex);
                    
                    
                    if (resultIt != officeResults.end()) {
                        similar = resultIt->second.similar;
                        score = resultIt->second.score;
                        
                    } else {
                        
                        // Fallback if batch failed for this pair
                        if (files[i].type == "word") {
                            auto result = similarityFinder.areWordSimilarFallback(files[i], files[j]);
                            similar = result.first;
                            score = result.second;
                        } else if (files[i].type == "excel") {
                            auto result = similarityFinder.areExcelSimilarFallback(files[i], files[j]);
                            similar = result.first;
                            score = result.second;
                        } else if (files[i].type == "powerpoint") {
                            auto result = similarityFinder.arePowerPointSimilarFallback(files[i], files[j]);
                            similar = result.first;
                            score = result.second;
                        }
                    }
                }
            } else {
                // Non-Office files: use existing methods
                auto result = similarityFinder.areFilesSimilar(files[i], files[j]);
                similar = result.first;
                score = result.second;
            }
            
            doneComparisons++;
            
            if (similar) {
                FileInfo similarFile = files[j];
                similarFile.similarity_score = score;
                group.push_back(similarFile);
                processed[j] = true;
                
                
            }
            
            // Print progress every 50 comparisons
            if (doneComparisons % 50 == 0) {
                auto now = std::chrono::steady_clock::now();
                double elapsed = std::chrono::duration<double>(now - startTime).count();
                double eta = elapsed / doneComparisons * (totalComparisons - doneComparisons);
                std::cerr << "Processed " << doneComparisons << "/" << totalComparisons
                          << " comparisons, ETA: " << (int)eta << "s" << std::endl;
            }
        }
    }
    
    if (group.size() > 1) {
        similarGroups.push_back(group);
        
    }
}

std::cerr << "Similarity scan complete!" << std::endl;
return similarGroups;
}
// ---------------------------------------------------------
// Main function
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    // Windows: UTF-8 Support aktivieren
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        std::setlocale(LC_ALL, ".UTF8");
    #endif
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory> [--similar]" << std::endl;
        return 1;
    }
    
    std::string directory = argv[1];
    bool findSimilar = (argc >= 3 && std::string(argv[2]) == "--similar");
    
    std::cerr << "Scanning directory: " << directory << std::endl;
    
    FileScanner scanner;
    auto files = scanner.findFiles(directory);
    
    if (files.empty()) {
        std::cerr << "No files found" << std::endl;
        return 0;
    }
    
    std::cerr << "Found " << files.size() << " files to process" << std::endl;
    
    // Start with just file count
    size_t totalWork = files.size();
    
    auto exactDuplicates = scanner.findExactDuplicates(files);
    
    // Output exact duplicates
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
        // Filter out exact duplicates
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
        
        // Calculate comparisons AFTER filtering
        std::map<std::string, int> filesPerType;
        for (const auto& file : filesForSimilarity) {
            filesPerType[file.type]++;
        }
        
        for (const auto& [type, count] : filesPerType) {
            totalWork += count * (count - 1) / 2;
        }
        
        // Output total work AFTER calculating everything
        std::cerr << "TOTAL_WORK:" << totalWork << std::endl;
        
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
    } else {
        // If not finding similar, output total work now
        std::cerr << "TOTAL_WORK:" << totalWork << std::endl;
    }
    
    return 0;
}