# MediaDuplicateFinder
**MediaDuplicateFinder** is a high-performance duplicate file finder that supports **any file type** — including images, audio, text, and Office documents.

It detects both **exact duplicates** (via SHA-256 hashing) and **similar files** (via perceptual hashing and content analysis), even across different formats, sizes, and quality levels.

**Architecture:**
- **C++ core** for high-speed file scanning and hash computation
- **Python GUI** (Tkinter) for intuitive file management and safe deletion
- **Batch processor** for parallel Office document comparison (`office_comparer_batch.py`)

---

## 📦 Download
The latest precompiled version (including `duplicate_gui.py`, `office_comparer_batch.py`, and `duplicate_finder.exe`) can be downloaded from the [**Releases** section](../../releases/latest).  
Simply download the ZIP file, extract it, and run `duplicate_gui.py`.  
Make sure that all three files are located in the same directory.


---

## 🚀 Key Features

- **⚡ Ultra-fast duplicate detection** using SHA-256 hashing (Windows) or optimized hash algorithm (Linux/Mac)
- **🖼️ Perceptual image similarity** via Average Hash and Difference Hash with Hamming distance
- **📊 Batch Office file processing** — analyze hundreds of Word/Excel/PowerPoint files in parallel
- **📝 Content-based text comparison** using TF-IDF vectorization and cosine similarity
- **🎯 Smart two-pass scanning** — finds exact duplicates first, then similarities (avoids redundant work)
- **📈 Real-time progress tracking** with live updates and ETA calculation
- **🛡️ Safe deletion workflow** — preview before deleting, automatic file priority system
- **🎨 Modern GUI** with filtering, statistics, and wasted space calculation
- **🔄 Cross-platform support** (Windows, Linux, macOS)

---

## 🧩 Components Overview
| Component | Language | Purpose |
|------------|-----------|----------|
| `duplicate_finder.exe` | C++ | Core scanner for exact and similar files |
| `duplicate_gui.py` | Python | GUI frontend to visualize and manage results |
| `office_comparer_batch.py` | Python | Specialized Office file similarity comparison |

---

## ⚙️ Requirements
- **Python 3.8+**
- Required Python packages:
  ```bash
  pip install pillow numpy openpyxl python-docx python-pptx scikit-learn pydub
The following files **must be in the same directory**:

- `duplicate_finder.exe` – C++ backend
- `duplicate_gui.py` – GUI frontend
- `office_comparer_batch.py` – Office comparison script

---

## 🖥️ Usage
1. Run **`duplicate_gui.py`**
![Start](gui_pictures/gui_start.png)
2. Click on **Browse** and select the folder you want to scan.
3. Click on **Scan Duplicates & Similar Files**. It will automatically scan all subfolders.
![Working](gui_pictures/gui_working.png)
4. Finished Scan: You can see the results.
![Finished](gui_pictures/gui_finished.png)
5. Delete files: You can now click on **Delete All Exact Duplicates**, or delete similar files manually, or delete similar groups.
<p align="center">
  <img src="gui_pictures/gui_delete_file.png" alt="Delete File" width="48%">
  <img src="gui_pictures/gui_delete_duplicates_warning_window.png" alt="Delete Warning" width="48%">
</p

![Delete_group](gui_pictures/gui_delete_group.png)

![Delete_similar](gui_pictures/gui_delete_similar_preview.png)



---

## 🔧 How It Works

### **Exact Duplicate Detection**
1. Recursively scan directory for supported file types
2. Calculate SHA-256 hash for each file (Windows) or fast custom hash (Linux/Mac)
3. Group files with identical hashes
4. Report groups with 2+ files

### **Similarity Detection**

| File Type | Method | Threshold |
|-----------|--------|-----------|
| **Images** | Average Hash + Difference Hash → Hamming distance | ≤15 bits difference |
| **Audio** | Filename + metadata similarity | >90% similarity |
| **Office (Word/PowerPoint)** | TF-IDF cosine similarity on extracted text | >60% similarity |
| **Office (Excel)** | Cell-by-cell comparison of sheet data | >70% match rate |
| **Text/PDF/CSV** | Word-based Jaccard similarity | >60% similarity |
| **Archives** | Size ratio + filename similarity | >80% size + >60% name |

### **Performance Optimizations**
- ⚡ **Batch processing**: All Office file comparisons collected and processed in one Python call
- 🔄 **Parallel Office processing**: Uses multiprocessing.Pool (N-1 CPU cores)
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
- **Parallel processing** using multiprocessing.Pool
- Content extraction from Word (python-docx), Excel (openpyxl), PowerPoint (python-pptx)
- TF-IDF vectorization for text similarity (scikit-learn)
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
## 🛠️ Build Instructions

If you want to build the C++ backend manually:
```bash
g++ -std=c++17 main_cli.cpp -o duplicate_finder
