# MediaDuplicateFinder

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)
[![Python](https://img.shields.io/badge/Python-3.8%2B-blue)](https://www.python.org/doc/)

**MediaDuplicateFinder** is a high-performance duplicate file finder that supports **any file type** — including images, audio, text, and Office documents.

It detects both **exact duplicates** (via SHA-256 hashing) and **similar files** (via perceptual hashing and content analysis), even across different formats, sizes, and quality levels.

**Architecture:**
- **C++ core** for high-speed file scanning and hash computation
- **Python GUI** (Tkinter) for intuitive file management and safe deletion
- **Batch processor** for parallel Office document comparison (`office_comparer_batch.py`)

---

## 📦 Download & Installation

### **Quick Start (Recommended)**
1. Download the latest precompiled version from [**Releases**](../../releases/latest) If you download `duplicate_finder_exe.zip` and extract it, you **don't need any other installations**.  
2. Extract the ZIP file
3. Ensure all three files are in the same directory:
   - `duplicate_finder.exe` 
   - `duplicate_gui.py`
   - `office_comparer_batch.py`
4. Install Python dependencies:
```bash
   pip install pillow numpy openpyxl python-docx python-pptx scikit-learn
```
5. Run the GUI:
```bash
   python duplicate_gui.py
```

### **System Requirements**
- **Python 3.8+**
- **Windows 7+** 
- Required Python packages: `pillow`, `numpy`, `openpyxl`, `python-docx`, `python-pptx`, `scikit-learn`

---

## 🚀 Key Features

- ⚡ **Ultra-fast duplicate detection** using SHA-256 hashing (Windows) or optimized hash algorithm (Linux/Mac)
- 🖼️ **Perceptual image similarity** via Average Hash and Difference Hash with Hamming distance
- 📊 **Batch Office file processing** — analyze hundreds of Word/Excel/PowerPoint files in parallel
- 📝 **Content-based text comparison** using TF-IDF vectorization and cosine similarity
- 🎯 **Smart two-pass scanning** — finds exact duplicates first, then similarities (avoids redundant work)
- 📈 **Real-time progress tracking** with live updates and ETA calculation
- 🛡️ **Safe deletion workflow** — preview before deleting, automatic file priority system
- 🎨 **Modern GUI** with filtering, statistics, and wasted space calculation
- 🔄 **Cross-platform support** (Windows, Linux, macOS)

---

## 🖥️ Usage Guide

### **1. Start the Application**
Run `duplicate_gui.py` to launch the GUI:

![Start](gui_pictures/gui_start.png)

---

### **2. Select Directory**
Click **Browse** and select the folder you want to scan. The tool will automatically scan all subfolders.

---

### **3. Start Scanning**
Click **Scan Duplicates & Similar Files** to begin the analysis:

![Working](gui_pictures/gui_working.png)

---

### **4. View Results**
Once the scan completes, you'll see:
- **Exact duplicate groups** (🔴) — identical files
- **Similar file groups** (🟡) — files with matching content but different formats/quality
- **Statistics** — total groups, files, and wasted disk space

![Finished](gui_pictures/gui_finished.png)

---

### **5. Delete Duplicates**

#### **Option A: Delete All Exact Duplicates**
Click **Delete All Exact Duplicates** to automatically remove all exact copies (keeps one file per group):

<p align="center">
  <img src="gui_pictures/gui_delete_duplicates_warning_window.png" alt="Delete Warning" width="60%">
</p>

---

#### **Option B: Delete Specific Files or Groups**
Right-click on any file or group for more options:

<p align="center">
  <img src="gui_pictures/gui_delete_file.png" alt="Delete File" width="48%">
  <img src="gui_pictures/gui_delete_group.png" alt="Delete Group" width="48%">
</p>

---

#### **Option C: Preview Similar Files Before Deletion**
For similar files (not exact duplicates), you can preview which files will be kept/deleted:

![Delete Similar](gui_pictures/gui_delete_similar_preview.png)

---

## 🔧 How It Works

### **Exact Duplicate Detection**
1. Recursively scan directory for supported file types
2. Calculate SHA-256 hash for each file (Windows) or fast custom hash (Linux/Mac)
3. Group files with identical hashes
4. Report groups with 2+ files

---

### **Similarity Detection**

| File Type | Method | Threshold |
|-----------|--------|-----------|
| **Images** | Average Hash + Difference Hash → Hamming distance | ≤15 bits difference |
| **Audio** | Filename + metadata similarity | >90% similarity |
| **Office (Word/PowerPoint)** | TF-IDF cosine similarity on extracted text | >60% similarity |
| **Office (Excel)** | Cell-by-cell comparison of sheet data | >70% match rate |
| **Text/PDF/CSV** | Word-based Jaccard similarity | >60% similarity |
| **Archives** | Size ratio + filename similarity | >80% size + >60% name |

---

### **Performance Optimizations**
- ⚡ **Batch processing**: All Office file comparisons collected and processed in one Python call
- 🔄 **Parallel Office processing**: Uses `multiprocessing.Pool` (N-1 CPU cores)
- 📦 **Two-pass scanning**: Excludes exact duplicates from similarity search
- 🎯 **Queue-based GUI updates**: Non-blocking progress tracking
- 💾 **Memory-efficient Excel loading**: openpyxl with `read_only=True` and `data_only=True`

---

## 🏗️ Architecture

MediaDuplicateFinder uses a **hybrid multi-language architecture** for optimal performance:

### **C++ Core** (`duplicate_finder.exe`)
- High-speed file scanning with recursive directory traversal
- SHA-256 hashing for exact duplicate detection (Windows Crypto API)
- Perceptual hashing for images using `stb_image` library
- Efficient batch collection for Office file comparisons
- Two-pass algorithm: exact duplicates → similarities

### **Python Backend** (`office_comparer_batch.py`)
- **Parallel processing** using `multiprocessing.Pool`
- Content extraction from Word (`python-docx`), Excel (`openpyxl`), PowerPoint (`python-pptx`)
- TF-IDF vectorization for text similarity (`scikit-learn`)
- Fast Excel comparison with read-only mode
- JSON-based inter-process communication

### **Python GUI** (`duplicate_gui.py`)
- Modern Tkinter interface with threaded scanning
- Real-time progress updates via queue-based communication
- Smart filtering (exact/similar/all)
- Wasted space calculation
- Safe deletion with preview and priority system

---

## 📁 Supported File Types

| Category | Extensions | Detection Method |
|----------|-----------|------------------|
| **Images** | `.jpg`, `.jpeg`, `.png`, `.bmp`, `.webp`, `.tiff` | Perceptual hashing (aHash + dHash) |
| **Audio** | `.mp3`, `.flac`, `.wav`, `.aac`, `.ogg`, `.m4a` | Filename/metadata similarity |
| **Office** | `.docx`, `.xlsx`, `.xls`, `.pptx` | Content extraction + batch comparison |
| **Text** | `.txt`, `.pdf`, `.csv` | Word-based content analysis |
| **Archives** | `.zip`, `.rar`, `.7z`, `.exe` | Size + filename similarity |

---

## 🧩 Components Overview

| Component | Language | Purpose |
|-----------|----------|---------|
| `duplicate_finder.exe` | C++ | Core scanner for exact and similar files |
| `duplicate_gui.py` | Python | GUI frontend to visualize and manage results |
| `office_comparer_batch.py` | Python | Specialized Office file similarity comparison |

---

## 🛠️ Build from Source

If you want to compile the C++ backend manually:

### **Windows (MinGW/MSVC)**
```bash
g++ -std=c++17 main_cli.cpp -o duplicate_finder
```

Make sure `stb_image.h` is in the same directory as `duplicate_finder.cpp`.

---

## 📜 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

### **Third-Party Libraries**

This project uses the following open-source libraries:

**C++ Libraries:**
- [stb_image](https://github.com/nothings/stb) - Public Domain / MIT License

**Python Libraries:**
- [python-docx](https://github.com/python-openxml/python-docx) - MIT License
- [python-pptx](https://github.com/scanny/python-pptx) - MIT License
- [openpyxl](https://openpyxl.readthedocs.io/) - MIT License
- [scikit-learn](https://scikit-learn.org/) - BSD 3-Clause License
- [Pillow](https://python-pillow.org/) - HPND License
- [NumPy](https://numpy.org/) - BSD License

All third-party licenses are compatible with the MIT License.

---

## ⭐ Support

If you find this project useful, please consider giving it a ⭐!
