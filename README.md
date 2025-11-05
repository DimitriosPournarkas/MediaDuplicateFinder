# MediaDuplicateFinder

**MediaDuplicateFinder** is a high-performance duplicate file finder that supports **any file type** — including images, audio, text, and Office documents.  
It detects both **exact duplicates** and **similar files**, even across different formats, sizes, and quality levels.

The core scanning engine is written in **C++** for maximum speed and efficiency.  
A **Python GUI** provides an intuitive interface for scanning, filtering, and safely deleting duplicates.  
For Office document comparison, the script `office_comparer_batch.py` is used.

---

## 🚀 Key Features
- Ultra-fast duplicate detection for large file collections  
- Finds **similar files** using perceptual hashing (images) and audio fingerprinting (audio)  
- Supports text and Office file comparison  
- Modern **Tkinter-based GUI** for easy file browsing and duplicate management  
- Cross-platform support (Windows & Linux)  

---

## 🧩 Components Overview
| Component | Language | Purpose |
|------------|-----------|----------|
| `duplicate_finder.exe` | C++ | Core scanner for exact and similar files |
| `duplicate_gui.py` | Python | GUI frontend to visualize and manage results |
| `office_comparer_batch.py` | Python | Specialized Office file similarity comparison |

---

## 🖥️ Usage
1. Run **`duplicate_finder.exe`** (C++ core) to perform the initial scan.  
2. Launch **`duplicate_gui.py`** (or the compiled `duplicate_gui.exe`) to browse, filter, and delete duplicates.  
3. Optionally, run **`office_comparer_batch.py`** to compare Word, Excel, or PowerPoint files.

---

## ⚙️ Requirements
- **Python 3.8+**
- Required Python packages:
  ```bash
  pip install pillow numpy openpyxl python-docx pydub tk
```

