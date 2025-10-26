class ConsoleGUI {
// private:
//     FileScanner scanner;
//     std::string currentDirectory;

// public:
//     void showMenu() {
//         while(true) {
//             system("cls");  // Clear screen (Windows)
            
//             // Coole ASCII Art
//             std::cout << R"(
//   __  __      _ _   _      ______      _ _       _   _             
//  |  \\/  |    | (_) (_)    |  ____|    | (_)     | | (_)            
//  | \\  / |  __| |_  _  ___| |__  __  __| |_  ___ | |_ _  ___  _ __  
//  | |\\/| | / _` | | | |/ _ \\  __| \\ \\/ /| | |/ _ \\| __| |/ _ \\| '_ \\ 
//  | |  | || (_| | | | |  __/ |____ >  < | | | (_) | |_| | (_) | | | |
//  |_|  |_| \\__,_|_|_| |\\___|______/_/\\_\\|_|_|\\___/ \\__|_|\\___/|_| |_|
//                    _/ |                                             
//                   |__/                                              
// )" << std::endl;

//             std::cout << std::string(60, '=') << "\n";
//             std::cout << "                  MAIN MENU\n";
//             std::cout << std::string(60, '=') << "\n";
            
//             // Directory info
//             if(currentDirectory.empty()) {
//                 std::cout << " 📁 Directory: [NOT SET]\n";
//             } else {
//                 std::cout << " 📁 Directory: " << currentDirectory << "\n";
//             }
            
//             std::cout << std::string(60, '-') << "\n";
//             std::cout << " 1. 📂 Set Directory\n";
//             std::cout << " 2. 🔍 Scan for Duplicates\n"; 
//             std::cout << " 3. 🔄 Scan for Similar Files\n";
//             std::cout << " 4. 🚀 Full Scan (Duplicates + Similar)\n";
//             std::cout << " 5. 🗑️  Delete Duplicates\n";
//             std::cout << " 6. ❌ Exit\n";
//             std::cout << std::string(60, '-') << "\n";
//             std::cout << " Select option: ";
            
//             char choice = _getch();
//             std::cout << choice << "\n";
            
//             switch(choice) {
//                 case '1': setDirectory(); break;
//                 case '2': scanDuplicates(); break;
//                 case '3': scanSimilar(); break;
//                 case '4': fullScan(); break;
//                 case '5': deleteDuplicates(); break;
//                 case '6': return;
//                 default: 
//                     std::cout << " ❌ Invalid option! Press any key...";
//                     _getch();
//             }
//         }
//     }
    
//     void setDirectory() {
//         system("cls");
//         std::cout << " 📂 SET DIRECTORY\n";
//         std::cout << std::string(40, '=') << "\n";
//         std::cout << " Current: " << (currentDirectory.empty() ? "[NOT SET]" : currentDirectory) << "\n\n";
//         std::cout << " Enter directory path: ";
        
//         std::string newDir;
//         std::getline(std::cin, newDir);
        
//         if(!newDir.empty()) {
//             currentDirectory = newDir;
//             std::cout << " ✅ Directory set!\n";
//         } else {
//             std::cout << " ❌ No directory entered!\n";
//         }
        
//         std::cout << "\n Press any key to continue...";
//         _getch();
//     }
    
//     void scanDuplicates() {
//         if(!checkDirectory()) return;
        
//         system("cls");
//         std::cout << " 🔍 SCANNING FOR DUPLICATES\n";
//         std::cout << std::string(40, '=') << "\n";
//         std::cout << " Scanning: " << currentDirectory << "\n\n";
        
//         auto files = scanner.findFiles(currentDirectory);
//         std::cout << " 📊 Found " << files.size() << " files\n\n";
        
//         if(files.empty()) {
//             std::cout << " ❌ No files found!\n";
//             std::cout << "\n Press any key to continue...";
//             _getch();
//             return;
//         }
        
//         std::cout << " 🔄 Calculating hashes...\n";
//         auto duplicates = scanner.findDuplicates(files);
        
//         displayDuplicates(duplicates);
//     }
    
//     void scanSimilar() {
//         if(!checkDirectory()) return;
        
//         system("cls");
//         std::cout << " 🔄 SCANNING FOR SIMILAR FILES\n";
//         std::cout << std::string(40, '=') << "\n";
        
//         auto files = scanner.findFiles(currentDirectory);
//         auto duplicates = scanner.findDuplicates(files);
        
//         // Filter out exact duplicates
//         std::set<std::string> exactDuplicatePaths;
//         for(const auto& [hash, fileList] : duplicates) {
//             for(const auto& file : fileList) {
//                 exactDuplicatePaths.insert(file.path);
//             }
//         }
        
//         std::vector<FileInfo> filesForSimilarity;
//         for(const auto& file : files) {
//             if(exactDuplicatePaths.find(file.path) == exactDuplicatePaths.end()) {
//                 filesForSimilarity.push_back(file);
//             }
//         }
        
//         std::cout << " 🔍 Finding similar files...\n";
//         auto similarFiles = scanner.findSimilarFiles(filesForSimilarity);
        
//         displaySimilarFiles(similarFiles);
//     }
    
//     void fullScan() {
//         if(!checkDirectory()) return;
        
//         system("cls");
//         std::cout << " 🚀 FULL SCAN\n";
//         std::cout << std::string(40, '=') << "\n";
        
//         auto files = scanner.findFiles(currentDirectory);
//         std::cout << " 📊 Found " << files.size() << " files\n\n";
        
//         // Duplicates
//         std::cout << " 🔄 Finding duplicates...\n";
//         auto duplicates = scanner.findDuplicates(files);
//         displayDuplicates(duplicates);
        
//         // Similar files
//         std::set<std::string> exactDuplicatePaths;
//         for(const auto& [hash, fileList] : duplicates) {
//             for(const auto& file : fileList) {
//                 exactDuplicatePaths.insert(file.path);
//             }
//         }
        
//         std::vector<FileInfo> filesForSimilarity;
//         for(const auto& file : files) {
//             if(exactDuplicatePaths.find(file.path) == exactDuplicatePaths.end()) {
//                 filesForSimilarity.push_back(file);
//             }
//         }
        
//         std::cout << "\n\n 🔍 Finding similar files...\n";
//         auto similarFiles = scanner.findSimilarFiles(filesForSimilarity);
//         displaySimilarFiles(similarFiles);
        
//         std::cout << "\n\n 🎉 Scan complete! Press any key...";
//         _getch();
//     }
    
//     void deleteDuplicates() {
//         std::cout << " 🗑️  Delete feature coming soon...\n";
//         std::cout << " Press any key to continue...";
//         _getch();
//     }

// private:
//     bool checkDirectory() {
//         if(currentDirectory.empty()) {
//             std::cout << " ❌ Please set directory first!\n";
//             std::cout << " Press any key to continue...";
//             _getch();
//             return false;
//         }
//         return true;
//     }
    
//     void displayDuplicates(const std::map<std::string, std::vector<FileInfo>>& duplicates) {
//         if(duplicates.empty()) {
//             std::cout << " ✅ No duplicates found!\n";
//             return;
//         }
        
//         std::cout << "\n 🎯 DUPLICATES FOUND:\n";
//         std::cout << std::string(40, '-') << "\n";
        
//         int groupNum = 1;
//         int totalDuplicates = 0;
        
//         for(const auto& [hash, fileList] : duplicates) {
//             std::cout << " Group " << groupNum++ << " (" << fileList.size() << " files):\n";
//             for(const auto& file : fileList) {
//                 std::string filename = std::filesystem::path(file.path).filename().string();
//                 std::cout << "   📄 " << filename << " (" << file.size_bytes << " bytes, " << file.type << ")\n";
//             }
//             totalDuplicates += fileList.size() - 1;
//             std::cout << "\n";
//         }
        
//         std::cout << " 📈 Total duplicate files: " << totalDuplicates << "\n";
//     }
    
//     void displaySimilarFiles(const std::vector<std::vector<FileInfo>>& similarFiles) {
//         if(similarFiles.empty()) {
//             std::cout << " ✅ No similar files found!\n";
//             return;
//         }
        
//         std::cout << "\n 🔍 SIMILAR FILES FOUND:\n";
//         std::cout << std::string(40, '-') << "\n";
        
//         int groupNum = 1;
        
//         for(const auto& group : similarFiles) {
//             std::cout << " Similar Group " << groupNum++ << " (" << group.size() << " files):\n";
//             for(const auto& file : group) {
//                 std::string filename = std::filesystem::path(file.path).filename().string();
//                 std::cout << "   📄 " << filename << " (" << file.size_bytes << " bytes, " << file.type << ")\n";
//             }
//             std::cout << "\n";
//         }
//     }
// };