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
#include <unordered_map>

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

    // ================== BATCH PROCESSING ==================
    std::map<size_t, ComparisonResult> compareOfficeFilesBatch(
        const std::vector<ComparisonPair>& comparisons) {
        
        if (comparisons.empty()) return {};
        
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
        
        std::string inputFile = "temp_input_" + std::to_string(std::time(nullptr)) + ".json";
        std::string outputFile = "temp_output_" + std::to_string(std::time(nullptr)) + ".json";
        
        std::ofstream outf(inputFile);
        outf << jsonInput.str();
        outf.close();
        
        std::string currentDir = fs::current_path().string();
        
        #ifdef _WIN32
            std::string command = "cd /d \"" + currentDir + "\" && python office_comparer_batch.py < \"" 
                                  + inputFile + "\" > \"" + outputFile + "\"";
        #else
            std::string command = "cd \"" + currentDir + "\" && python3 office_comparer_batch.py < \"" 
                                  + inputFile + "\" > \"" + outputFile + "\"";
        #endif
        
        int result = system(command.c_str());
        
        std::map<size_t, ComparisonResult> results;
        if (result == 0) {
            std::ifstream inf(outputFile);
            std::stringstream buffer;
            buffer << inf.rdbuf();
            std::string jsonOutput = buffer.str();
            inf.close();
            
            results = parseJsonResults(jsonOutput, comparisons);
        }
        
        std::remove(inputFile.c_str());
        std::remove(outputFile.c_str());
        return results;
    }

    // ================== FALLBACK METHODS ==================
    std::pair<bool, double> areWordSimilarFallback(const ::FileInfo& doc1, const ::FileInfo& doc2) {
        std::string name1 = fs::path(doc1.path).stem().string();
        std::string name2 = fs::path(doc2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        return {nameSim > 0.7, nameSim};
    }
    
    std::pair<bool, double> areExcelSimilarFallback(const ::FileInfo& xls1, const ::FileInfo& xls2) {
        double sizeRatio = (double)std::min(xls1.size_bytes, xls2.size_bytes) 
                         / std::max(xls1.size_bytes, xls2.size_bytes);
        std::string name1 = fs::path(xls1.path).stem().string();
        std::string name2 = fs::path(xls2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        
        bool similar = (sizeRatio > 0.8) && (nameSim > 0.7);
        return {similar, similar ? (sizeRatio + nameSim) / 2.0 : 0.0};
    }
    
    std::pair<bool, double> arePowerPointSimilarFallback(const ::FileInfo& ppt1, const ::FileInfo& ppt2) {
        std::string name1 = fs::path(ppt1.path).stem().string();
        std::string name2 = fs::path(ppt2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        return {nameSim > 0.7, nameSim};
    }

    // ================== IMAGE COMPARISON ==================
    struct ImageData {
        unsigned char* pixels;
        int width;
        int height;
        int channels;
    };

    ImageData loadGrayscaleImage(const std::string& path) {
        ImageData img;
        img.pixels = stbi_load(path.c_str(), &img.width, &img.height, &img.channels, 1);
        return img;
    }

    uint64_t calculateAverageHash(const ImageData& img) {
        const int hashSize = 8;
        const int resizedWidth = hashSize;
        const int resizedHeight = hashSize;

        std::vector<unsigned char> resized(resizedWidth * resizedHeight);
        float xRatio = static_cast<float>(img.width) / resizedWidth;
        float yRatio = static_cast<float>(img.height) / resizedHeight;

        for (int y = 0; y < resizedHeight; ++y) {
            for (int x = 0; x < resizedWidth; ++x) {
                int srcX = static_cast<int>(x * xRatio);
                int srcY = static_cast<int>(y * yRatio);
                resized[y * resizedWidth + x] = img.pixels[srcY * img.width + srcX];
            }
        }

        double avg = 0.0;
        for (auto val : resized) avg += val;
        avg /= resized.size();

        uint64_t hash = 0;
        for (auto val : resized) hash = (hash << 1) | (val > avg ? 1 : 0);
        return hash;
    }

    uint64_t calculateDifferenceHash(const ImageData& img) {
        const int hashSize = 8;
        const int resizedWidth = hashSize + 1;
        const int resizedHeight = hashSize;

        std::vector<unsigned char> resized(resizedWidth * resizedHeight);
        float xRatio = static_cast<float>(img.width) / resizedWidth;
        float yRatio = static_cast<float>(img.height) / resizedHeight;

        for (int y = 0; y < resizedHeight; ++y) {
            for (int x = 0; x < resizedWidth; ++x) {
                int srcX = static_cast<int>(x * xRatio);
                int srcY = static_cast<int>(y * yRatio);
                resized[y * resizedWidth + x] = img.pixels[srcY * img.width + srcX];
            }
        }

        uint64_t hash = 0;
        for (int y = 0; y < hashSize; ++y)
            for (int x = 0; x < hashSize; ++x)
                hash = (hash << 1) | (resized[y * resizedWidth + x] > resized[y * resizedWidth + x + 1] ? 1 : 0);
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

    std::pair<bool, double> areImagesSimilar(const ::FileInfo& img1, const ::FileInfo& img2) {
        ImageData data1 = loadGrayscaleImage(img1.path);
        ImageData data2 = loadGrayscaleImage(img2.path);
        if (!data1.pixels || !data2.pixels) return {false, 0.0};

        uint64_t dhash1 = calculateDifferenceHash(data1);
        uint64_t dhash2 = calculateDifferenceHash(data2);
        uint64_t ahash1 = calculateAverageHash(data1);
        uint64_t ahash2 = calculateAverageHash(data2);

        stbi_image_free(data1.pixels);
        stbi_image_free(data2.pixels);

        if (!dhash1 || !dhash2 || !ahash1 || !ahash2) return {false, 0.0};

        int dDistance = hammingDistance(dhash1, dhash2);
        int aDistance = hammingDistance(ahash1, ahash2);
        double similarity = (1.0 - dDistance / 64.0 + 1.0 - aDistance / 64.0) / 2.0;
        bool similar = ((dDistance + aDistance) / 2.0) <= 15;

        return {similar, similar ? similarity : 0.0};
    }

    // ================== AUDIO COMPARISON ==================
    std::pair<bool, double> areAudioSimilar(const ::FileInfo& audio1, const ::FileInfo& audio2) {
        std::string name1 = fs::path(audio1.path).stem().string();
        std::string name2 = fs::path(audio2.path).stem().string();
        std::transform(name1.begin(), name1.end(), name1.begin(), ::tolower);
        std::transform(name2.begin(), name2.end(), name2.begin(), ::tolower);

        if (name1 == name2) return {true, 1.0};
        if ((name1 + "1") == name2 || (name2 + "1") == name1) return {true, 0.95};
        if ((name1 + "2") == name2 || (name2 + "2") == name1) return {true, 0.95};

        double nameSim = calculateStringSimilarity(name1, name2);
        return {nameSim > 0.9, nameSim};
    }

    // ================== TEXT/DOCUMENT COMPARISON ==================
    std::string extractTextContent(const ::FileInfo& file) {
        std::ifstream fs(file.path);
        if (!fs) return "";
        std::string content, line;
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
                        [](char c){ return !std::isalnum(c); }), word.end());
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            if (word.length() > 2) words.insert(word);
        }
        return words;
    }

    double calculateTextSimilarity(const std::string& text1, const std::string& text2) {
        if (text1.empty() || text2.empty()) return 0.0;
        std::set<std::string> words1 = extractWords(text1);
        std::set<std::string> words2 = extractWords(text2);

        int common = 0;
        for (auto& w : words1) if (words2.count(w)) common++;
        int total = words1.size() + words2.size() - common;
        return total > 0 ? (double)common / total : 0.0;
    }

    std::pair<bool, double> areDocumentsSimilar(const ::FileInfo& doc1, const ::FileInfo& doc2) {
        double sizeRatio = (double)std::min(doc1.size_bytes, doc2.size_bytes) 
                         / std::max(doc1.size_bytes, doc2.size_bytes);
        if (sizeRatio < 0.3) return {false, 0.0};

        std::string name1 = fs::path(doc1.path).stem().string();
        std::string name2 = fs::path(doc2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        if (nameSim > 0.7) return {true, nameSim};

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

    // ================== ARCHIVE COMPARISON ==================
    std::pair<bool, double> areArchivesSimilar(const ::FileInfo& arch1, const ::FileInfo& arch2) {
        double sizeRatio = (double)std::min(arch1.size_bytes, arch2.size_bytes) 
                         / std::max(arch1.size_bytes, arch2.size_bytes);
        std::string name1 = fs::path(arch1.path).stem().string();
        std::string name2 = fs::path(arch2.path).stem().string();
        double nameSim = calculateStringSimilarity(name1, name2);
        bool similar = sizeRatio > 0.8 && nameSim > 0.6;
        return {similar, similar ? (sizeRatio + nameSim)/2.0 : 0.0};
    }

    // ================== MAIN DISPATCHER ==================
    std::pair<bool, double> areFilesSimilar(const ::FileInfo& file1, const ::FileInfo& file2) {
        if (file1.type != file2.type) return {false, 0.0};

        if (file1.type == "image") return areImagesSimilar(file1, file2);
        else if (file1.type == "audio") return areAudioSimilar(file1, file2);
        else if (file1.type == "text") return areDocumentsSimilar(file1, file2);
        else if (file1.type == "other") return areArchivesSimilar(file1, file2);
        return {false, 0.0};
    }

    // ================== UTILITY ==================
    double calculateStringSimilarity(const std::string& s1, const std::string& s2) {
        std::string s1_lower = s1, s2_lower = s2;
        std::transform(s1_lower.begin(), s1_lower.end(), s1_lower.begin(), ::tolower);
        std::transform(s2_lower.begin(), s2_lower.end(), s2_lower.begin(), ::tolower);

        if (s1_lower == s2_lower) return 1.0;
        if (s1_lower.find(s2_lower) != std::string::npos) return 0.8;
        if (s2_lower.find(s1_lower) != std::string::npos) return 0.8;

        int common = 0;
        for (char c1 : s1_lower)
            for (char c2 : s2_lower)
                if (c1 == c2) common++;

        int total = s1_lower.length() + s2_lower.length();
        return total > 0 ? (2.0 * common)/total : 0.0;
    }

private:
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
        size_t pos = 0, compIndex = 0;
        while ((pos = json.find("\"similar\"", pos)) != std::string::npos &&
               compIndex < comparisons.size()) {
            
            ComparisonResult result;
            size_t colonPos = json.find(":", pos);
            if (colonPos == std::string::npos) break;
            size_t valueStart = colonPos + 1;
            while (valueStart < json.length() && (json[valueStart] == ' ' || json[valueStart] == '\t')) valueStart++;
            if (json.substr(valueStart, 4) == "true") result.similar = true;
            else result.similar = false;

            size_t scorePos = json.find("\"score\"", pos);
            if (scorePos != std::string::npos && scorePos < json.find("}", pos)) {
                colonPos = json.find(":", scorePos);
                if (colonPos != std::string::npos) {
                    valueStart = colonPos + 1;
                    while (valueStart < json.length() && (json[valueStart] == ' ' || json[valueStart] == '\t')) valueStart++;
                    size_t endPos = json.find_first_of(",}", valueStart);
                    std::string scoreStr = json.substr(valueStart, endPos - valueStart);
                    try { result.score = std::stod(scoreStr); } catch(...) { result.score = 0.0; }
                }
            } else result.score = 0.0;

            size_t resultIndex = comparisons[compIndex].index;
            results[resultIndex] = result;
            compIndex++;
            pos = json.find("}", pos) + 1;
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


    int processed = 0;
    auto startTime = std::chrono::steady_clock::now();
    
    for (const auto& file : files) {
        std::string hash = calculateHash(file.path);
        if (!hash.empty()) duplicates[hash].push_back(file);

        processed++;
        
        if (processed % 20 == 0) {
            std::cerr << "Processed " << processed << "/" << files.size() << " files" << std::endl;
            // auto now = std::chrono::steady_clock::now();
            // double elapsed = std::chrono::duration<double>(now - startTime).count();
            // double eta = elapsed / processed * (files.size() - processed);
            
        }
    }

    

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
    std::unordered_map<std::string, int> filesPerType;
    for (const auto& file : files) {
        filesPerType[file.type]++;
    }
    
    // Calculate total comparisons for same-type files only
    size_t totalComparisons = 0;
    for (const auto& [type, count] : filesPerType) {
        totalComparisons += count * (count - 1) / 2;
    }
    
    
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
               
                
                comparisonIndexMap[{i, j}] = pair.index;
                officeComparisons.push_back(pair);
            }
        }
    }
    
    // ========== STEP 2: Execute batch Office comparison (ONLY ONCE!) ==========
  std::map<size_t, SimilarityFinder::ComparisonResult> officeResults;
if (!officeComparisons.empty()) {
    officeResults = similarityFinder.compareOfficeFilesBatch(officeComparisons);
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
                //Debug
                 
            }
            
            // Print progress every 50 comparisons
            if (doneComparisons % 20 == 0) {
                int progress = static_cast<int>((doneComparisons * 100.0) / totalComparisons);
                std::cerr << "\r[" << std::string(progress / 2, '#') 
                << std::string(50 - progress / 2, ' ') 
                << "] " << progress << "% (" 
                << doneComparisons << "/" << totalComparisons << ")   " << std::flush;
            }
        }
    }
    
            if (group.size() > 1) {
                similarGroups.push_back(group);  
            }
}


return similarGroups;
}
// ---------------------------------------------------------
// Main function
// ---------------------------------------------------------
int main(int argc, char* argv[]) {
    // ===== UTF-8 support for Windows consoles =====
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        std::setlocale(LC_ALL, ".UTF8");
    #endif
    
    // ===== Argument check =====
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
        std::cerr << "No files found in directory: " << directory << std::endl;
        return 0;
    }

    // ===== Step 1: Exact duplicates =====
    auto exactDuplicates = scanner.findExactDuplicates(files);

    // Print exact duplicate groups
    for (const auto& [hash, fileList] : exactDuplicates) {
        if (fileList.size() > 1) {
            std::cout << "EXACT|1.0" << std::endl;
            for (const auto& file : fileList)
                std::cout << file.path << std::endl;
            std::cout << "---GROUP---" << std::endl;
        }
    }

    // ===== Step 2: Similar files (optional) =====
    if (findSimilar) {
        // Exclude known exact duplicates (keep only first file per group)
        std::set<std::string> duplicatePaths;
        for (const auto& [hash, fileList] : exactDuplicates) {
            for (size_t i = 1; i < fileList.size(); ++i)
                duplicatePaths.insert(fileList[i].path);
        }

        // Filter files for similarity comparison
        std::vector<FileInfo> filesForSimilarity;
        filesForSimilarity.reserve(files.size());
        for (const auto& file : files) {
            if (duplicatePaths.find(file.path) == duplicatePaths.end())
                filesForSimilarity.push_back(file);
        }

        // Precompute total comparison workload
        std::unordered_map<std::string, int> filesPerType;
        for (const auto& file : filesForSimilarity)
            filesPerType[file.type]++;

        size_t totalWork = 0;
        for (const auto& [type, count] : filesPerType)
            totalWork += static_cast<size_t>(count) * (count - 1) / 2;

        std::cerr << "TOTAL_WORK: " << totalWork << std::endl;

        // Run similarity detection
        auto similarFiles = scanner.findSimilarFiles(filesForSimilarity);

        // Print similar file groups
        for (const auto& group : similarFiles) {
            if (group.size() < 2) continue;

            double avgScore = 0.0;
            for (const auto& f : group) avgScore += f.similarity_score;
            avgScore /= group.size();

            std::cout << "SIMILAR|" << std::fixed << std::setprecision(2) << avgScore << std::endl;
            for (const auto& f : group)
                std::cout << f.path << "|" << std::fixed << std::setprecision(2)
                          << f.similarity_score << std::endl;
            std::cout << "---GROUP---" << std::endl;
        }
    } else {
        // Similarity mode disabled → only print total file count
        std::cerr << "TOTAL_WORK: " << files.size() << std::endl;
    }

    return 0;
}
